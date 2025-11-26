/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 F&S Elektronik Systeme GmbH
 *
 * Configuration settings for all F&S boards based on i.MX7ULP. This is
 * PicoCoreMX7ULP.
 *
 * Activate with one of the following targets:
 *   make fsimx7ulp_config   Configure for i.MX7ULP boards
 *   make                     Build u-boot-dtb.imx
 *
 *
 * The following addresses are given as offsets of the device.
 *
 * eMMC flash layout with separate Kernel/FDT MTD partition
 * -------------------------------------------------------------------------
 * BOOTPARTITION1
 * 0x0000_0400 - 0x000C_0400: UBoot: U-Boot image (768KB)
 * 0x0010_0000 - 0x0010_8000: UBoot: U-Boot Environment (32KB)
 * 0x0010_8000 - 0x0040_0000: Empty (3MB-32KB)
 *
 * User HW partition only:
 * 0x0000_0000 - 0x0000_0200: MasterBootRecord (512B)
 * 0x0010_0000 - 0x0200_0000: System: Contains Kernel/FDT/M4 image (31MB)
 * 0x0200_0000 - 0x0A8B_8000: ROOTFS: Root filesystem (dynamic)
 * 0x0A8B_9000 -         END: Data: data partition (Size - rootfs offset)
 *
 *
 */

#ifndef __FSIMX7ULP_CONFIG_H
#define __FSIMX7ULP_CONFIG_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>		/* IRAM_BASE_ADDR, IRAM_SIZE */

/*****************************************************************************
 * High Level Configuration Options
 *****************************************************************************/
/*### TODO: check if needed NXP specific mx7ulp_evk */
#define CFG_SYS_HZ_CLOCK	1000000 /* Fixed at 1Mhz from TSTMR */

/* set Cortex-A7 specific addresses */
#define SRC_BASE_ADDR		CMC1_RBASE
#define IRAM_BASE_ADDR		OCRAM_0_BASE
#define IOMUXC_BASE_ADDR	IOMUXC1_RBASE

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR		WDG1_RBASE

/* Define U-Boot offset in emmc */
#define CFG_SYS_MMC_U_BOOT_START 0x00000400

/*****************************************************************************
 * Memory Layout
 *****************************************************************************/
/* Physical addresses of DDR and CPU-internal SRAM */
#define PHYS_SDRAM		0x60000000ul
#define PHYS_SDRAM_SIZE		SZ_1G
#define CFG_SYS_SDRAM_BASE	PHYS_SDRAM

/* MX7ULP has 256KB SRAM, mapped from 0x2F000000-0x2F03FFFF */
#define CFG_SYS_INIT_RAM_ADDR	(IRAM_BASE_ADDR)
#define CFG_SYS_INIT_RAM_SIZE	SZ_256K

/* Init value for stack pointer, set at end of internal SRAM, keep room for
   global data behind stack. */
#define CFG_SYS_INIT_SP_OFFSET (CFG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)


/*****************************************************************************
 * Clock Settings and Timers
 *****************************************************************************/
/* Basic input clocks */


/*****************************************************************************
 * GPIO
 *****************************************************************************/


/*****************************************************************************
 * OTP Memory (Fuses)
 *****************************************************************************/


/*****************************************************************************
 * Serial Console (UART)
 *****************************************************************************/
#define LPUART_BASE		LPUART4_RBASE
#define CFG_SYS_UART_PORT	1	/* Default UART port */
#define CONFIG_CONS_INDEX       (CFG_SYS_UART_PORT)


/*****************************************************************************
 * I2C
 *****************************************************************************/


/*****************************************************************************
 * LEDs
 *****************************************************************************/
/* ### TODO: check if needed F&S specific */
/* #define CONFIG_BLINK_IMX */


/*****************************************************************************
 * PMIC
 *****************************************************************************/
/* PMIC on F&S i.MX7ULP boards in UBoot */
/* ### TODO: check if needed F&S specific */


/*****************************************************************************
 * Real Time Clock (RTC)
 *****************************************************************************/
/* ###TODO### */


/*****************************************************************************
 * Ethernet
 *****************************************************************************/
/* no ethernet on i.MX7ULP boards */


/*****************************************************************************
 * USB Host
 *****************************************************************************/
