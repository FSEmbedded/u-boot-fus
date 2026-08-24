// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 *
 * Linux version of fs_image_common.c.
 */
#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <errno.h>			/* IS_ERR_VALUE() */
#include <stdio.h>
#include <fcntl.h>			/* open() */
#include <unistd.h>			/* read(), close() */
#include <sys/stat.h>			/* fstat() */
#include "linux_helpers.h"		/* fit_get_size(), confirm_yesno() ... */
#include "../../board/F+S/common/fs_image_common.h"

#define MAX_IMAGE_SIZE (4 * 1024 * 1024)
#define MAX_BOARD_CFG_SIZE 0x2000

static u8 image_ram[2 * MAX_IMAGE_SIZE];
static u8 board_cfg[MAX_BOARD_CFG_SIZE];


/* Return the address of the board configuration */
void *fs_image_get_cfg_addr(void)
{
	return board_cfg;
}

/* Simply get the filename */
static bool fs_image_get_image_params(int argc, char *const argv[],
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
static bool fs_image_provide_file(struct fs_image_params *ip)
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
static bool fs_image_store_file(struct fs_image_params *ip)
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

/* Include the original file */
#include "../../board/F+S/common/fs_image_nonspl.c"

bool read_board_cfg(const char *boot_dev_name)
{
	struct flash_info fi;
	int err;
	struct storage_info si;

	fi.boot_dev_name = boot_dev_name;
	fi.boot_dev = fs_image_get_boot_dev_from_name(boot_dev_name);

	/* Prepare flash information from where to load */
	switch (fi.boot_dev) {
#if 0 //###def CONFIG_NAND_MXS
	case NAND_BOOT:
		err = fs_image_get_flash_nand(&fi, 0, true);
		break;
#endif

#ifdef CONFIG_MMC
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
		err = fs_image_get_flash_mmc(&fi, fi.boot_dev - MMC1_BOOT, true);
		if (fi.boot_hwpart) {
			si.start[0] = 0x00000000;
			si.start[1] = 0x00000000;
			si.size = fi.boot_part_size;
			si.hwpart[0] = fi.boot_hwpart;
			si.hwpart[1] = 3 - fi.boot_hwpart;
		} else {
			si.start[0] = 0x00008000; /* skip GPT in first 32KiB */
			si.start[1] = 0x00400000;
			si.size = 0x003f8000;
			si.hwpart[0] = 0;
			si.hwpart[1] = 0;
		}
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

	err = fi.ops->read_board_cfg(&fi, &si, board_cfg);
	fi.ops->put_flash(&fi);
	if (err) {
		printf("Reading BOARD-CFG failed: %s\n", strerror(-err));
		return false;
	}

	return true;
}
