// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright 2025 F&S Elektronik Systeme GmbH
 * Hartmut Keller <keller@fs-net.de>
 *
 * Handle F&S nboot.fs and uboot.fs images.
 *
 * For a description of the NBoot file format see:
 *  - board/F+S/common/fs_image_spl.c for fsimx8mm/mn/mp
 *  - board/F+S/common/fs_cntr_common.c for fsimx8ulp and fsimx91/93
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>			/* open() */
#include <unistd.h>			/* read(), close() */
#include <sys/stat.h>			/* fstat() */
#include <linux/libfdt.h>
#include <linux/kconfig.h>
#include <linux/mmc/ioctl.h>		/* mmc_ioc_cmd_set_data(), ... */
#include <sys/ioctl.h>			/* ioctl() */
#include <command.h>
#include <ctype.h>			/* tolower() */
#include <linux/compiler_attributes.h>
#include "linux_helpers.h"
#include "../../board/F+S/common/fs_image_common.h"
#include "../../board/F+S/common/fs_board_common.h" /* fs_board_get_boot_dev_from_name() */
#include "../../include/imx_container.h"

/* From kernel's linux/mmc/mmc.h */
#define MMC_SWITCH                6   /* ac   [31:0] See below   R1b */
#define MMC_SEND_EXT_CSD          8   /* adtc                    R1  */
#define MMC_SWITCH_MODE_WRITE_BYTE      0x03    /* Set target to value */

#define EXT_CSD_CMD_SET_NORMAL          (1<<0)
#define EXT_CSD_PART_CONFIG             179     /* R/W */
#define EXT_CSD_BOOT_MULT               226     /* RO */

#define MMC_RSP_NONE	0			/* no response */
#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_CMD_AC	(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	(2 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_BUSY (1 << 10)		/* card may send busy */

#define MMC_RSP_SPI_R1	(MMC_RSP_SPI_S1)
#define MMC_RSP_SPI_R1B	(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY)

#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)

#define MAX_IMAGE_SIZE (4 * 1024 * 1024)
#define MAX_BOARD_CFG_SIZE 0x2000

#define SYS_BDINFO "/sys/bdinfo/"
#define SYS_ARCH SYS_BDINFO "arch"
#define SYS_BOARD_ID SYS_BDINFO "board-id"
#define SYS_BOOT_DEV SYS_BDINFO "boot_dev"

static u8 image_ram[2*MAX_IMAGE_SIZE];
static u8 board_cfg[MAX_BOARD_CFG_SIZE];
static char board_id[MAX_DESCR_LEN + 1];
static u8 ext_csd[512];

#if 0 //###
static int load_saved_nboot(void)
{
	size_t bytes_read;
	const char *fname = "/dev/mmcblk0boot1";
	FILE *hwpart = fopen(fname, "ro");

	if (!hwpart) {
		fprintf(stderr, "Error opening %s, exiting...\n", fname);
		return -ENOENT;
	}

	bytes_read = fread(saved_nboot_buffer + FSH_SIZE, 1,
			   MAX_NBOOT_SIZE - FSH_SIZE, hwpart);
	if (!bytes_read) {
		fprintf(stderr, "Error reading from %s, exiting...\n", fname);
		fclose(hwpart);
		return -EINVAL;
	}
	fclose(hwpart);

	return 0;
}
#endif

