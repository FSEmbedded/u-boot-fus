// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 F&S Elektronik Systeme GmbH
 */

/*
 * TCM layout (SPL)
 * ----------------
 * unused
 *
 * OCRAM layout SPL/ATF
 * NOTE: SRAM2 is used as L2 Cache after SPL
 * --------------------
 * 0x2201_0000 (SRAM0): 	Region reserved by ROM loader (64KB)
 * --------------------
 * 0x2202_0000 (SRAM2):		SPL		(130KiB)CONFIG_SPL_TEXT_BASE
 * 0x2204_0800 (SRAM2):		FDT		(32KiB)	CONFIG_SPL_MULTI_DTB_FIT_USER_DEF_ADDR
 * 0x2204_2800 (SRAM2):		INDEX		(8KiB)	CFG_FUS_INDEX_ADDR
 * 0x2204_a800 (SRAM2):		SPL_STACK	(78KiB)	(+ MALLOC_F + GLOBAL_DATA)
 * 0x2205_E000 (SRAM2): 				CONFIG_SPL_STACK
 * 0x2205_E000 (SRAM2): 	BSS data	( 8KiB)	CONFIG_SPL_BSS_START_ADDR
 * 0x2206_0000 (SRAM2):		END
 * --------------------
 * 0x2004_0000 (SSRAM_P5): 	EARLY_AHAB_BASE/ATF	(192KiB)	CFG_SPL_ATF_ADDR
 * 0x2005_5000 (SSRAM_P5): 	DRAM-TIMING		(16KB)		CONFIG_SAVED_DRAM_TIMING_BASE
 * --------------------
 * 0x2100_E000 (SRAM1):		BOARD-CFG	(8KiB)	CONFIG_FUS_BOARDCFG_ADDR
 * --------------------
 * 
 * DRAM Layout UBOOT/TEE
 * ---------------------
 * 0x8000_0000:	AHAB_BASE            (64KB)     CFG_SYS_SDRAM_BASE
 * 0x8001_0000: (free)               (1984KB)
 * 0x8020_0000: UBOOT                (3MB)      CONFIG_TEXT_BASE
 * 0x8050_0000: (free)               (603MB)
 * 0xa6000000:	TEE                  (32MB)     CFG_SPL_TEE_ADDR
 * 0x48000000: (free)
 */

#ifndef __FSIMX8ULP_H
#define __FSIMX8ULP_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

/* RAM Layout */
#define CFG_SYS_OCRAM_BASE		0x22020000
#define CFG_SYS_OCRAM_SIZE		0x40000
#define CONFIG_FUS_BOARDCFG_ADDR		0x2100e000
#define CFG_SPL_DRAM_TIMING_ADDR	CONFIG_SAVED_DRAM_TIMING_BASE
#define CFG_SPL_INDEX_ADDR		0x22042800
#define CFG_SPL_ATF_ADDR		0x20040000
#define CFG_SPL_TEE_ADDR		0xa6000000

#define CFG_SYS_UBOOT_BASE \
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* ENET Config */
#if defined(CONFIG_FEC_MXC)
#define PHY_ANEG_TIMEOUT		20000
#define CONFIG_FEC_XCV_TYPE             RMII
#define FEC_QUIRK_ENET_MAC
#endif

#include <config_fus_bootcmd.h>
/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	"arch=" CONFIG_SYS_BOARD "\0" 						\
	"script=boot_script.scr\0"						\
	"scriptaddr=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" 			\
	"kernel_addr_r=0x80400000\0"						\
	"fdt_addr_r=0x83000000\0"						\
	"fdt_addr=0x83000000\0"							\
	"fdt_high=0xffffffffffffffff\0"						\
	"set_bootfdt=setenv fdtfile ${platform}.dtb\0"				\
	"fdtfile=undef\0" 							\
	"cntr_addr_r=0x98000000\0"						\
	"cntr_loadaddr=0x84000000\0"						\
	"bootcntrfile=os_cntr_signed.cntr\0" 					\
	"splashimage=0x90000000\0" 						\
	"bootfile=Image\0" 							\
	"prepare_mcore=setenv mcore_clk clk-imx93.mcore_booted;\0" 		\
	"bootm_size=0x10000000\0" 						\
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" 			\
	"prefix=/\0"								\
	"updatecheck=undef\0"							\
	"installcheck=undef\0"							\
	"recovercheck=undef\0"							\
	FUS_BOOT_ENV								\
	FUS_AHAB_ENV								\
	FUS_AB_BOOT								\
	FUS_LEGACY_BOOT								\
	".default_boot=setenv boot_targets fus_legacy "				\
		"mmc0 mmc1 usb0 usb1;\0"					\
	"boot_targets=fus_legacy mmc0 mmc1 usb0 usb1\0"


/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR	0x80000000
#define CFG_SYS_INIT_RAM_SIZE	0x80000

#define CFG_SYS_SDRAM_BASE		0x80000000
#define CFG_SPL_ATF_ADDR		0x20040000
#define CFG_SPL_FUS_EARLY_AHAB_BASE	CFG_SPL_ATF_ADDR
#define PHYS_SDRAM			0x80000000

/*
 * The real DRAM Size is determinied by BOARD-CFG.
 * PHYS_SDRAM_SIZE is used for xrdc configs and imx8ulp_arm64_mem_map.
 * The Value in imx8ulp_arm64_mem_map is overwritten during dram_init()
 * with the size from BOARD-CFG.
 */
#define PHYS_SDRAM_SIZE			0x40000000 /* 1GB DRAM-Region */

/* Monitor Command Prompt */

/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR			WDG3_RBASE

/* USB Configs */
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#endif
