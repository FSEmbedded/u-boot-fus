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

/* ------------- MMC low-level access in Linux backend ---------------------- */

static struct mmc_ll_linux {
	int hwpart;		      /* Current hardware partition */
	bool rw;		      /* false: read-only, true: read/write */
	int fd[3];		      /* Filedescriptors user/boot1/boot2 */
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
static int fs_image_write_mmc(struct flash_info *fi, uint offs, uint size,
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

static void fs_image_put_flash_mmc(struct flash_info *fi)
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

extern bool check_board_cfg(struct fs_header_v1_0 *fsh);
static int fs_image_read_board_cfg_mmc(struct flash_info *fi, int copy,
				       void *board_cfg)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;
       	struct fs_header_v1_0 *fsh = board_cfg;
	ssize_t count;
	size_t size;
	int fd;
	off_t offs;
	off_t end;
	u8 hwpart = fi->boot_hwpart;

	if (hwpart) {
		/* Booting from boot1/2 hwpart: use appropriate copy */
		offs = 0;
		end = fi->boot_part_size;
		if (copy)
			hwpart = 3 - hwpart;
	} else {
		/* Booting from User hwpart: search in first or second 4 MiB */
		if (!copy) {
			offs = 0x00008000; /* skip GPT in first 32KiB */
			end = 0x00400000;
		} else {
			offs = 0x00400000;
			end = 0x00800000;
		}
	}
	fd = ll->fd[hwpart];

	/* Search for BOARD-CFG */
	do {
		if (lseek(fd, offs, SEEK_SET) == -1)
			return -errno;

		/* Read F&S header */
		count = read(fd, fsh, FSH_SIZE);
		if (count == (ssize_t)-1)
			return -errno;
		if (count != FSH_SIZE)
			return -EWOULDBLOCK;

		/* If there is a matching BOARD-CFG, load and verify it */
		if (fs_image_match_board_id(fsh)) {
			size = fs_image_get_size(fsh, false);
			count = read(fd, fsh + 1, size);
			if (count == (ssize_t)-1)
				return -errno;
			if (count != (ssize_t)size)
				return -EWOULDBLOCK;
			if (check_board_cfg(fsh)) {
				printf("  Found BOARD-CFG in hwpart %d at"
				       " offset 0x%lx\n", hwpart, offs);
				return 0;
			}
			printf("  Ignoring invalid BOARD-CFG in hwpart %d at"
			       " offset 0x%lx\n", hwpart, offs);
		}
		offs += FSH_SIZE;
	} while (offs < end);

	return -ENOENT;
}

/* Include the original file */
#include "../../board/F+S/common/fs_image_mmc.c"

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

