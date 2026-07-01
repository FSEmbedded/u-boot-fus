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
