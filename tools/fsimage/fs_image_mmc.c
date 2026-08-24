// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 *
 * Linux version of fs_image_mmc.c.
 */
#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <linux/libfdt.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>			/* open() */
#include <unistd.h>			/* read(), close() */
#include <sys/stat.h>			/* fstat() */
#include <linux/mmc/ioctl.h>		/* mmc_ioc_cmd_set_data(), ... */
#include <sys/ioctl.h>			/* ioctl() */
#include "linux_helpers.h"		/* fit_get_size(), confirm_yesno() ... */
#include "../../board/F+S/common/fs_image_common.h"
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
#include "imx_container.h"		/* struct container_hdr, ... */
#endif

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

static u8 ext_csd[512];

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

static bool read_sysfs_uint(int sysfs_fd, uint *val)
{
	char content[16];
	char *endp;
	ssize_t count;

	count = pread(sysfs_fd, content, sizeof(content), 0);
	if ((count == -1) || (count == 0))
		return false;

	*val = simple_strtoul(content, &endp, 0);
	if (endp == content)
		return false;

	return true;
}

static bool read_sysfs_bool(int sysfs_fd, bool *flag)
{
	uint val;

	if (!read_sysfs_uint(sysfs_fd, &val))
		return false;

	*flag = (val != 0);

	return true;
}


/* ------------- MMC low-level access in Linux backend ---------------------- */

static struct mmc_ll_linux {
	int hwpart;		      /* Current hardware partition */
	bool ro;		      /* false: read-only, true: read/write */
	int fd[3];		      /* Filedescriptors user/boot1/boot2 */
	int force_ro_fd[3];	      /* Filedescriptors for force_ro file */
} mmc_ll_linux;

/* Switch to a new hardware partition */
static int fs_image_set_hwpart_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;

	ll->hwpart = si->hwpart[copy];

	return 0;
}

/* Set the hardware partition to boot from in the future */
static int fs_image_set_boot_hwpart_mmc(struct flash_info *fi, int boot_hwpart)
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
static int fs_image_read_mmc(struct flash_info *fi, uint offs, uint size,
			     uint lim, uint flags, void *buf)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	ssize_t count;
	int fd;

	debug("  -> mmc_read from offs 0x%x (block 0x%x) size 0x%x\n",
	      offs, offs / fi->temp_size, size);

	if (ll->hwpart < 0)
		return -EINVAL;		/* No partition selected */

	fd = ll->fd[ll->hwpart];
	count = pread(fd, buf, size, offs);
	if (count == -1)
		return -errno;		/* Seek or read error */

	if (count != (ssize_t)size)
		return -EIO;		/* EOF */

	return 0;
}

/* Save some data (only full blocks) to eMMC */
static int fs_image_write_mmc(struct flash_info *fi, uint offs, uint size,
			      uint lim, uint flags, void *buf)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	ssize_t count;
	int fd;

	if (ll->ro)
		return -EROFS;		/* Read-only environment */

	if (ll->hwpart < 0)
		return -EINVAL;		/* No partition selected */

	fd = ll->fd[ll->hwpart];
	count = pwrite(fd, buf, size, offs);
	if (count == -1)
		return -errno;		/* Seek or write error */
	if (count != (ssize_t)size)
		return -ENOSPC;		/* No space left */

	return 0;
}

static void fs_image_put_flash_mmc(struct flash_info *fi)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	int hwpart;

	ll->hwpart = -1;
	for (hwpart = 0; hwpart < 3; hwpart++) {
		/* Close the hwpartition file */
		if (ll->fd[hwpart] != -1) {
			fsync(ll->fd[hwpart]);
			close(ll->fd[hwpart]);
			ll->fd[hwpart] = -1;
		}

		/* If force_ro file is still open here, set it back to 1 */
		if (ll->force_ro_fd[hwpart] != -1) {
			pwrite(ll->force_ro_fd[hwpart], "1\n", 3, 0);
			close(ll->force_ro_fd[hwpart]);
			ll->force_ro_fd[hwpart] = -1;
		}
	}
}

