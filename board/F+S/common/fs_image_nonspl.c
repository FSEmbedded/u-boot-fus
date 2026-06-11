// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright 2025 F&S Elektronik Systeme GmbH
 *
 * Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
 *
 * F&S image processing
 */

#ifdef __UBOOT__
#include <common.h>
#include <fdt_support.h>		/* fdt_getprop_u32_default_node() */
//####include <mmc.h>
//####include <nand.h>
//####include <sdp.h>
#include <asm/global_data.h>		/* DECLARE_GLOBAL_DATA_PTR */
//####include <asm/sections.h>
//####include <u-boot/crc.h>			/* crc32() */

//#####include "fs_dram_common.h"		/* fs_dram_init_common() */

//####include <asm/mach-imx/checkboot.h>
#ifdef CONFIG_FS_SECURE_BOOT
//####include <asm/mach-imx/hab.h>
//####include <stdbool.h>
//####include <hang.h>
#endif

#else

#include <linux/kconfig.h>		/* Get kconfig macros only */
#include "linux_helpers.h"
//####include "../../../include/fdt_support.h"
#include <linux/libfdt.h>
//####include "../../../include/linux/libfdt_env.h"
//####include <string.h>
#include <stdio.h>
#include <errno.h>
//####include "crc32.h"
#endif /* __UBOOT__ */

#include "fs_image_common.h"		/* Own interface */

struct env_info {
	unsigned int start[2];
	unsigned int size;
};

/* ------------- Functions only in U-Boot, not SPL ------------------------- */

#ifdef __UBOOT__
/*
 * Return if currently running from Secondary SPL. This function is called
 * early in boot_f phase of U-Boot and must not access any variables.
 */
bool fs_image_is_secondary(void)
{
	struct fs_header_v1_0 *fsh = fs_image_get_cfg_addr();
	u8 *size = (u8 *)&fsh->info.file_size_low;

	/*
	 * We know that a BOARD-CFG is smaller than 64KiB. So only the first
	 * two bytes of the file_size are actually used. Especially the 8th
	 * byte is definitely 0. SPL uses this byte to indicate if it was
	 * running from Primary (0) or Secondary SPL (<>0). Take this info and
	 * reset the byte to 0 before validating the BOARD-CFG.
	 */
	if (size[7]) {
		size[7] = 0;
		return true;
	}

	return false;
}

bool fs_image_is_secondary_uboot(void)
{
	struct fs_header_v1_0 *fsh = fs_image_get_cfg_addr();
	u8 *size = (u8 *)&fsh->info.file_size_low;

	/*
	 * Similar to the SPL, we use the "file_size_high" field of the
	 * BOARD-CFG as an indicator that we booted the UBoot from the
	 * secondary partition.
	 */
	if (size[6]) {
		size[6] = 0;
		return true;
	}

	printf("UBoot is secondary\n");
	return false;
}

/*
 * Return address of board configuration in OCRAM; search for it if not
 * available at expected place. This function is called early in boot_f phase
 * of U-Boot and must not access any variables. Use global data instead.
 */
bool fs_image_find_cfg_in_ocram(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	struct fs_header_v1_0 *fsh;
	const char *type = "BOARD-CFG";

	/* Try expected location first */
	fsh = fs_image_get_regular_cfg_addr();
	if (fs_image_match(fsh, type, NULL))
		return true;

	/*
	 * Search it from beginning of OCRAM.
	 *
	 * To avoid having to search this location over and over again, save a
	 * pointer to it in global data.
	 */
	fsh = (struct fs_header_v1_0 *)CFG_SYS_OCRAM_BASE;
	do {
		if (fs_image_match(fsh, type, NULL)) {
			gd->board_cfg = (ulong)fsh;
			return true;
		}
		fsh++;
	} while ((ulong)fsh < (CFG_SYS_OCRAM_BASE + CFG_SYS_OCRAM_SIZE));

	return false;
}
#endif /* __UBOOT__ */

static int fs_image_fdt_err(const char *name, const char *reason, int err)
{
	printf("Entry %s in BOARD-CFG %s\n", name, reason);

	return err;
}

/* Get count values from given device tree property and check alignment */
int fs_image_get_fdt_val(void *fdt, int offs, const char *name, uint align,
			 int count, uint *val)
{
	int len;
	const fdt32_t *start;
	int i;

	start = fdt_getprop(fdt, offs, name, &len);
	if (!start)
		return fs_image_fdt_err(name, "missing", -ENOENT);

	/* Be pedantic, if nboot-info values are wrong, nothing will work */
	if (!len || (len % sizeof(fdt32_t)))
		return fs_image_fdt_err(name, "invalid", -EINVAL);
	len /= sizeof(fdt32_t);
	if (len > count)
		return fs_image_fdt_err(name, "has too many values", -EINVAL);

	/* Fetch all available values */
	for (i = 0; i < len; i++) {
		val[i] = fdt32_to_cpu(start[i]);
		if (align && (val[i] % align))
			return fs_image_fdt_err(name, "not aligned", -EINVAL);
	}

	/* If we have less than count values, repeat the last value */
	for (i = len; i < count; i++)
		val[i] = val[len - 1];

	return 0;
}


#ifdef CONFIG_NAND_MXS
static const struct env_info fs_image_known_env_nand[] = {
	{
		.start = {0x480000, 0x4c0000},
		.size = 0x4000
	},
};

int fs_image_get_known_env_nand(uint index, uint start[2], uint *size)
{
	const struct env_info *env_info;

	printf("Using env location fallback #%d for old NBoot\n", index);
	if (index > ARRAY_SIZE(fs_image_known_env_nand)) {
		puts("No such fallback for env location\n");
		return -EINVAL;
	}

	env_info = &fs_image_known_env_nand[index];

	start[0] = env_info->start[0];
	start[1] = env_info->start[1];
	if (size)
		*size = env_info->size;

	return 0;
}

#endif

#ifdef CONFIG_MMC
/*
 * List of known environment positions before it was moved to nboot-info.
 *
 * Remarks:
 * - Old versions of PicoCoreMX8MN, where no redundand environment was used
 *   and the environment was on 0x400000, are not supported.
 * - efusMX8X still uses no redundand environment, updating from such a version
 *   is not implemented yet. (### TODO ###)
 */
#ifdef CONFIG_IMX8MM
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x100000, 0x104000},
		.size = 0x4000
	},
	{
		.start = {0x100000, 0x104000},
		.size = 0x2000
	},
};
#elif defined CONFIG_IMX8MN
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x138000, 0x13c000},
		.size = 0x4000
	},
};
#elif defined CONFIG_IMX8MP
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x138000, 0x13c000},
		.size = 0x4000
	},
	{
		.start = {0x100000, 0x104000},
		.size = 0x4000
	},
};
#elif defined CONFIG_IMX8X
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x200000, 0x200000},
		.size = 0x2000
	},
};
#else
static const struct env_info fs_image_known_env_mmc[0];
#endif

int fs_image_get_known_env_mmc(uint index, uint start[2], uint *size)
{
	const struct env_info *env_info;

	printf("Using env location fallback #%d for old NBoot\n", index);
	if (index > ARRAY_SIZE(fs_image_known_env_mmc)) {
		puts("No such fallback for env location\n");
		return -EINVAL;
	}

	env_info = &fs_image_known_env_mmc[index];

	start[0] = env_info->start[0];
	start[1] = env_info->start[1];
	if (size)
		*size = env_info->size;

	return 0;
}

#endif /* CONFIG_MMC */
