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
 * 0x2100_E000 (SRAM1):		BOARD-CFG	(8KiB)	CFG_FUS_BOARDCFG_ADDR
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
#define CFG_FUS_BOARDCFG_ADDR		0x2100e000
#define CFG_SPL_DRAM_TIMING_ADDR	CONFIG_SAVED_DRAM_TIMING_BASE
#define CFG_SPL_INDEX_ADDR		0x22042800
#define CFG_SPL_ATF_ADDR		0x20040000
#define CFG_SPL_TEE_ADDR		0xa6000000

#define CFG_SYS_UBOOT_BASE \
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

// Gets overridden by UBoot if the Board is closed
#define AHAB_ENV "sec_boot=no\0"
#define BOOT_OS "boot_os=booti ${loadaddr} - ${fdt_addr_r};\0"
#define BOOT_OS_FIT "boot_os_fit=bootm 0x84000000#conf-${fdtfile};\0"

#if defined(CONFIG_MTDIDS_DEFAULT)
#define CFG_MTDPART_DEFAULT ""
#endif

/* ENET Config */
#if defined(CONFIG_FEC_MXC)
#define PHY_ANEG_TIMEOUT		20000
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 0)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x83800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0"\
	"sd_dev=2\0"

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	BOOTENV \
	AHAB_ENV \
	"arch=" CONFIG_SYS_BOARD "\0" \
	"scriptaddr=0x83500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"image=Image\0" \
	"splashimage=0x90000000\0" \
	"console=undef\0" \
	".console_none=setenv console\0"				\
	".console_serial=setenv console ${sercon},${baudrate}\0" \
	".console_display=setenv console tty1\0"		\
	"fdt_addr_r=0x83000000\0"			\
	"fdt_addr=0x83000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"cntr_addr=0x98000000\0"			\
	"cntr_file=os_cntr_signed.cntr\0" \
	"boot_fit=no\0" \
	"fdtfile=undef\0" \
	"set_bootfdt=setenv fdtfile ${platform}.dtb\0"			\
	"bootm_size=0x10000000\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcroot=/dev/mmcblk0p2 rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs ${jh_clk} console=${console} root=${mmcroot}\0 " \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr_r} ${fdtfile}\0" \
	"loadcntr=fatload mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${cntr_file}\0" \
	"auth_os=auth_cntr ${cntr_addr}\0" \
	BOOT_OS \
	BOOT_OS_FIT \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${sec_boot} = yes; then " \
			"if run auth_os; then " \
				"run boot_os_fit; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
				"bootm ${loadaddr}; " \
			"else " \
				"if run loadfdt; then " \
					"run boot_os; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi;" \
		"fi;\0" \
	"netargs=setenv bootargs ${jh_clk} console=${console} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs;  " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if test ${sec_boot} = yes; then " \
			"${get_cmd} ${cntr_addr} ${cntr_file}; " \
			"if run auth_os; then " \
				"run boot_os_fit; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"${get_cmd} ${loadaddr} ${image}; " \
			"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
				"bootm ${loadaddr}; " \
			"else " \
				"if ${get_cmd} ${fdt_addr_r} ${fdtfile}; then " \
					"run boot_os; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi;" \
		"fi;\0" \
	"bsp_bootcmd=echo Running BSP bootcmd ...; " \
		"mmc dev ${mmcdev}; " \
		"if mmc rescan; then " \
			"if test ${sec_boot} = yes; then " \
				"if run loadsignedscript; then " \
		  			"run signedscript; " \
	   			"fi; " \
		   		"if run loadcntr; then " \
			   		"run mmcboot; " \
		   		"else run netboot; " \
		   		"fi; " \
		   	"else " \
		   		"if run loadimage; then " \
			   		"run mmcboot; " \
		   		"else run netboot; " \
		   		"fi; " \
			"fi; " \
		"fi;"

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR	0x80000000
#define CFG_SYS_INIT_RAM_SIZE	0x80000

#define CFG_SYS_SDRAM_BASE		0x80000000
#define CFG_SPL_ATF_ADDR		0x20040000
#define CFG_SPL_FUS_EARLY_AHAB_BASE	CFG_SPL_ATF_ADDR
#define PHYS_SDRAM			0x80000000

/**
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

// /* USB Configs */
// #define CONFIG_USB_MAX_CONTROLLER_COUNT 2

#endif