#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
/* Read boot container from flash and return size of original BOOT-INFO image */
static int fs_image_get_boot_info_size_mmc(struct flash_info *fi, uint start,
					   uint lim)
{
	uint offs = start;
	uint size;
	uint img_entry_offs;
	struct container_hdr hdr;
	struct boot_img_t img_entry;
	int err;

	/* Read ELE boot container header */
	err = fi->ops->read(fi, offs, sizeof(hdr), lim, 0, &hdr);
	if (err)
		return err;
	if (!valid_container_hdr(&hdr))
		return -EINVAL;

	size = (hdr.length_msb << 8) + hdr.length_lsb;
	debug("  - ELE boot container: offs=0x%x size=0x%x (%d images)\n",
	       offs, size, hdr.num_images);
	offs += ALIGN(size, CONTAINER_HDR_ALIGNMENT);

	/* Read OEM boot container header */
	err = fi->ops->read(fi, offs, sizeof(hdr), lim, 0, &hdr);
	if (err)
		return err;
	if (!valid_container_hdr(&hdr))
		return -EINVAL;

	size = (hdr.length_msb << 8) + hdr.length_lsb;
	debug("  - OEM boot container: offs=0x%x size=0x%x (%d images)\n",
	       offs, size, hdr.num_images);
	if (!hdr.num_images)
		return -EINVAL;

	/*
	 * Theoretically, the images of a container could be in random order,
	 * then we would need to check all image entries of the image array.
	 * Also the signature block could be behind the images, then we would
	 * need to check for the end of the signature block, too (see
	 * get_container_size() in image-container.c). But both is not the
	 * case in F&S images, here the signature block is in front of the
	 * image array and the last image in the image array has the highest
	 * offset. Therefore it is sufficient to simply look for the end of
	 * this last entry.
	 */
	img_entry_offs = offs + sizeof(hdr);
	size = sizeof(img_entry);
	img_entry_offs += size * (hdr.num_images - 1);
	debug("  - image entry #%d: offs=0x%x size=0x%x\n",
	       (hdr.num_images - 1), img_entry_offs, size);
	err = fi->ops->read(fi, img_entry_offs, size, lim, 0, &img_entry);
	if (err)
		return err;
	debug("  - image: offset=0x%x size=0x%x\n", img_entry.offset, img_entry.size);
	offs += img_entry.offset + img_entry.size;
	offs = ALIGN(offs, CONTAINER_HDR_ALIGNMENT);
	offs += CONTAINER_HDR_ALIGNMENT - 2 * FSH_SIZE;

	return offs - start;
}

/*
 * Find toplevel image of given type in flash, starting at offs; return
 * F&S header in fsh and offset as return value
 */
static int fs_image_find_toplevel_offset_mmc(struct flash_info *fi, uint offs,
					     uint lim, const char *type,
					     struct fs_header_v1_0 *fsh)
{
	int err;

	while (offs < lim) {
		err = fi->ops->read(fi, offs, FSH_SIZE, lim, 0, fsh);
		if (err)
			return err;

		if (!fs_image_is_fs_image(fsh))
			break;

		debug("  - %s (%s): offs=0x%x\n", fsh->type, fsh->param.descr, offs);

		if (fs_image_match(fsh, type, NULL))
			return (int)offs;

		offs += fs_image_get_size(fsh, true);
	}

	return -ENOENT;
}

/* Values determined when BOARD-CFG is loaded from flash */
struct nboot_info_fixup {
	uint nboot_start;
	uint nboot_size;
	uint uboot_start;
	uint uboot_size;
} nboot_info_fixup;

