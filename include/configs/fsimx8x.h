/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FSIMX8X_H
#define __FSIMX8X_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

/* disable FAT write becaue its dosn't work 
   with F&S FAT driver
*/
#undef CONFIG_FAT_WRITE

/* need for F&S bootaux */
#define M4_BOOTROM_BASE_ADDR           0x0
#define IMX_SIP_SRC_M4_START           IMX_SIP_SRC_MCU_START
#define IMX_SIP_SRC_M4_STARTED         IMX_SIP_SRC_MCU_STARTED

/* The load address of U-Boot is now independent from the size. Just load it
   at some rather low address in RAM. It will relocate itself to the end of
   RAM automatically when executed. */
#define CONFIG_BOARD_SIZE_LIMIT 0x100000	/* Size of uboot.nb0 */

/* The final stack sizes are set up in board.c using the settings below */
#define CONFIG_SYS_STACK_SIZE	(128*1024)

#ifdef CONFIG_SPL_BUILD

#define CONFIG_PARSE_CONTAINER

#ifdef CONFIG_QSPI_BOOT
#define CONFIG_SPL_SPI_LOAD
#endif

#define CONFIG_SPL_TEXT_BASE            0x0
#define CONFIG_SPL_MAX_SIZE             (124 * 1024)
#define CONFIG_SYS_MONITOR_LEN          (1024 * 1024)

#ifdef CONFIG_NAND_BOOT
#ifndef CONFIG_PARSE_CONTAINER
#define CONFIG_SPL_NAND_RAW_ONLY
#endif
#define CONFIG_SPL_NAND_SUPPORT
#define CONFIG_SPL_DMA_SUPPORT
#define CONFIG_SPL_NAND_MXS
#define CONFIG_SYS_NAND_U_BOOT_OFFS     (0x8000000)  /*Put the FIT out of first 128MB boot area */
#define CONFIG_SPL_NAND_BOOT
#define CONFIG_SYS_NAND_U_BOOT_DST		0x80000000
#define CONFIG_SYS_NAND_U_BOOT_SIZE     (1024 * 1024 )

#define CONFIG_SYS_NAND_U_BOOT_START    0x80000000
#endif

#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR         0x1040 /* (32K + 2Mb)/sector_size */
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION              0


#define CONFIG_SPL_WATCHDOG_SUPPORT
#define CONFIG_SPL_DRIVERS_MISC_SUPPORT
#define CONFIG_SPL_LDSCRIPT             "arch/arm/cpu/armv8/u-boot-spl.lds"
#define CONFIG_SPL_STACK                0x013E000
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_BSS_START_ADDR      0x00138000
#define CONFIG_SPL_BSS_MAX_SIZE        0x8000	/* 20 KB */
#define CONFIG_SYS_SPL_MALLOC_START    0x00120000
#define CONFIG_SYS_SPL_MALLOC_SIZE     0x18000   /* 60 KB */
#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_MALLOC_F_ADDR            0x00120000 /* malloc f used before GD_FLG_FULL_MALLOC_INIT set */

#define CONFIG_SPL_RAW_IMAGE_ARM_TRUSTED_FIRMWARE

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE /* For RAW image gives a error info not panic */

#define CONFIG_OF_EMBED
#define CONFIG_ATF_TEXT_BASE 0x80000000
#define CONFIG_SYS_ATF_START 0x80000000
/* #define CONFIG_FIT */

/* Since the SPL runs before ATF, MU1 will not be started yet, so use MU0 */
#define SC_IPC_CH                       SC_IPC_AP_CH0

#endif

#define CONFIG_REMAKE_ELF

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_ARCH_MISC_INIT

/* Flat Device Tree Definitions */
#define CONFIG_OF_BOARD_SETUP

#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_IMPORTENV
#undef CONFIG_CMD_IMLS

#undef CONFIG_CMD_CRC32
#undef CONFIG_BOOTM_NETBSD

#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR       0
#define USDHC1_BASE_ADDR                0x5B010000
#define USDHC2_BASE_ADDR                0x5B020000
#define CONFIG_SUPPORT_EMMC_BOOT	/* eMMC specific */

#define CONFIG_ENV_OVERWRITE


#define CONFIG_FSL_HSIO
#ifdef CONFIG_FSL_HSIO
#define CONFIG_PCIE_IMX8X
#define CONFIG_CMD_PCI
#define CONFIG_PCI
#define CONFIG_PCI_PNP
#define CONFIG_PCI_SCAN_SHOW
#endif

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

/* FUSE command */
#define CONFIG_CMD_FUSE

/* GPIO configs */
#define CONFIG_MXC_GPIO

/* ENET Config */
#define CONFIG_MII

#define CONFIG_FEC_MXC
#define CONFIG_FEC_XCV_TYPE             RGMII
#define FEC_QUIRK_ENET_MAC

