// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 *
 * Linux version of fs_image_nand.c.
 */
//####TODO
#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <linux/libfdt.h>
#include <errno.h>
#include <stdio.h>
#include "linux_helpers.h"		/* fit_get_size(), confirm_yesno() ... */

/* ### the following signatures are most probably not required in linux */
//###int mtdparts_init(void);
//###int mtd_id_parse(const char *id, const char **ret_id, u8 *dev_type, u8 *dev_num);
//###int find_dev_and_part(const char *id, struct mtd_device **dev,
//###                      u8 *part_num, struct part_info **part);

/* Include the original file */
#include "../../board/F+S/common/fs_image_nand.c"


#if 0
/* Known offsets for the BOARD-CFG in NAND flash of previous versions */
static const unsigned int fs_image_known_boardcfg_offs_nand[][2] = {
#ifdef CONFIG_IMX8MM
	{ 0x00180000, 0x002c0000 },
	{ 0x00180000, 0x00300000 },
#elif defined CONFIG_IMX8MN
	{ 0x00180000, 0x002c0000 },
	{ 0x000c0000, 0x00240000 },
	{ 0x00180000, 0x00300000 },
#endif
};
#endif