static int fs_image_try_board_cfg_mmc(struct flash_info *fi, int copy,
				      const struct storage_info *si,
				      void *board_cfg)
{
	uint offs;
	uint size;
	uint num_images;
	uint start = si->start[copy];
	uint lim = start + si->size;
       	struct fs_header_v1_0 one_fsh;
       	const struct fs_header_v1_0 *fsh;
	int err;

	printf("  Trying copy %d (hwpart %d)... ", copy, si->hwpart[copy]);
	debug("\n");

	err = fi->ops->set_hwpart(fi, copy, si);
	if (err)
		return err;

	/* Skip the BOOT-INFO part of NBoot in flash */
	err = fs_image_get_boot_info_size_mmc(fi, start, lim);
	if (err < 0)
		return err;
	offs = start + err; 		/* points to BOARD-ID */
	err = fi->ops->read(fi, offs, FSH_SIZE, lim, 0, &one_fsh);
	if (err)
		return err;
	if (!fs_image_match(&one_fsh, "BOARD-ID", fs_image_get_board_id())) {
		printf("BOARD-ID mismatch: found %s, should be %s\n",
		       one_fsh.param.descr, fs_image_get_board_id());
		return -EINVAL;
	}
	debug("  - BOARD-ID (%s): offs=0x%x\n", one_fsh.param.descr, offs);

	offs += FSH_SIZE;		/* points behind BOARD-ID */

	/* Find offset to BOARD-INFO, this is nboot_info.start */
	err = fs_image_find_toplevel_offset_mmc(fi, offs, lim,
						"BOARD-INFO", &one_fsh);
	if (err < 0)
		return err;
	nboot_info_fixup.nboot_start = (uint)err;

	/* Keep offset to BOARD-INFO's INDEX in offs for later */
	offs = (uint)err + FSH_SIZE + fs_image_get_extra_size(&one_fsh);

	/* Find nboot_info.size */
	err += fs_image_get_size(&one_fsh, true); /* points behind BOARD-INFO */
	err = fs_image_find_toplevel_offset_mmc(fi, err, lim,
						"DRAM-INFO", &one_fsh);
	if (err < 0)
		return err;
	err += fs_image_get_size(&one_fsh, true); /* points behind DRAM-INFO */
	nboot_info_fixup.nboot_size = (uint)err - nboot_info_fixup.nboot_start;
	debug("  - nboot_start=0x%x nboot_size=0x%x\n",
	      nboot_info_fixup.nboot_start, nboot_info_fixup.nboot_size);

	/* Load F&S header of INDEX to determine size */
	err = fi->ops->read(fi, offs, FSH_SIZE, lim, 0, &one_fsh);
	if (err)
		return err;
	if (!fs_image_match(&one_fsh, "INDEX", NULL))
		return -ENOENT;

	/* Load INDEX itself into board_cfg[] */
	size = fs_image_get_size(&one_fsh, false);
	debug("  - INDEX of BOARD_INFO: offs=0x%x, size=0x%x\n", offs, size);
	offs += FSH_SIZE;
	err = fi->ops->read(fi, offs, size, lim, 0, board_cfg);
	if (err)
		return err;

	/* Find the correct BOARD-CFG */
	num_images = fs_image_index_get_n(&one_fsh);
	fsh = board_cfg;
	offs += size;
	while (1) {
		if (!num_images || !fs_image_is_fs_image(fsh))
			return -ENOENT;
		if (fs_image_match_board_id(fsh))
			break;
		offs += fs_image_get_size(fsh++, false);
		num_images--;
	}

	/* Load BOARD-CFG (incl. header) to board_cfg[] */
	memmove(board_cfg, fsh, FSH_SIZE);
	err = fi->ops->read(fi, offs, fs_image_get_size(fsh, false), lim, 0,
			    board_cfg + FSH_SIZE);
	if (err)
		return err;

	printf("BOARD-CFG found at offset 0x%x\n", offs);

	return 0;
}

static bool fs_image_try_uboot_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si)
{
	uint offs = nboot_info_fixup.nboot_start + nboot_info_fixup.nboot_size;
	uint lim = si->start[copy] + si->size;
       	struct fs_header_v1_0 one_fsh;
	int err;

	printf("  Trying copy %d (hwpart %d)... ", copy, si->hwpart[copy]);
	debug("\n");
	err = fi->ops->set_hwpart(fi, copy, si);
	if (err)
		return err;
	err = fs_image_find_toplevel_offset_mmc(fi, offs, lim,
						"U-BOOT-INFO", &one_fsh);
	if (err < 0)
		return err;

	nboot_info_fixup.uboot_start = (uint)err;
	nboot_info_fixup.uboot_size = fs_image_get_size(&one_fsh, true);
	debug("  - uboot_start=0x%x uboot_size=0x%x\n",
	      nboot_info_fixup.uboot_start, nboot_info_fixup.uboot_size);

	printf("U-Boot found at offset 0x%x\n", err);

	return 0;
}

