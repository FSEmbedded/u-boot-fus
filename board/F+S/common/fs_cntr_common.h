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

#define FUS_CNTR_ALIGNMENT 0x400

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
bool cntr_image_check_sha(const struct boot_img_t *img, const void *blob);
bool fs_cntr_is_valid_signature(const struct container_hdr *cntr_hdr);
bool fs_cntr_is_signed(const struct container_hdr *cntr);
bool fs_cntr_is_nxp_signed(const struct container_hdr *cntr);
bool fs_cntr_is_oem_signed(const struct container_hdr *cntr);
bool fs_cntr_is_ele_fw(const struct container_hdr *cntr);
bool fs_cntr_is_v2x_fw(const struct container_hdr *cntr);
ulong fs_cntr_get_boot_cntr_size(const struct container_hdr *cntr_hdr, uint *oem_offset);
bool fs_cntr_is_image_data(const struct boot_img_t *img);

#endif /* !__FS_CNTR_COMMON_H__ */