/* Use USB1 as host */
/* ### TODO: check if needed F&S specific */
/* #define CONFIG_USB_EHCI_POWERDOWN */	/* Shut down VBUS power on usb stop */
#define CONFIG_USB_MAX_CONTROLLER_COUNT 1


/*****************************************************************************
 * USB Device
 *****************************************************************************/


/*****************************************************************************
 * Keyboard
 *****************************************************************************/


/*****************************************************************************
 * SD/MMC Card, eMMC
 *****************************************************************************/
#define CFG_SYS_FSL_ESDHC_ADDR 	0
#define CFG_SYS_FSL_USDHC_NUM   1


/*****************************************************************************
 * NOR Flash
 *****************************************************************************/
/* No NOR flash on F&S i.MX7ULP boards */


/*****************************************************************************
 * SPI Flash
 *****************************************************************************/
#ifdef CONFIG_FSL_QSPI
//####define FSL_QSPI_FLASH_NUM	1
//####define FSL_QSPI_FLASH_SIZE	SZ_64M
//####define QSPI0_BASE_ADDR		0x410A5000
//####define QSPI0_AMBA_BASE		0xC0000000
#endif

/*****************************************************************************
 * NAND Flash
 *****************************************************************************/


/*****************************************************************************
 * Command Line Editor (Shell)
 *****************************************************************************/


/*****************************************************************************
 * Command Definition
 *****************************************************************************/


/*****************************************************************************
 * Display (LCD)
 *****************************************************************************/
/* ### TODO: add display support for picocoremx7ulp */
#if 0
/* ### TODO: check if needed */
#define CONFIG_VIDEO_MXS		/* Use MXS display driver */
#define CONFIG_VIDEO_LOGO		/* Allow a logo on the console... */
#define CONFIG_VIDEO_BMP_LOGO		/* ...as BMP image... */
#define CONFIG_BMP_16BPP		/* ...with 16 bits per pixel */
#define CONFIG_SPLASH_SCREEN		/* Support splash screen */

/* ### TODO: check if needed NXP */
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_CMD_BMP
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_IMX_VIDEO_SKIP

#define CONFIG_MXC_MIPI_DSI_NORTHWEST
#define CONFIG_HX8363
#endif


/*****************************************************************************
 * Network Options
 *****************************************************************************/
/* ### TODO: check if needed - for network on variables */


/*****************************************************************************
 * Filesystem Support
 *****************************************************************************/


/*****************************************************************************
 * Generic MTD Settings
 *****************************************************************************/


/*****************************************************************************
 * M4 specific configuration
 *****************************************************************************/
#define MCU_BOOTROM_BASE_ADDR	TCML_BASE


/*****************************************************************************
 * Environment
 *****************************************************************************/
/*
 * Environment size and location are now set in the defconfig. The environment
 * is held in the heap, so keep the real size small to not waste malloc space.
 * We could activate CONFIG_SYS_REDUNDAND_ENVIRONMENT, this would make sense.
 */

/* Add some variables that are not predefined in U-Boot. For example set
   fdt_high to 0xffffffff to avoid that the device tree is relocated to the
   end of memory before booting, which is not necessary in our setup (and
   would result in problems if RAM is larger than ~1,7GB).

   All entries with content "undef" will be updated in board_late_init() with
   a board-specific value (detected at runtime).

   We use ${...} here to access variable names because this will work with the
   simple command line parser, who accepts $(...) and ${...}, and also the
   hush parser, who accepts ${...} and plain $... without any separator.

   If a variable is meant to be called with "run" and wants to set an
   environment variable that contains a ';', we can either enclose the whole
   string to set in (escaped) double quotes, or we have to escape the ';' with
   a backslash. However when U-Boot imports the environment from NAND into its
   hash table in RAM, it interprets escape sequences, which will remove a
   single backslash. So we actually need an escaped backslash, i.e. two
   backslashes. Which finally results in having to type four backslashes here,
   as each backslash must also be escaped with a backslash in C. */
#define BOOT_WITH_FDT "\\\\; bootm ${loadaddr} - ${fdtaddr}\0"