static int fs_image_read_board_cfg_mmc(struct flash_info *fi,
				       const struct storage_info *si,
				       void *board_cfg)
{
	int err;
	int copy, start_copy;

	nboot_info_fixup.nboot_start = 0;
	nboot_info_fixup.nboot_size = 0;
	nboot_info_fixup.uboot_start = 0;
	nboot_info_fixup.uboot_size = 0;

	/* Find BOARD_CFG and get nboot start and size information */
	start_copy = fs_image_get_start_copy(false, false);
	copy = start_copy;
	do {
		err = fs_image_try_board_cfg_mmc(fi, copy, si, board_cfg);
		if (err)
			printf("Failed (%d)\n", err);
		copy = 1 - copy;
	} while (err && (copy != start_copy));

	if (err)
		return err;

	/* Get uboot start and size information */
	printf("Reading U-Boot parameters from %s\n", fi->devname);
	start_copy = fs_image_get_start_copy(true, false);
	copy = start_copy;
	do {
		err = fs_image_try_uboot_mmc(fi, copy, si);
		if (err)
			printf("Failed (%d)\n", err);
		copy = 1 - copy;
	} while (err && (copy != start_copy));

	return 0;			/* Missing U-Boot is acceptable */
}

#else

/* Known eMMC offsets for the BOARD-CFG of previous versions, new to old */
static const uint known_boardcfg_offs_mmc[][2] = {
#ifdef CONFIG_IMX8MM
	{ 0x00088000, 0x00448000 },
	{ 0x00040000, 0x00140000 },
#elif defined CONFIG_IMX8MN
	{ 0x00048000, 0x00448000 },
	{ 0x00048000, 0x00740000 },
	{ 0x00040000, 0x00740000 },
	{ 0x00048000, 0x00140000 },
#elif defined CONFIG_IMX8MP
	{ 0x00048000, 0x00448000 },
	{ 0x00048000, 0x00740000 },
	{ 0x00040000, 0x00740000 },
#elif defined CONFIG_IMX8X
	{ 0x00080000, 0x00740000 },
	{ 0x00040000, 0x00740000 },
#endif
};

static int fs_image_try_board_cfg(struct flash_info *fi, uint offs, uint lim,
				   void *board_cfg)
{
	int err;
	uint size;

	/* Read F&S header */
	err = fi->ops->read(fi, offs, FSH_SIZE, lim, 0, board_cfg);
	if (err)
		return err;

	/* Is there a matching BOARD-CFG? */
	if (!fs_image_match_board_id(board_cfg))
		return 1;

	/* Load full BOARD-CFG */
	size = fs_image_get_size(board_cfg, true);
	err = fi->ops->read(fi, offs, size, lim, 0, board_cfg);
	if (err)
		return err;

	printf("BOARD-CFG found at offset 0x%x\n", offs);

	return 0;
}

static int fs_image_try_known_offsets_mmc(struct flash_info *fi, int copy,
					  const struct storage_info *si,
					  void *board_cfg)
{
	uint offs;
	uint lim = si->start[copy] + si->size;
	int slot = si->hwpart[copy] ? 0 : copy;
	int err;
	int i;

	printf("  Trying known offsets for copy %d (hwpart %d)... ",
	       copy, si->hwpart[copy]);
	debug("\n");

	err = fi->ops->set_hwpart(fi, copy, si);
	if (err)
		return err;

	for (i = 0; i < ARRAY_SIZE(known_boardcfg_offs_mmc); i++) {
		offs = known_boardcfg_offs_mmc[i][slot];
		err = fs_image_try_board_cfg(fi, offs, lim, board_cfg);
		if (err <= 0)
			return err;	/* Error or found */
	}

	return -ENOENT;
}

static int fs_image_search_board_cfg_mmc(struct flash_info *fi, int copy,
					 const struct storage_info *si,
					 void *board_cfg)
{
	uint offs = si->start[copy];
	uint lim = offs + si->size;
	int err;

	printf("  Searching BOARD_CFG in copy %d (hwpart %d)\n", copy,
	       si->hwpart[copy]);

	err = fi->ops->set_hwpart(fi, copy, si);
	if (err)
		return err;

	do {
		err = fs_image_try_board_cfg(fi, offs, lim, board_cfg);
		if (err <= 0)
			return err;	/* Error or found */
		offs += FSH_SIZE;
	} while (offs < lim);

	return -ENOENT;
}

