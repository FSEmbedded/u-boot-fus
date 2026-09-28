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

	if (ll->hwpart != si->hwpart[copy]) {
		ll->hwpart = si->hwpart[copy];
		fs_image_drop_temp(fi);
	}

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

/* Sync data with flash */
static int fs_image_sync_mmc(struct flash_info *fi)
{
	struct mmc_ll_linux *ll = &mmc_ll_linux;

	/* Writing asynchronously may return delayed errors */
	if (fsync(ll->fd[ll->hwpart]) == -1)
		return -errno;

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