#ifdef CONFIG_BOOTDELAY
#define FSBOOTDELAY
#else
#define FSBOOTDELAY "bootdelay=undef\0"
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bd_auxcore=undef\0" \
	"bd_kernel=undef\0" \
	"bd_fdt=undef\0" \
	"bd_rootfs=undef\0" \
	"console=undef\0" \
	".console_none=setenv console\0" \
	".console_serial=setenv console console=${sercon},${baudrate}\0" \
	".console_display=setenv console console=tty1\0" \
	"login=undef\0" \
	".login_none=setenv login login_tty=null\0" \
	".login_serial=setenv login login_tty=${sercon},${baudrate}\0" \
	".login_display=setenv login login_tty=tty1\0" \
	".network_off=setenv network\0" \
	".network_on=setenv network ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}:${netdev}\0" \
	".network_dhcp=setenv network ip=dhcp\0" \
	"rootfs=undef\0" \
	".rootfs_nfs=setenv rootfs root=/dev/nfs nfsroot=${serverip}:${rootpath}\0" \
	".rootfs_mmc=setenv rootfs root=/dev/mmcblk${usdhcdev}p2 rootwait\0" \
	".rootfs_usb=setenv rootfs root=/dev/sda1 rootwait\0" \
	"kernel=undef\0" \
	".kernel_tftp=setenv kernel tftpboot . ${bootfile}\0" \
	".kernel_nfs=setenv kernel nfs . ${serverip}:${rootpath}/${bootfile}\0" \
	".kernel_mmc=setenv kernel mmc rescan\\\\; load mmc ${mmcdev} . ${bootfile}\0" \
	".kernel_usb=setenv kernel usb start\\\\; load usb 0 . ${bootfile}\0" \
	"fdt=undef\0" \
	"fdtaddr=63000000\0" \
	".fdt_none=setenv fdt bootm\0" \
	".fdt_tftp=setenv fdt tftpboot ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT \
	".fdt_nfs=setenv fdt nfs ${fdtaddr} ${serverip}:${rootpath}/${bootfdt}" BOOT_WITH_FDT \
	".fdt_mmc=setenv fdt mmc rescan\\\\; load mmc ${mmcdev} ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT \
	".fdt_usb=setenv fdt usb start\\\\; load usb 0 ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT \
	"auxcore=undef\0" \
	"auxcoreaddr=1ffd0000\0" \
	".auxcore_tftp=setenv auxcore tftpboot . ${bootauxfile}\\\\; cp.b . ${auxcoreaddr} \\\\${filesize}\\\\; bootaux ${auxcoreaddr}\0" \
	".auxcore_mmc=setenv auxcore mmc rescan\\\\; load mmc 0 . ${bootauxfile}\\\\; cp.b . ${auxcoreaddr} \\\\${filesize}\\\\; bootaux ${auxcoreaddr}\0" \
	".auxcore_usb=setenv auxcore mmc start\\\\; load usb 0 . ${bootauxfile}\\\\; cp.b . ${auxcoreaddr} \\\\${filesize}\\\\; bootaux ${auxcoreaddr}\0" \
	".auxcore_none=setenv auxcore \\\\;\0" \
	"mode=undef\0" \
	".mode_rw=setenv mode rw\0" \
	".mode_ro=setenv mode ro\0" \
	"netdev=usb0\0" \
	"init=undef\0" \
	".init_init=setenv init\0" \
	".init_linuxrc=setenv init init=linuxrc\0" \
	"sercon=undef\0" \
	"installcheck=undef\0" \
	"updatecheck=undef\0" \
	"recovercheck=undef\0" \
	"platform=undef\0" \
	"arch=fsimx7ulp\0" \
	"bootfdt=undef\0" \
	"bootauxfile=undef\0" \
	"usbnet_devaddr=00:05:51:00:00:01\0" \
	"usbnet_hostaddr=00:05:51:00:00:02\0" \
	"cdc_connect_timeout=30\0" \
	FSBOOTDELAY \
	"fdt_high=ffffffff\0" \
	"set_bootfdt=setenv bootfdt ${platform}.dtb\0" \
	"set_bootargs=setenv bootargs ${console} ${login} ${network} ${rootfs} ${mode} ${init} ${extra}\0"

/*****************************************************************************
 * DFU (USB Device Firmware Update, requires USB device support)
 *****************************************************************************/
/* ###TODO### */


/*****************************************************************************
 * Tools
 *****************************************************************************/


/*****************************************************************************
 * Libraries
 *****************************************************************************/
/*#define USE_PRIVATE_LIBGCC*/


#endif	/* __CONFIG_H */
