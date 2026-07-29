/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 NXP
 */

/*
 * OCRAM (352KB) layout SPL/ATF
 * --------------------
 * 0x2048_0000: SPL                  (180KB)    CONFIG_SPL_TEXT_BASE
 * 0x204D_6000: Stack                           CONFIG_SPL_STACK / CONFIG_SPL_BSS_START_ADDR
 * 0x204D_8000: (end of OCRAM)                  End of BSS data
 *
 * NPU SRAM (1024KB) layout SPL/ATF
 * --------------------
 * 0x4AA0_0000: BOARD-CFG            (8KB)      CONFIG_FUS_BOARDCFG_ADDR
 * 0x4AA0_2000: INDEX                (8KB)      CFG_FUS_INDEX_ADDR (Potentiell per SW überprüfen, ELE macht Probleme bei ULP -> Senatore)
 * 0x4AA0_4000: ATF/EARLY_AHAB_BASE  (64KB)     CFG_SPL_ATF_ADDR
 * 0x4AA1_4000: FDT                  (32KB)	CONFIG_SPL_MULTI_DTB_FIT_USER_DEF_ADDR
 * 0x4AA1_C000: DRAM-TIMING          (128KB)    CONFIG_SAVED_DRAM_TIMING_BASE
 * 0x4AA3_C000: DRAM-FW              (320KB)    CFG_SPL_DRAM_FW_ADDR (max size from oei-ddr)
 * 0x4AA8_C000: (not used)           (463KB)    Available for future use
 * 0x4AAD_FC00: SPL MALLOC           (128KB)    SPL_CUSTOM_MALLOC_ADDR
 * 0x4AAF_FC00: SDP loadbuffer       (1KB)      CONFIG_SDP_LOADADDR
 * 0x4AB0_0000: (end of SRAM)
 *
 * The DRAM_FW is loaded to the above address, validated and then copied to
 * &_end of SPL where it is expected by the DRAM initialization code.
 *
 * DRAM Layout UBOOT/TEE
 * ---------------------
 * 0x8A20_0000: AHAB_BASE            (64KB)
 * 0x8C00_0000: TEE                  (32MB)
 * 0x9020_0000: UBOOT                (3MB)
 */

#ifndef __FSIMX95_H
#define __FSIMX95_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>

/* RAM Layout */
/* Use NPU_SRAM instead of OCRAM for i.MX95 */
#define CFG_SYS_NPU_SRAM_BASE		0x4AA00000
#define CFG_SYS_NPU_SRAM_SIZE		0x100000
#define CFG_SYS_OCRAM_BASE		CFG_SYS_NPU_SRAM_BASE
#define CFG_SYS_OCRAM_SIZE		CFG_SYS_NPU_SRAM_SIZE
#define CONFIG_FUS_BOARDCFG_ADDR		CFG_SYS_OCRAM_BASE
#define CFG_SPL_DRAM_FW_ADDR		0x4AA3C000
#define CFG_SPL_DRAM_FW_EXE_ADDR	CFG_SPL_DRAM_FW_ADDR
#define CFG_SPL_DRAM_TIMING_ADDR	CONFIG_SAVED_DRAM_TIMING_BASE
#define CFG_SPL_INDEX_ADDR		0x4AA02000
#define CFG_SPL_ATF_ADDR		0x4AA04000
//#define CFG_MALLOC_F_ADDR		0x4AA80000 // -> Breaks U-Boot (NPU SRAM usage?)
#define CFG_SPL_TEE_ADDR		0x8C000000

#define CFG_SYS_INIT_RAM_ADDR	0x90000000
#define CFG_SYS_INIT_RAM_SIZE	0x200000

#define CFG_SYS_SDRAM_BASE		0x90000000
#define PHYS_SDRAM				0x90000000

#define PHYS_SDRAM_SIZE			0x70000000 /* 2GB - 256MB DDR */
#ifdef CONFIG_TARGET_IMX95_15X15_EVK
#define PHYS_SDRAM_2_SIZE		0x180000000 /* 6GB (Totally 8GB) */
#else
#define PHYS_SDRAM_2_SIZE		0x380000000 /* 14GB (Totally 16GB) */
#endif

#define CFG_SYS_SECURE_SDRAM_BASE	0x8A000000 /* Secure DDR region for A55, SPL could use first 2MB */
#define CFG_SYS_SECURE_SDRAM_SIZE	0x06000000

#define WDOG_BASE_ADDR			WDG3_BASE_ADDR

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx95_evk_android.h"
#endif

#endif
