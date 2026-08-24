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
#include <command.h>
#include <ctype.h>			/* tolower() */
#include <linux/compiler_attributes.h>
#include "linux_helpers.h"
#include "../../board/F+S/common/fs_image_common.h"
#include "../../include/imx_container.h"

#define SYS_BDINFO "/sys/bdinfo/"
#define SYS_ARCH SYS_BDINFO "arch"
#define SYS_BOARD_ID SYS_BDINFO "board-id"
#define SYS_BOOT_DEV SYS_BDINFO "boot_dev"
#define SYS_BOOT_COPY SYS_BDINFO "boot_copy"
#define SYS_VERSION  SYS_BDINFO "nboot_version"

static char sys_board_id[MAX_DESCR_LEN + 1];
static char sys_boot_copy[10];
static unsigned int boot_copy;
static char nboot_version[20];
static char boot_dev_name[10];

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


/* ------------- Functions needed to avoid large libraries ----------------- */

/* From boot/fdt_support.c */
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

	return fdt32_to_cpu(val[cell]);
}

#define FIT_DATA_PROP		"data"
#define FIT_DATA_SIZE_PROP	"data-size"
#define FIT_DATA_OFFSET_PROP	"data-offset"
#define FIT_DATA_POSITION_PROP	"data-position"

/**
 * fit_image_get_data_size() - Get 'data-size' property from image node
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_size: holds the data-size property
 *
 * Context:
 * This code is taken from boot/image-fit.c.
 *
 * Return:
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

/**
 * fit_image_get_data_offset() - Get 'data-offset' property from image node
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_offset: holds the data-offset property
 *
 * Context:
 * This code is taken from boot/image-fit.c.
 *
 * Return:
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
 * fit_image_get_data_position() - Get 'data-position' property from image node
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_position: holds the data-position property
 *
 * Context:
 * This code is taken from boot/image-fit.c.
 *
 * Return:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
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
 * fit_image_get_data() - Get 'data' property and its size from image node
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data: holds the data property's data address
 * @size: holds the data property's data size
 *
 * Context:
 * This code is taken from boot/image-fit.c with debug code removed.
 *
 * Return:
 *     0, on success
 *     -1, on failure
 *
 * fit_image_get_data() finds data property in a given component image node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 */
int fit_image_get_data(const void *fit, int noffset,
		       const void **data, size_t *size)
{
	int len;

	*data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
	if (*data == NULL) {
		*size = 0;
		return -1;
	}

	*size = len;
	return 0;
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	return strtoul(cp, endp, base);
}

long simple_strtol(const char *cp, char **endp, unsigned int base)
{
	return strtol(cp, endp, base);
}

int confirm_yesno(void)
{
	char input[8];
	char *p;

	if (!fgets(input, sizeof(input), stdin))
		return 0;

	for (p = input; *p; ++p) {
		if (*p == '\n') {
			*p = '\0';
			break;
		}
		*p = tolower((unsigned char) *p);
	}

	return (!strcmp(input, "y") || !strcmp(input, "yes"));
}


/* ------------- Container handling ---------------------------------------- */

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

/* Check that the version of this BOARD-CFG is the same as the one booted from */
bool check_board_cfg(struct fs_header_v1_0 *fsh)
{
#if 0 //### With the current list of former offsets do not check version anymore
	const void *version;
	int offs;
	void *fdt = fs_image_find_cfg_fdt(fsh);

	if (!fdt)
		return false;

	offs = fs_image_get_nboot_info_offs(fdt);
	if (offs < 0) {
		/* Very old BOARD-CFGs didn't have the nboot-info subnode */
		offs = fs_image_get_board_cfg_offs(fdt);
		if (offs < 0)
			return false;
	}

	version = fdt_getprop(fdt, offs, "version", NULL);
	if (!version)
		return false;

	if (strcmp(version, nboot_version))
		return false;

#endif //###
	return true;
}

/* ------------- Functions that differ from U-Boot ------------------------- */

#if 0 //###
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
#endif //###