#if 0 //###
//### TODO: Statt sich an den Containern entlang zu hangeln, sollte man
//### einfach gezielt das BOARD-ID Image über die F&S-Header-Kette suchen.
int extract_board_config(void)
{
	u64 search = (u64)(saved_nboot_buffer + 0x40);
	int i;
	char board_id[MAX_DESCR_LEN + 1];
	char *point;
	struct fs_header_v1_0* fsh;
	struct fs_header_v1_0 *cfg_fsh;
	void *cfg_img;
	struct index_info idx_info;

	/* Search the second container header, i.e. the BOARD-INFO container */
	for (i = 0; i < 2; i++) {
		do {
			/* Next container is 1K aligned */
			search += 0x400;
		} while (!valid_container_hdr((struct container_hdr *)search));
	}

	/* Get BOARD-ID which is two F&S headers before */
	fsh = (struct fs_header_v1_0 *)(search - 2 * FSH_SIZE);
	strncpy(board_id, fsh->param.descr, MAX_DESCR_LEN);
	board_id[MAX_DESCR_LEN] = 0;

	/* Strip board revision */
	point = strchr(board_id, '.');
	if (point)
		*point = 0;

	/* Find the BOARD-CFG header for this BOARD-ID in index */
	cfg_fsh = fs_image_find(++fsh , "BOARD-CFG", board_id, &idx_info);

	/* Find the real BOARD-CFG image */
	cfg_img = fs_image_find_cfg_fdt_idx(&idx_info);

	/* Copy header and image to saved_board_cfg_buffer[] */
	memcpy(saved_board_cfg_buffer, (void *)cfg_fsh, FSH_SIZE);
	memcpy(saved_board_cfg_buffer + FSH_SIZE, cfg_img,
	       fs_image_get_size(cfg_fsh, false));

	/* Check if BOARD-CFG is valid */
	if (!fs_image_is_ocram_cfg_valid()) {
		fprintf(stderr, "Error, no valid BOARD-CFG found.\n");
		return -EINVAL;
	}

	/*
	 * Set the current board_id name and the compare_id that is used in
	 * fs_image_find_board_cfg().
	 */
	fs_image_set_board_id_from_cfg();

	return 0;
}
#endif

int read_extcsd(int fd)
{
	struct mmc_ioc_cmd idata = {};

	memset(ext_csd, 0, sizeof(u8) * 512);
	idata.write_flag = 0;
	idata.opcode = MMC_SEND_EXT_CSD;
	idata.arg = 0;
	idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	idata.blksz = 512;
	idata.blocks = 1;
	mmc_ioc_cmd_set_data(idata, ext_csd);

	if (ioctl(fd, MMC_IOC_CMD, &idata) == -1)
		return errno;

	return 0;
}

static void fill_switch_cmd(struct mmc_ioc_cmd *cmd, __u8 index, __u8 value)
{
	cmd->opcode = MMC_SWITCH;
	cmd->write_flag = 1;
	cmd->arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) |
		   (value << 8) | EXT_CSD_CMD_SET_NORMAL;
	cmd->flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
}

static int write_extcsd_value(int fd, u8 index, u8 value, uint timeout_ms)
{
	struct mmc_ioc_cmd idata = {};

	fill_switch_cmd(&idata, index, value);

	/* Kernel will set cmd_timeout_ms if 0 is set */
	idata.cmd_timeout_ms = timeout_ms;

	if (ioctl(fd, MMC_IOC_CMD, &idata) == -1)
		return -errno;

	return 0;
}

/* ------------- Functions needed to avoid large libraries ----------------- */

u32 fdt_getprop_u32_default_node(const void *fdt, int off, int cell,
				const char *prop, const u32 dflt)
{
	const fdt32_t *val;
	int len;

	val = fdt_getprop(fdt, off, prop, &len);

	/* Check if property exists */
	if (!val)
		return dflt;

	/* Check if property is long enough */
	if (len < ((cell + 1) * sizeof(uint32_t)))
		return dflt;

	return fdt32_to_cpu(*val);
}

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0)
#endif

#define FIT_DATA_SIZE_PROP	"data-size"
/**
 * Get 'data-size' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_size: holds the data-size property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_size(const void *fit, int noffset, int *data_size)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_SIZE_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_size = fdt32_to_cpu(*val);

	return 0;
}

#define FIT_DATA_OFFSET_PROP	"data-offset"
/**
 * Get 'data-offset' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_offset: holds the data-offset property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_OFFSET_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_offset = fdt32_to_cpu(*val);

	return 0;
}

/**
 * Get 'data-position' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_position: holds the data-position property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */

#define FIT_DATA_POSITION_PROP	"data-position"
int fit_image_get_data_position(const void *fit, int noffset,
				int *data_position)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_POSITION_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_position = fdt32_to_cpu(*val);

	return 0;
}

/**
 * fit_get_name - get FIT node name
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     NULL, on error
 *     pointer to node name, on success
 */