#define CONFIG_PHY_GIGE /* Support for 1000BASE-X */

#define CONFIG_PHYLIB
#define CONFIG_PHY_ATHEROS

/* ENET0 & ENET1 connects AR8031 on CPU board */
#define CONFIG_FEC_ENET_DEV 0

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			0x5B040000
#define CONFIG_FEC_MXC_PHYADDR          0x4
#define CONFIG_ETHPRIME                 "eth0"
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			0x5B050000
#define CONFIG_FEC_MXC_PHYADDR          0x5
#define CONFIG_ETHPRIME                 "eth1"
#endif


/* ENET0 MDIO are shared */
#define CONFIG_FEC_MXC_MDIO_BASE	0x5B040000

#define CONFIG_LIB_RAND
#define CONFIG_NET_RANDOM_ETHADDR

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV "sec_boot=yes\0"
#else
#define AHAB_ENV "sec_boot=no\0"
#endif

/* Boot M4 */
#define M4_BOOT_ENV \
	"m4_0_image=m4_0.bin\0" \
	"loadm4image_0=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4_0_image}\0" \
	"m4boot_0=run loadm4image_0; dcache flush; bootaux ${loadaddr} 0\0" \

#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:128m(nandboot),16m(nandfit),32m(nandkernel),16m(nanddtb),8m(nandtee),-(nandrootfs) "
#endif

#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"clk_ignore_unused "\
	"\0" \
	"initrd_addr=0x83100000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0" \
	"sd_dev=1\0" \

/* Initial environment variables */
#ifdef CONFIG_NAND_BOOT
#define CONFIG_EXTRA_ENV_SETTINGS		\
	CONFIG_MFG_ENV_SETTINGS \
	"bootargs=console=ttyLP0,115200 ubi.mtd=6 "  \
		"root=ubi0:nandrootfs rootfstype=ubifs "		     \
		MFG_NAND_PARTITION \
		"\0"\
	"console=ttyLP0,115200 earlycon=lpuart32,0x5a080000,115200\0" \
	"mtdparts=" MFG_NAND_PARTITION "\0" \
	"fdt_addr=0x83000000\0"
#else
#define CONFIG_EXTRA_ENV_SETTINGS		\
	CONFIG_MFG_ENV_SETTINGS \
	M4_BOOT_ENV \
	AHAB_ENV \
	"script=boot.scr\0" \
	"image=Image\0" \
	"panel=NULL\0" \
	"console=ttyLP2\0" \
	"earlycon=lpuart32,0x5a080000\0" \
	"fdt_addr=0x83000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"cntr_addr=0x98000000\0"			\
	"cntr_file=os_cntr_signed.bin\0" \
	"boot_fdt=try\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} earlycon=${earlycon},${baudrate} root=${mmcroot}\0 " \
	"loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"loadcntr=fatload mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${cntr_file}\0" \
	"auth_os=auth_cntr ${cntr_addr}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${sec_boot} = yes; then " \
			"if run auth_os; then " \
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if run loadfdt; then " \
					"booti ${loadaddr} - ${fdt_addr}; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"else " \
				"echo wait for boot; " \
			"fi;" \
		"fi;\0" \
	"ethaddr=00:05:51:07:55:83\0" \
	"eth1addr=00:05:51:07:55:84\0" \
	"ipaddr=10.0.0.23\0" \
	"serverip=10.0.0.103\0" \
	"tftpflash=echo Loading flash.bin to RAM; " \
		"if tftp flash.bin; then " \
			"if run emmcflash; then " \
				"echo Successfully flashed eMMC; " \
			"else " \
				"echo ERR: failed to flash to eMMC; " \
			"fi; " \
		"else " \
			"echo ERR: failed to load flash.bin from tftp; " \
		"fi;\0" \
	"emmcflash= mmc dev 0 1; " \
		"setexpr blkcnt ${filesize} + 0x1ff; " \
		"setexpr blkcnt ${blkcnt} / 0x200; " \
		"echo Writing RAM to flash; " \
		"mmc write ${loadaddr} 0x0 ${blkcnt}; " \
		"mmc dev 0 0;\0" \
	"netargs=setenv bootargs console=${console},${baudrate} earlycon=${earlycon},${baudrate} " \
	"root=/dev/nfs ip=dhcp nfsroot=${serverip}:/rootfs,v3,tcp ro\0" \
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
				"booti ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"${get_cmd} ${loadaddr} ${image}; " \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
					"booti ${loadaddr} - ${fdt_addr}; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"else " \
				"booti; " \
			"fi;" \
		"fi;\0"
#endif

#ifdef CONFIG_NAND_BOOT
#define CONFIG_BOOTCOMMAND \
	"nand read ${loadaddr} 0x9000000 0x2000000;"\
	"nand read ${fdt_addr} 0xB000000 0x100000;"\
	"booti ${loadaddr} - ${fdt_addr}"
