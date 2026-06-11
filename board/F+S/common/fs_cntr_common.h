// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 F&S Elektronik Systeme GmbH
 *
 * Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
 *
 * F&S image processing
 *
 */
#if __UBOOT__
#include <imx_container.h>
#else
#include "../../../include/linux/compiler_attributes.h"
#include "../../../include/imx_container.h"
#endif

#ifndef __FS_CNTR_COMMON_H__
#define __FS_CNTR_COMMON_H__

#define IMX_CNTR_NAME "IMX_CONTAINER"
#define AHAB_CNTR_NAME "AHAB_CONTAINER"

#if defined(CONFIG_SPL_BUILD)
struct fsh_load_info{
	struct fs_header_v1_0 *fsh;
	struct spl_load_info *load_info;
	uint offset;
};

int fs_cntr_init(bool need_cfg);
int fs_cntr_load_board_id(void);
#endif /* CONFIG_SPL_BUILD */

/**
 * Check HASH Value of a Image entry in Image Array
 * return false, if sha is invalid
 */
bool cntr_image_check_sha(struct boot_img_t *img, void *blob);
bool fs_cntr_is_valid_signature(struct container_hdr *cntr_hdr);
bool fs_cntr_is_signed(struct container_hdr *cntr);


#endif /* !__FS_CNTR_COMMON_H__ */