static int fs_image_read_board_cfg_mmc(struct flash_info *fi,
				       const struct storage_info *si,
				       void *board_cfg)
{
	int err;
	int copy, start_copy;

	/* First look for BOARD-CFG at known offsets */
	start_copy = fs_image_get_start_copy(false, false);
	copy = start_copy;
	do {
		err = fs_image_try_known_offsets_mmc(fi, copy, si, board_cfg);
		if (err < 0)
			printf("Failed (%d)\n", err);
		copy = 1 - copy;
	} while (err & (copy != start_copy));

	if (err) {
		/* No BOARD-CFG found at known offsets, search for it */
		printf("  Warning, no BOARD-CFG found at known offsets,\n");
		do {
			err = fs_image_search_board_cfg_mmc(fi, copy, si,
							    board_cfg);
			if (err)
				printf("Failed (%d)\n", err);
			copy = 1 - copy;
		} while (err & (copy != start_copy));

		if (err)
			return err;
	}

	return 0;
}
#endif /* CONFIG_IS_ENABLED(FS_CNTR_COMMON) */

/* Include the original file */
#include "../../board/F+S/common/fs_image_mmc.c"

int fs_image_get_flash_mmc(struct flash_info *fi, int devnum, bool ro)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
	const char *mmc_hwpart_name[3] = {"", "boot0", "boot1"};
	int flags = ro ? O_RDONLY : O_RDWR;
	char devname[64];
	int hwpart;
	bool force_ro;
	int fd;
	ssize_t count;
	const char *reason;

	ll->ro = ro;
	ll->hwpart = 0;
	ll->fd[0] = -1;
	ll->fd[1] = -1;
	ll->fd[2] = -1;
	ll->force_ro_fd[0] = -1;
	ll->force_ro_fd[1] = -1;
	ll->force_ro_fd[2] = -1;

	snprintf(fi->devname, MAX_FI_DEVNAME, "mmcblk%d", devnum);

	/* In write mode, clear the force_ro flags and save previous state */
	for (hwpart = 0; hwpart < 3; hwpart++) {
		/*
		 * In write mode, if force_ro is set, clear it and keep file
		 * open to enable it later again in fs_image_put_flash_mmc().
		 * If force_ro is already cleared, no action is required and
		 * the file can be closed right here.
		 */
		if (!ro) {
			snprintf(devname, sizeof(devname),
				 "/sys/block/%s%s/force_ro",
				 fi->devname, mmc_hwpart_name[hwpart]);
			fd = open(devname, O_RDWR);
			if (fd == -1)
				goto open_err;
			if (!read_sysfs_bool(fd, &force_ro)) {
				printf("Cannot read %s\n", devname);
				goto err;
			}
			if (force_ro) {
				ll->force_ro_fd[hwpart] = fd;
				count = pwrite(fd, "0\n", 3, 0);
				if (count == -1)
					goto err;
				if (count < 3) {
					reason = "clear";
					goto show_err;
				}
			} else {
				close(fd);
			}
		}

		/* Open the corresponding hwpart in ro or rw mode */
		snprintf(devname, sizeof(devname), "/dev/%s%s",
			 fi->devname, mmc_hwpart_name[hwpart]);
		ll->fd[hwpart] = open(devname, flags);
		if (ll->fd[hwpart] == -1)
			goto open_err;
	}

	/* Read Extenden CSD register to get some eMMC parameters */
	if (read_extcsd(ll->fd[0])) {
		printf("Cannot read extcsd of %s:\n", fi->devname);
		goto err;
	}

	fi->boot_hwpart = (ext_csd[EXT_CSD_PART_CONFIG] >> 3) & 7;
	if (fi->boot_hwpart > 2)
		fi->boot_hwpart = 0;
	fi->boot_part_size = ext_csd[EXT_CSD_BOOT_MULT] << 17;
	fi->temp_size = 0x200;
	fi->ops = &flash_ops_mmc;

	return 0;

open_err:
	reason = "open";
show_err:
	printf("Cannot %s %s: %s\n", reason, devname, strerror(errno));
err:
	fs_image_put_flash_mmc(fi);

	return -1;
}