static inline const char *fit_get_name(const void *fit_hdr,
				       int noffset, int *len)
{
	return fdt_get_name(fit_hdr, noffset, len);
}

static void fit_get_debug(const void *fit, int noffset,
			  char *prop_name, int err)
{
	debug("Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
	      prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
	      fdt_strerror(err));
}

#define FIT_DATA_PROP		"data"
/**
 * fit_image_get_data - get data property and its size for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @data: double pointer to void, will hold data property's data address
 * @size: pointer to size_t, will hold data property's data size
 *
 * fit_image_get_data() finds data property in a given component image node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_data(const void *fit, int noffset,
		       const void **data, size_t *size)
{
	int len;

	*data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
	if (*data == NULL) {
		fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
		*size = 0;
		return -1;
	}

	*size = len;
	return 0;
}

#if 0 //###
// Digest is for compatibility between nboot and linux function signature,
// always NULL when called and unused for Linux implementatiom.
ulong parse_loadaddr(char *filename, void *digest) {
	FILE *file = fopen(filename, "ro");
	if(!file) {
		printf("Error opening %s, exiting...\n", filename);
		return -ENOENT;
	}
	size_t bytes_read = fread(nboot_buffer, 1, 4*1024*1024, file);
	if(!bytes_read) {
		printf("Error reading data from %s, exiting...\n", filename);
		return -EINVAL;
	}
	fclose(file);
	return (ulong)nboot_buffer;
}

ulong get_loadaddr(void){
	return (ulong)saved_nboot_buffer;
}
#endif

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base) {
	return strtoul(cp, endp, base);
}

long simple_strtol(const char *cp, char **endp, unsigned int base) {
	return strtol(cp, endp, base);
}

//TODO: DD klappt das??
#define cpu_to_fdt32(x) __builtin_bswap32(x)

#if 0 //### kann vermutlich weg
/**
 * fdt_find_and_setprop: Find a node and set it's property
 *
 * @fdt: ptr to device tree
 * @node: path of node
 * @prop: property name
 * @val: ptr to new value
 * @len: length of new property value
 * @create: flag to create the property if it doesn't exist
 *
 * Convenience function to directly set a property given the path to the node.
 */
int fdt_find_and_setprop(void *fdt, const char *node, const char *prop,
			 const void *val, int len, int create)
{
	int nodeoff = fdt_path_offset(fdt, node);

	if (nodeoff < 0)
		return nodeoff;

	if ((!create) && (fdt_get_property(fdt, nodeoff, prop, NULL) == NULL))
		return 0; /* create flag not set; so exit quietly */

	return fdt_setprop(fdt, nodeoff, prop, val, len);
}
#endif //###

enum boot_stage_type {
	BT_STAGE_PRIMARY = 0x6,
	BT_STAGE_SECONDARY = 0x9,
	BT_STAGE_RECOVERY = 0xa,
	BT_STAGE_USB = 0x5,
};

int get_bootrom_bootstage(u32 *bstage)
{
	return -ENODEV;
}

int get_container_size(ulong addr, u16 *header_length)
{
	struct container_hdr *phdr;
	struct boot_img_t *img_entry;
	struct signature_block_hdr *sign_hdr;
	u8 i = 0;
	u32 max_offset = 0, img_end;

	phdr = (struct container_hdr *)addr;
	if (!valid_container_hdr(phdr)) {
		debug("Wrong container header\n");
		return -EFAULT;
	}

	max_offset = phdr->length_lsb + (phdr->length_msb << 8);
	if (header_length)
		*header_length = max_offset;

	img_entry = (struct boot_img_t *)(addr + sizeof(struct container_hdr));
	for (i = 0; i < phdr->num_images; i++) {
		img_end = img_entry->offset + img_entry->size;
		if (img_end > max_offset)
			max_offset = img_end;

		debug("img[%u], end = 0x%x\n", i, img_end);

		img_entry++;
	}

	if (phdr->sig_blk_offset != 0) {
		sign_hdr = (struct signature_block_hdr *)(addr + phdr->sig_blk_offset);
		u16 len = sign_hdr->length_lsb + (sign_hdr->length_msb << 8);

		if (phdr->sig_blk_offset + len > max_offset)
			max_offset = phdr->sig_blk_offset + len;

		debug("sigblk, end = 0x%x\n", phdr->sig_blk_offset + len);
	}

	return max_offset;
}