#else
#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadbootscript; then " \
			   "run bootscript; " \
		   "else " \
			   "if test ${sec_boot} = yes; then " \
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
		   "fi; " \
	   "else booti ${loadaddr} - ${fdt_addr}; fi"
#endif

/* Link Definitions */
#define CONFIG_LOADADDR			0x80280000

#define CONFIG_SYS_LOAD_ADDR           CONFIG_LOADADDR

#define CONFIG_SYS_INIT_SP_ADDR         0x80200000


/* Default environment is in SD */
#define CONFIG_ENV_SIZE			0x2000

#ifdef CONFIG_NAND_BOOT
#define CONFIG_ENV_OFFSET       (120 << 20)
#elif defined(CONFIG_QSPI_BOOT)
#define CONFIG_ENV_OFFSET       (4 * 1024 * 1024)
#define CONFIG_ENV_SECT_SIZE	(128 * 1024)
#define CONFIG_ENV_SPI_BUS	CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS	CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE	CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ	CONFIG_SF_DEFAULT_SPEED
#else
#define CONFIG_ENV_OFFSET       SZ_2M // (64 * SZ_64K)
#define CONFIG_SYS_MMC_ENV_PART		1	/* boot partition */
#endif

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

/* On LPDDR4 board, USDHC1 is for eMMC, USDHC2 is for SD on CPU board
  */
#define CONFIG_SYS_MMC_ENV_DEV		0   /* USDHC1 */
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */
#define CONFIG_SYS_FSL_USDHC_NUM	2

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		((CONFIG_ENV_SIZE + (32*1024)) * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_2			0x880000000
#define PHYS_SDRAM_1_SIZE		0x40000000	/* 1 GB totally */
#define PHYS_SDRAM_2_SIZE		0x00000000
#define PHYS_SDRAM_SIZE (PHYS_SDRAM_1_SIZE + PHYS_SDRAM_2_SIZE)

#define CONFIG_SYS_MEMTEST_START    0xA0000000
#define CONFIG_SYS_MEMTEST_END      (CONFIG_SYS_MEMTEST_START + (PHYS_SDRAM_1_SIZE >> 2))

/* Serial */
#define CONFIG_BAUDRATE			115200

#define CONFIG_SYS_SERCON_NAME "ttymxc"	/* Base name for serial devices */
#define CONFIG_SYS_UART_PORT	2	/* Default UART port */
#define CONFIG_CONS_INDEX       (CONFIG_SYS_UART_PORT)

/* have to define for F&S serial_mxc / serial_lpuart driver */
#define LPUART1_BASE (LPUART_BASE + 0x00000)
#define LPUART2_BASE (LPUART_BASE + 0x10000)
#define LPUART3_BASE (LPUART_BASE + 0x20000)
#define LPUART4_BASE (LPUART_BASE + 0x30000)
#define LPUART5_BASE 0xFFFFFFFF

/* Monitor Command Prompt */
#define CONFIG_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2     "> "
#define CONFIG_SYS_CBSIZE              1024
#define CONFIG_SYS_MAXARGS             64
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		8000000	/* 8MHz */

#ifndef CONFIG_DM_PCA953X
#define CONFIG_PCA953X
#define CONFIG_CMD_PCA953X
#define CONFIG_CMD_PCA953X_INFO
#endif

#define CONFIG_IMX_SMMU

#ifdef CONFIG_FSL_FSPI
#define CONFIG_SF_DEFAULT_BUS		0
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_SPEED	40000000
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define FSL_FSPI_FLASH_SIZE		SZ_1M
#define FSL_FSPI_FLASH_NUM		1
#define FSPI0_BASE_ADDR			0x5d120000
#define FSPI0_AMBA_BASE			0
#define CONFIG_SYS_FSL_FSPI_AHB
#endif

#ifdef CONFIG_CMD_NAND
#define CONFIG_NAND_MXS
#define CONFIG_CMD_NAND_TRIMFFS

/* NAND stuff */
#define CONFIG_SYS_MAX_NAND_DEVICE     1
#define CONFIG_SYS_NAND_BASE           0x40000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION

/* DMA stuff, needed for GPMI/MXS NAND support */
#define CONFIG_APBH_DMA
#define CONFIG_APBH_DMA_BURST
#define CONFIG_APBH_DMA_BURST8
#endif

/* USB Config */
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2

/* USB OTG controller configs */
#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#endif
#endif /* CONFIG_CMD_USB */

#ifdef CONFIG_USB_GADGET
#define CONFIG_USBD_HS
#endif

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx8qxp_arm2_android.h"
#endif

/* Framebuffer */
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_IMXDPUV1
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_IMX_VIDEO_SKIP
#endif

#define CONFIG_OF_SYSTEM_SETUP

#define CONFIG_CMD_READ
#define CONFIG_SERIAL_TAG

#endif /* __FSIMX8X_H */
