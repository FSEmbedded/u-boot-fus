// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 F&S Elektronik Systeme GmbH
 *
 * Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
 *
 * F&S image processing
 *
 */

#ifndef __FS_CNTR_COMMON_H__
#define __FS_CNTR_COMMON_H__

#define IMX_CNTR_NAME "IMX_CONTAINER"
#define AHAB_CNTR_NAME "AHAB_CONTAINER"

/* Board specific init */
int fs_board_basic_init(void);
void fs_cntr_init(bool need_cfg);

#endif /* !__FS_CNTR_COMMON_H__ */