int confirm_yesno(void) {
	char input[8];
	if(!fgets(input, sizeof(input), stdin)) {
		return 0;
	}
	input[strcspn(input, "\n")] = '\0';
	for(char *p = input; *p; ++p) {
		*p = tolower((unsigned char) *p);
	}
	if((strcmp(input, "y") == 0) || (strcmp(input, "yes") == 0)) {
		return 1;
	}
	return 0;
}


/* ------------- MMC low-level access in Linux backend ---------------------- */

static struct mmc_ll_linux {
	int hwpart;		      /* Current hardware partition */
	bool rw;		      /* false: read-only, true: read/write */
	int fd[3];		      /* Filedescriptors user/boot1/boot2 */
} mmc_ll_linux;

/* Switch to a new hardware partition */
int fs_image_set_hwpart_mmc(struct flash_info *fi, int copy,
			    const struct storage_info *si)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;

	ll->hwpart = si->hwpart[copy];

	return 0;
}

/* Set the hardware partition to boot from in the future */
int fs_image_set_boot_hwpart_mmc(struct flash_info *fi, int boot_hwpart)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	u8 value;
	int err;

	if ((boot_hwpart < 0) || (boot_hwpart == fi->boot_hwpart))
		return 0;

	printf("\nSwitching %s to boot hwpart %d...", fi->devname, boot_hwpart);

	if (!boot_hwpart)
		boot_hwpart = 7;

	value = ext_csd[EXT_CSD_PART_CONFIG] & ~(7 << 3);
	value |= boot_hwpart << 3;
	err = write_extcsd_value(ll->fd[0], EXT_CSD_PART_CONFIG, value, 0);

	if (!err)
		fi->boot_hwpart = boot_hwpart;

	return err;
}

/* Read image at offset with given size */
int fs_image_read_mmc(struct flash_info *fi, uint offs, uint size,
		      uint lim, uint flags, u8 *buf)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	ssize_t count;
	int fd;

	debug("  -> mmc_read from offs 0x%x (block 0x%x) size 0x%x\n",
	      offs, offs / fi->temp_size, size);

	if (ll->hwpart < 0)
		return -EINVAL;		/* No partition selected */

	fd = ll->fd[ll->hwpart];
	if (lseek(fd, offs, SEEK_SET) == (off_t)-1)
		return -errno;		/* Seek error */

	count = read(fd, buf, size);
	if (count == -1)
		return -errno;		/* Read error */

	if (count != (ssize_t)size)
		return -EIO;		/* EOF */

	return 0;
}

/* Save some data (only full blocks) to eMMC */
int fs_image_write_mmc(struct flash_info *fi, uint offs, uint size,
		       uint lim, uint flags, u8 *buf)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	off_t seek;
	ssize_t count;
	int fd;

	if (!ll->rw)
		return -EROFS;		/* Read-only environment */

	if (ll->hwpart < 0)
		return -EINVAL;		/* No partition selected */

	fd = ll->fd[ll->hwpart];
	seek = lseek(fd, offs, SEEK_SET);
	if (seek == (off_t)-1)
		return -errno;		/* Seek error */

	count = write(fd, buf, size);
	if (count == -1)
		return -errno;		/* Write error */
	if (count != (ssize_t)size)
		return -ENOSPC;		/* No space left */

	return 0;
}

void fs_image_put_flash_mmc(struct flash_info *fi)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;

	ll->hwpart = -1;
	if (ll->fd[0] != -1)
		close(ll->fd[0]);
	if (ll->fd[1] != -1)
		close(ll->fd[1]);
	if (ll->fd[2] != -1)
		close(ll->fd[2]);
}

