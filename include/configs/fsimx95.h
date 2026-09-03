// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 F&S Elektronik Systeme GmbH
 */

/*
 * OCRAM (352KB) layout SPL/ATF
 * --------------------
 * 0x2048_0000: SPL                  (~200KB)   CONFIG_SPL_TEXT_BASE
 * 0x204D_8000: (end of OCRAM)
 *
 * NPU SRAM (1024KB) layout SPL/ATF
 * --------------------
 * 0x4AA0_0000: BOARD-CFG            (8KB)      CONFIG_FUS_BOARDCFG_ADDR
 * 0x4AA0_2000: INDEX                (8KB)      CFG_FUS_INDEX_ADDR (Potentiell per SW überprüfen, ELE macht Probleme bei ULP -> Senatore)
 * 0x4AA0_4000: ATF/EARLY_AHAB_BASE  (64KB)     CFG_SPL_ATF_ADDR
 * 0x4AA1_4000: FDT                  (32KB)	CONFIG_SPL_MULTI_DTB_FIT_USER_DEF_ADDR
 * 0x4AA1_C000: DRAM-TIMING          (128KB)    CONFIG_SAVED_DRAM_TIMING_BASE
 * 0x4AA3_C000: DRAM-FW              (320KB)    CFG_SPL_DRAM_FW_ADDR (max size from oei-ddr)
 * 0x4AA8_C000: Limit of Stack       (0KB)      End of DRAM-FW
 * 0x4AAE_6C00: Stack                (363KB)    CONFIG_SPL_STACK - MALLOC_F_LEN (descending)
 * 0x4AAE_6C00: MALLOC_F             (96KB)     CONFIG_SPL_STACK - MALLOC_F_LEN
 * 0x4AAF_EC00: BSS                  (4KB)      CONFIG_SPL_BSS_START_ADDR / CONFIG_SPL_STACK
 * 0x4AAF_FC00: SDP loadbuffer       (1KB)      CONFIG_SDP_LOADADDR
 * 0x4AB0_0000: (end of SRAM)
 *
 * The DRAM_FW is loaded to the above address, validated and then handed off
 * to System Manager M33, where the actual DRAM initialization happens.
 *
 * DRAM Layout UBOOT/TEE
 * ---------------------
 * 0x8A20_0000: AHAB_BASE            (64KB)
 * 0x8C00_0000: TEE                  (32MB)
 * 0x9020_0000: UBOOT                (3MB)
 * 0x9050_0000: AHAB_BASE_NS         (64KB)
 * 0x9051_0000: TEE_NS               (32MB)
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

#include <config_fus_bootcmd.h>
/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	"arch=" CONFIG_SYS_BOARD "\0" 						\
	"script=boot_script.scr\0"						\
	"scriptaddr=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" 			\
	"kernel_addr_r=0x90400000\0"						\
	"fdt_addr_r=0x93000000\0"						\
	"fdt_addr=0x93000000\0"							\
	"fdt_high=0xffffffffffffffff\0"						\
	"set_bootfdt=setenv fdtfile ${platform}.dtb\0"				\
	"fdtfile=undef\0" 							\
	"cntr_addr_r=0xa8000000\0"						\
	"cntr_loadaddr=0x94000000\0"						\
	"bootcntrfile=os_cntr_signed.cntr\0" 					\
	"splashimage=0xa0000000\0" 						\
	"bootfile=Image\0" 							\
	"prepare_mcore=setenv mcore_clk pd_ignore_unused;\0" 		\
	"bootm_size=0x10000000\0" 						\
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" 			\
	"prefix=/\0"								\
	"updatecheck=undef\0"							\
	"installcheck=undef\0"							\
	"recovercheck=undef\0"							\
	FUS_BOOT_ENV								\
	FUS_AHAB_ENV								\
	FUS_AB_BOOT								\
	MCORE_BOOT								\
	FUS_LEGACY_BOOT								\
	FUS_WIN_BOOT								\
	".default_boot=setenv boot_targets fus_legacy "				\
		"mmc0 mmc1 usb0 usb1;\0"					\
	"boot_targets=fus_legacy mmc0 mmc1 usb0 usb1\0"

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR	0x90000000
#define CFG_SYS_INIT_RAM_SIZE	0x200000

#define CFG_SPL_FUS_EARLY_AHAB_BASE	CFG_SPL_ATF_ADDR

#define CFG_SYS_SDRAM_BASE		0x90000000
#define PHYS_SDRAM				0x90000000
#define PHYS_SDRAM_SIZE			0x70000000 /* 2GB - 256MB DDR */

#define CFG_SYS_SECURE_SDRAM_BASE	0x8A000000 /* Secure DDR region for A55, SPL could use first 2MB */
#define CFG_SYS_SECURE_SDRAM_SIZE	0x06000000

/* On boards with a running SM, eMMC is already configured in TRDC.
 * SPL has all rights to write to ATF and OPTee range, but USDHC
 * already lost access to it. The workaround is to define non secure
 * destinations for ATF and OPTee via container and later copy them
 * to the right addresses after container handling. */
#define CFG_SPL_ATF_SECURE_ADDR		0x8A200000
#define CFG_SPL_TEE_SECURE_ADDR		0x8C000000

#define WDOG_BASE_ADDR			WDG3_BASE_ADDR

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx95_evk_android.h"
#endif

#endif /* __FSIMX95_H */
