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
 * 0x204E_0000: ATF/EARLY_AHAB_BASE  (96KB)     CFG_SPL_ATF_ADDR
 * 0x204F_8000: SPL_STACK            (136KB)    (+ MALLOC_F + GLOBAL_DATA)
 * 0x2051_9DD0:                                 CONFIG_SPL_STACK
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
 * 0x9600_0000: TEE                  (2MB)      CFG_SPL_TEE_ADDR
 * 0x9620_0000: (free)
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

// /* eMMC Layout */
// /* Offsets in eMMC where BOARD-CFG and FIRMWARE are stored */
// #define CFG_FUS_BOARDCFG_MMC0	0x00048000
// #define CFG_FUS_BOARDCFG_MMC1	0x00448000

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

// Gets overridden by UBoot if the Board is closed
#define AHAB_ENV "sec_boot=no\0"
#define BOOT_OS "boot_os=booti ${loadaddr} - ${fdt_addr_r};\0"
#define BOOT_OS_FIT "boot_os_fit=bootm 0x84000000#conf-${fdtfile};\0"

#if defined(CONFIG_MTDIDS_DEFAULT)
#define CFG_MTDPART_DEFAULT ""
#endif

/* ENET Config */
/* ENET1 */
#define CONFIG_SYS_DISCOVER_PHY

#if defined(CONFIG_CMD_NET)
#define CONFIG_ETHPRIME                 "eth0" /* Set qos to primary since we use its MDIO */
#define FDT_SEQ_MACADDR_FROM_ENV

#define CONFIG_FEC_XCV_TYPE             RGMII
#define FEC_QUIRK_ENET_MAC

//#define DWC_NET_PHYADDR					4
#ifdef CONFIG_DWC_ETH_QOS
#define CONFIG_SYS_NONCACHED_MEMORY	(1 * SZ_1M) /* 1M */
#endif

#define PHY_ANEG_TIMEOUT 20000

#define CONFIG_PHY_ATHEROS
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_IPADDR		10.0.0.252
#define CONFIG_SERVERIP		10.0.0.122
#define CONFIG_GATEWAYIP	10.0.0.5
#define CONFIG_ROOTPATH		"/rootfs"

#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(USB, usb, 0)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

#define MTDIDS_DEFAULT		""
#define MTDPART_DEFAULT		""

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	BOOTENV \
	AHAB_ENV \
	"arch=" CONFIG_SYS_BOARD "\0" \
	"prepare_mcore=setenv mcore_clk clk-imx93.mcore_booted;\0" \
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
	"mmcargs=setenv bootargs ${mcore_clk} console=${console} root=${mmcroot}\0 " \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"script=update_signed.scr \0" \
	"loadsignedscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"signedscript=echo Authenticate signed script...;" \
		"if auth_cntr ${loadaddr}; then " \
			"echo Running script...;" \
			"source;" \
		"else " \
			"echo Script cannot be authenticated, continue without script!;" \
		"fi\0" \
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
	"netargs=setenv bootargs ${mcore_clk} console=${console} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serveriploadaddrc}:${nfsroot},v3,tcp\0" \
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

#define CFG_SYS_INIT_RAM_ADDR		0x80000000
#define CFG_SYS_INIT_RAM_SIZE		0x200000

#define CFG_SYS_SDRAM_BASE		0x80000000
#define CFG_SPL_FUS_EARLY_AHAB_BASE	0x204e0000
#define PHYS_SDRAM			0x80000000
#define PHYS_SDRAM_SIZE			0x40000000 /* 1GB DDR */

#define CFG_SYS_FSL_USDHC_NUM	2

/* TODO: */
/* have to define for F&S serial_mxc driver */
#define UART1_BASE		0x44380000
#define UART2_BASE		0x44390000
#define UART3_BASE		0x42570000
#define UART4_BASE		0x42580000
#define UART5_BASE		0x42590000
#define UART6_BASE		0x425a0000
#define UART7_BASE		0x42690000
#define UART8_BASE		0x426a0000
#define UART9_BASE		0xFFFFFFFF

/* Monitor Command Prompt */

/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

#if defined(CONFIG_CMD_NET)
#define PHY_ANEG_TIMEOUT 20000
#endif

#ifdef CONFIG_IMX_MATTER_TRUSTY
#define NS_ARCH_ARM64 1
#endif

#if CONFIG_IS_ENABLED(FS_DEVICEINFO_COMMON)
#define CFG_FS_DEVICEINFO_ADDR 0x80000000
#endif

#endif /* __FSIMX93_H */