extern struct flash_ops flash_ops_mmc;
int fs_image_get_flash_mmc(struct flash_info *fi, int devnum, bool rw)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	int flags = rw ? O_RDWR : O_RDONLY;
	char devname[32];

	ll->rw = rw;
	ll->hwpart = 0;
	ll->fd[0] = -1;
	ll->fd[1] = -1;
	ll->fd[2] = -1;

	/* Open main device */
	snprintf(fi->devname, MAX_FI_DEVNAME, "mmcblk%d", devnum);
	snprintf(devname, 32, "/dev/%s", fi->devname);
	ll->fd[0] = open(devname, flags);
	if (ll->fd[0] == -1)
		goto err;

	if (read_extcsd(ll->fd[0])) {
		printf("Cannot read extcsd of %s\n", fi->devname);
		goto put;
	}

	fi->boot_hwpart = (ext_csd[EXT_CSD_PART_CONFIG] >> 3) & 7;
	if (fi->boot_hwpart > 2)
		fi->boot_hwpart = 0;
	fi->boot_part_size = ext_csd[EXT_CSD_BOOT_MULT] << 17;
	fi->temp_size = 0x200;

	/* Open boot1/2 partitions */
	snprintf(devname, 32, "/dev/%sboot0", fi->devname);
	ll->fd[1] = open(devname, flags);
	if (ll->fd[1] == -1)
		goto err;

	snprintf(devname, 32, "/dev/%sboot1", fi->devname);
	ll->fd[2] = open(devname, flags);
	if (ll->fd[2] == -1)
		goto err;

	fi->ops = &flash_ops_mmc;

	return 0;

err:
	printf("Cannot open %s: %s\n", devname, strerror(errno));
put:
	fs_image_put_flash_mmc(fi);

	return -1;
}

static int load_board_cfg_mmc(struct flash_info *fi, int copy)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
       	struct fs_header_v1_0 *fsh = (struct fs_header_v1_0 *)board_cfg;
	ssize_t count;
	size_t size;
	int fd;
	off_t offs;
	off_t end;

	if (fi->boot_hwpart) {
		/* Booting from boot1/2 hwpart: use appropriate copy */
		offs = 0;
		end = fi->boot_part_size;
		fd = ll->fd[!copy ? fi->boot_hwpart : 3 - fi->boot_hwpart];
	} else {
		/* Booting from User hwpart: search in first or second 4 MiB */
		if (!copy) {
			offs = 0x00008000; /* skip GPT in first 32KiB */
			end = 0x00400000;
		} else {
			offs = 0x00400000;
			end = 0x00800000;
		}
		fd = ll->fd[0];
	}

	if (lseek(fd, offs, SEEK_SET) == -1)
		return -errno;

	printf("  Searching BOARD-CFG... ");
	/* Search for BOARD-CFG */
	do {
		/* Read F&S header */
		count = read(fd, fsh, FSH_SIZE);
		if (count == (ssize_t)-1)
			return -errno;
		if (count != FSH_SIZE)
			return -EWOULDBLOCK;

		/* If BOARD-CFG found, load it and return success */
		if (fs_image_match(fsh, "BOARD-CFG", NULL)) {
			printf("found at offset 0x%lx\n"
			       "  Reading BOARD-CFG... ", offs);
			size = fs_image_get_size(fsh, false);
			count = read(fd, fsh + 1, size);
			if (count == (ssize_t)-1)
				return -errno;
			if (count != (ssize_t)size)
				return -EWOULDBLOCK;
			return 0;
		}
		offs += FSH_SIZE;
	} while (offs < end);

	return -ENOENT;
}

/* ------------- Functions that differ from U-Boot ------------------------- */

/* Return the address of the board configuration */
void *fs_image_get_cfg_addr(void)
{
	return board_cfg;
}

const char *fs_image_get_board_id(void)
{
	return board_id;
}

int fs_image_get_start_copy(void)
{
	int start_copy = 1;

	printf("TODO: Cannot determine SPL start copy, assuming Primary\n");
	printf("Booted from %s SPL, so starting with copy %d\n", \
	       start_copy ? "Primary" : "Secondary", start_copy);

	return start_copy;
}

int fs_image_get_start_copy_uboot(void)
{
	int start_copy = 1;

	printf("TODO: Cannot determine UBOOT start copy, assuming Primary\n");
	printf("Booted from %s UBOOT, so starting with copy %d\n", \
	       start_copy ? "Primary" : "Secondary", start_copy);

	return start_copy;
}