#ifdef CONFIG_IMX_HAB

/* ### TODO: Use own authentication function */
int imx_hab_authenticate_image(uint32_t ddr_start, uint32_t image_size,
			       uint32_t ivt_offset)
{
	return -EINVAL;
}
#endif /* CONFIG_IMX_HAB */

/* ### TODO: Read secure boot fuse from fuse bank */
bool fs_board_is_closed(void)
{
	return false;
}

unsigned int fs_image_get_boot_copy(void)
{
	return boot_copy;
}

/* ------------- Linux command line handling ------------------------------- */

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

#ifdef CONFIG_CMD_FDT
	if (!strcmp(argv[0], "board-cfg"))
		return fs_image_do_boardcfg(argc, argv);
#endif

	if (!strcmp(argv[0], "boot"))
		return fs_image_do_boot(argc, argv);

	if (!strcmp(argv[0], "list"))
		return fs_image_do_list(argc, argv);

	if (!strcmp(argv[0], "load"))
		return fs_image_do_load(argc, argv);

	if (!strcmp(argv[0], "save"))
		return fs_image_do_save(argc, argv);

	return CMD_RET_USAGE;
}

static bool read_sys(const char *name, char *value, uint size, bool needed)
{
	int fd;
	ssize_t count;

	fd = open(name, O_RDONLY);
	if (fd == -1) {
		if (needed)
			printf("Cannot open %s: %s\n", name, strerror(errno));
		return false;
	}

	count = read(fd, value, size);
	if (count == -1) {
		if (needed)
			printf("Cannot read %s: %s\n", name, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);

	if (count == 0) {
		if (needed)
			fprintf(stderr, "%s has no content\n", name);
		return false;
	}

	value[count - 1] = '\0';

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

	if (!read_sys(SYS_ARCH, current_arch, MAX_DESCR_LEN + 1, true))
	    return false;

	if (strcmp(current_arch, compiled_arch)) {
		fprintf(stderr, "Architecture mismatch! fsimage compiled for %s"
			" but this is %s.\n", compiled_arch, current_arch);
		return false;
	}

	return true;
}

extern bool read_board_cfg(const char *boot_dev_name);
int main(int argc, char *argv[])
{
	int status;

	/* Make sure that we are running on the intended arch */
	if (!check_current_arch())
		return 1;

	/* Read board-id from bdinfo */
	if (!read_sys(SYS_BOARD_ID, sys_board_id, sizeof(sys_board_id), true))
		return 1;
	fs_image_set_compare_id(sys_board_id);

	/* If available, read boot_copy from bdinfo*/
	if (!read_sys(SYS_BOOT_COPY, sys_boot_copy,
			 sizeof(sys_boot_copy), false)) {
		printf("Warning: Cannot read %s, assuming boot from copy 0\n",
		       SYS_BOOT_COPY);
		boot_copy = 0;
	} else {
		boot_copy = strtoul(sys_boot_copy, NULL, 0);
	}

	/* Read NBoot version from bdinfo */
	if (!read_sys(SYS_VERSION, nboot_version, sizeof(nboot_version), true))
		return 1;

	/* Read boot device */
	if (!read_sys(SYS_BOOT_DEV, boot_dev_name, sizeof(boot_dev_name), true))
		return 1;

	/* Read BOARD-CFG from flash */
	if (!read_board_cfg(boot_dev_name))
		return 1;

#if 0
	/* Load NBoot from flash, store in saved_nboot_buffer[] */
	if (load_saved_nboot() < 0)
		return 1;

	/* Extract BOARD-CFG from NBoot, store in saved_board_cfg_buffer[] */
	if (extract_board_config() < 0)
		return 1;
#endif

	/* fsimage_usage[] is defined in fs_image_nonspl.c */
	status = do_fsimage(argc, argv);
	if (status == CMD_RET_USAGE) {
		fprintf(stderr, "%s\n", fsimage_usage);
		return 1;
	}

	return status == CMD_RET_FAILURE;
}
