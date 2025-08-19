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
 * --------------------
 * 0x2048_0000: (reserved)           (96KB)     (used by ROM loader)
 * 0x2049_8000: BOARD-CFG            (8KB)      CFG_FUS_BOARDCFG_ADDR
 * 0x2049_A000: SPL                  (176KB)    CONFIG_SPL_TEXT_BASE
 * 0x204C_6000: DRAM-FW              (96KB)     CFG_SPL_DRAM_FW_ADDR
 * 0x204D_E000: INDEX                (8KB)      CFG_FUS_INDEX_ADDR
 * 0x204E_0000: ATF/EARLY_AHAB_BASE  (64KB)     CFG_SPL_ATF_ADDR
 * 0x204F_0000: FDT		     (32KB)	CONFIG_SPL_MULTI_DTB_FIT_USER_DEF_ADDR
 * 0x204F_8000: SPL_STACK            (136KB)    (+ MALLOC_F + GLOBAL_DATA)
 * 0x2051_A000:                                 CONFIG_SPL_STACK
 * 0x2051_A000: BSS data             (8KB)      CONFIG_SPL_BSS_START_ADDR
 * 0x2051_C000: DRAM-TIMING          (16KB)     CONFIG_SAVED_DRAM_TIMING_BASE
 * 0x2051_FFFF: (end of OCRAM)
 *
 * The DRAM_FW is loaded to the above address, validated and then copied to
 * &_end of SPL where it is expected by the DRAM initialization code. The
 * DRAM-TIMING is loaded to the above address, validated and used for DRAM
 * initialization. Then ddr_init() copies it to CONFIG_SAVED_DRAM_TIMING_BASE
 * where it is expected by the ATF (required later when switching bus speeds).
 *
 * DRAM Layout UBOOT/TEE
 * ---------------------
 * 0x8000_0000: AHAB_BASE            (64KB)     CFG_SYS_SDRAM_BASE
 * 0x8001_0000: (free)               (1984KB)
 * 0x8020_0000: UBOOT                (3MB)      CONFIG_TEXT_BASE
 * 0x8050_0000: (free)               (347MB)
 * 0x9600_0000: TEE                  (32MB)     CFG_SPL_TEE_ADDR
 * 0x9800_0000: (free)
 */

#ifndef __FSIMX93_H
#define __FSIMX93_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

/* RAM Layout */
#define CFG_SYS_OCRAM_BASE		0x20498000
#define CFG_SYS_OCRAM_SIZE		0x88000
#define CFG_FUS_BOARDCFG_ADDR		CFG_SYS_OCRAM_BASE
#define CFG_SPL_DRAM_FW_ADDR		0x204C6000
#define CFG_SPL_DRAM_TIMING_ADDR	CONFIG_SAVED_DRAM_TIMING_BASE
#define CFG_SPL_INDEX_ADDR		0x204DE000
#define CFG_SPL_ATF_ADDR		0x204E0000
#define CFG_SPL_TEE_ADDR		0x96000000

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* ENET Config */
/* ENET1 */
#define CONFIG_SYS_DISCOVER_PHY

#if defined(CONFIG_CMD_NET)
#define FDT_SEQ_MACADDR_FROM_ENV

#define CONFIG_FEC_XCV_TYPE             RGMII
#define FEC_QUIRK_ENET_MAC

//#define DWC_NET_PHYADDR					4
#ifdef CONFIG_DWC_ETH_QOS
#define CONFIG_SYS_NONCACHED_MEMORY	(1 * SZ_1M) /* 1M */
#endif

#define PHY_ANEG_TIMEOUT 20000
#endif /* CONFIG_CMD_NET */

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
	MCORE_BOOT								\
	FUS_LEGACY_BOOT								\
	".default_boot=setenv boot_targets fus_legacy "				\
		"mmc0 mmc1 usb0 usb1;\0"					\
	"boot_targets=fus_legacy mmc0 mmc1 usb0 usb1\0"
	

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR		0x80000000
#define CFG_SYS_INIT_RAM_SIZE		0x200000

#define CFG_SYS_SDRAM_BASE		0x80000000
#define CFG_SPL_FUS_EARLY_AHAB_BASE	CFG_SPL_ATF_ADDR
#define PHYS_SDRAM			0x80000000
#define PHYS_SDRAM_SIZE			0x40000000 /* 1GB DDR */

#define CFG_SYS_FSL_USDHC_NUM	2


/* Monitor Command Prompt */

/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

#ifdef CONFIG_IMX_MATTER_TRUSTY
#define NS_ARCH_ARM64 1
#endif

#if CONFIG_IS_ENABLED(FS_DEVICEINFO_COMMON)
#define CFG_FS_DEVICEINFO_ADDR 0x80000000
#endif

#endif /* __FSIMX93_H */