unsigned int fuse_read(int bank, int word, uint32_t *buf)
{
	const char *fname = "/sys/bus/nvmem/devices/fsb_s400_fuse0/nvmem";
	FILE *nvmem = fopen(fname, "rb");

	if (!nvmem)
		return -EIO;

	fseek(nvmem, (bank * 8 + word) * 4, SEEK_SET);
	fread(&buf, 4, 1, nvmem);
	fclose(nvmem);

	return 0;
}

/* Simply get the filename */
bool fs_image_get_image_params(int argc, char *const argv[],
			       struct fs_image_params *ip,
			       const char *def_fname)
{
	ip->addr = 0;
	ip->size = 0;
	ip->fname = NULL;

	if (argc > 1)
		return false;

	if (!def_fname)
		def_fname = "nboot.fs";
	if (argc > 0)
		ip->fname = argv[0];
	else
		ip->fname = def_fname;
	ip->addr = (ulong)image_ram;

	return true;
}

/* Load image from file ip->fname and fill in ip->size */
bool fs_image_provide_file(struct fs_image_params *ip)
{
	int fd;
	struct stat stat;
	ssize_t count;
	int err;

	printf("Loading %s... ", ip->fname);
	fd = open(ip->fname, O_RDONLY);
	if (fd == -1)
		goto err;

	if (fstat(fd, &stat) == -1)
		goto err;

	if (stat.st_size > sizeof(image_ram)) {
		err = EFBIG;
		goto out;
	}

	count = read(fd, (char *)ip->addr, stat.st_size);
	if (count == -1)
		goto err;

	if (count < stat.st_size) {
		err = ENOMSG;
		goto out;
	}
	if (close(fd) == -1) {
		fd = -1;
		goto err;
	}

	printf("done!\n");
	ip->size = stat.st_size;

	return true;

err:
	err = errno;
out:
	if (fd != -1)
		close(fd);

	printf("failed: %s\n", strerror(err));

	return false;
}

/* Write image with ip->size to file ip->fname */
bool fs_image_store_file(struct fs_image_params *ip)
{
	int fd = -1;
	ssize_t count;
	int err;

	printf("Writing %s... ", ip->fname);
	fd = open(ip->fname, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (fd == -1)
		goto err;

	count = write(fd, (char *)ip->addr, ip->size);
	if (count == -1)
		goto err;
	if (count < ip->size) {
		err = -EIO;
		goto out;
	}

	if (fsync(fd) == -1)
		goto err;

	if (close(fd) == -1) {
		fd = -1;
		goto err;
	}

	printf("done!\n");

	return true;

err:
	err = -errno;
out:
	if (fd != -1)
		close(fd);

	printf("failed: %s\n", strerror(err));

	return false;
}

#ifdef CONFIG_IMX_HAB

/* ### TODO: Use own authentication function */
int imx_hab_authenticate_image(uint32_t ddr_start, uint32_t image_size,
			       uint32_t ivt_offset)
{
	return -EINVAL;
}

/* ### TODO: Read secure boot fuse from fuse bank */
bool imx_hab_is_enabled(void)
{
	return false;
}

#endif /* CONFIG_IMX_HAB */


/* ------------- Linux command line handling ------------------------------- */

const char usage[] =
	"Usage:\n"
	"fsimage list <file>]\n"
	"    - List the content of the F&S image <file>\n"
	"fsimage load [-f] [uboot | nboot] <file>\n"
	"    - Verify the current NBoot or U-Boot and store in <file>\n"
	"fsimage save [-f] [-e <n>] [-b <n>] <file>\n"
	"    - Save the F&S image at the right place (NBoot, U-Boot)\n"
	"\n";


int do_fsimage(int argc, char *argv[])
{
	/* Drop argv[0] ("fsimage") */
	argc--;
	argv++;

	if (argc < 1)
		return CMD_RET_USAGE;

	if (!strcmp(argv[0], "arch"))
		return fs_image_do_arch(argc, argv);

	if (!strcmp(argv[0], "board-id"))
		return fs_image_do_boardid(argc, argv);

#if 0 //### TODO
	if (!strcmp(argv[0], "board-cfg"))
		return fs_image_do_boardcfg(argc, argv);
#endif

	if (!strcmp(argv[0], "boot"))
		return fs_image_do_boot(argc, argv);

	if (!strcmp(argv[0], "checksum"))
		return fs_image_do_checksum(argc, argv);

	if (!strcmp(argv[0], "list"))
		return fs_image_do_list(argc, argv);

	if (!strcmp(argv[0], "load"))
		return fs_image_do_load(argc, argv);

	if (!strcmp(argv[0], "save"))
		return fs_image_do_save(argc, argv);

	return CMD_RET_USAGE;
}

static bool read_bdinfo(const char *name, char *value, uint size)
{
	int fd;
	ssize_t count;

	fd = open(name, O_RDONLY);
	if (fd == -1) {
		printf("Cannot open %s: %s", name, strerror(errno));
		return false;
	}

	count = read(fd, value, size);
	if (count == -1) {
		printf("Cannot read %s: %s", name, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);

	if (count == 0) {
		fprintf(stderr, "%s has no content\n", name);
		return false;
	}

	value[count - 1] = '\0';

	return true;
}

static bool read_board_cfg(void)
{
	struct flash_info fi;
	char boot_dev_name[10];
	int err;

	if (!read_bdinfo(SYS_BOOT_DEV, boot_dev_name, 10))
		return false;
	fi.boot_dev_name = boot_dev_name;
	fi.boot_dev = fs_board_get_boot_dev_from_name(boot_dev_name);

	/* Prepare flash information from where to load */
	switch (fi.boot_dev) {
#if 0 //###def CONFIG_NAND_MXS
	case NAND_BOOT:
		err = fs_image_get_flash_nand(&fi, 0, rw);
		break;
#endif

#ifdef CONFIG_MMC
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
		err = fs_image_get_flash_mmc(&fi, fi.boot_dev - MMC1_BOOT, 0);
		break;
#endif
	default:
		printf("Cannot handle %s boot device\n", fi.boot_dev_name);
		return false;
	}

	if (err)
		return false;

	/* Try to find a valid BOARD-CFG copy */
	printf("Reading BOARD-CFG from %s\n", fi.devname);

	if (load_board_cfg_mmc(&fi, 0)) {
		int err = load_board_cfg_mmc(&fi, 1);
		if (err) {
			printf("failed: %s\n", strerror(-err));
			fs_image_put_flash_mmc(&fi);
			return false;
		}
	}

	printf("done!\n");
	fs_image_put_flash_mmc(&fi);

	/*
	 * Set the current board_id name and the compare_id that is used in
	 * fs_image_find_board_cfg().
	 */
	fs_image_set_board_id_from_cfg();

	return true;
}

/**
 * check_current_arch() - Verify F&S architecture
 *
 * Return: true if valid, false if verification failed
 *
 * Read the current architecture from /sys/bdinfo and compare with the
 * compiled-in architecture. Return 0 if matching, -1 otherwise.
 */
static bool check_current_arch(void)
{
	char current_arch[MAX_DESCR_LEN + 1];
	const char *compiled_arch = fs_image_get_arch();

	if (!read_bdinfo(SYS_ARCH, current_arch, MAX_DESCR_LEN + 1))
	    return false;

	if (strcmp(current_arch, compiled_arch)) {
		fprintf(stderr, "Architecture mismatch! fsimage compiled for %s"
			" but this is %s.\n", compiled_arch, current_arch);
		return false;
	}

	return true;
}


int main(int argc, char *argv[])
{
	int status;

	/* Make sure that we are running on the intended arch */
	if (!check_current_arch())
		return 1;

	if (!read_bdinfo(SYS_BOARD_ID, board_id, MAX_DESCR_LEN + 1))
		return 1;

	if (!read_board_cfg())
		return 1;

#if 0
	/* Load NBoot from flash, store in saved_nboot_buffer[] */
	if (load_saved_nboot() < 0)
		return 1;

	/* Extract BOARD-CFG from NBoot, store in saved_board_cfg_buffer[] */
	if (extract_board_config() < 0)
		return 1;
#endif

	status = do_fsimage(argc, argv);
	if (status == CMD_RET_USAGE) {
		fprintf(stderr, "%s\n", usage);
		return 1;
	}

	return status == CMD_RET_FAILURE;
}
