/*
 * Copyright (C) 2018 F&S Elektronik Systeme GmbH
 *
 * Configuration settings for all F&S boards based on Freescale Vybrid. These
 * are armStoneA5, PicoCOMA5, NetDCUA5, CUBEA5, AGATEWAY and HGATEWAY.
 *
 * Activate with one of the following targets:
 *   make fsvybrid_config       Configure for Vybrid boards
 *   make fsvybrid              Configure for Vybrid boards and build
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * The following addresses are given as offsets of the device.
 *
 * NAND flash layout with separate Kernel/FDT MTD partition 
 * -------------------------------------------------------------------------
 * 0x0000_0000 - 0x0001_FFFF: NBoot: NBoot image, primary copy (128KB)
 * 0x0002_0000 - 0x0003_FFFF: NBoot: NBoot image, secondary copy (128KB)
 * 0x0004_0000 - 0x000F_FFFF: UserDef: User defined data (768KB)
 * 0x0010_0000 - 0x0013_FFFF: Refresh: Swap blocks for refreshing (256KB)
 * 0x0014_0000 - 0x001B_FFFF: UBoot: U-Boot image (512KB)
 * 0x001C_0000 - 0x001F_FFFF: UBootEnv: U-Boot environment (256KB)
 * 0x0020_0000 - 0x005F_FFFF: Kernel: Linux Kernel zImage (4MB)
 * 0x0060_0000 -         END: TargetFS: Root filesystem (Size - 6MB)
 *
 * NAND flash layout with UBI only, Kernel/FDT in rootfs or kernel/FDT volume
 * -------------------------------------------------------------------------
 * 0x0000_0000 - 0x0001_FFFF: NBoot: NBoot image, primary copy (128KB)
 * 0x0002_0000 - 0x0003_FFFF: NBoot: NBoot image, secondary copy (128KB)
 * 0x0004_0000 - 0x000F_FFFF: UserDef: User defined data (768KB)
 * 0x0010_0000 - 0x0013_FFFF: Refresh: Swap blocks for refreshing (256KB)
 * 0x0014_0000 - 0x001B_FFFF: UBoot: U-Boot image (512KB)
 * 0x001C_0000 - 0x001F_FFFF: UBootEnv: U-Boot environment (256KB)
 * 0x0020_0000 -         END: TargetFS: Root filesystem (Size - 2MB)
 *
 * END: 0x07FF_FFFF for 128MB, 0x0FFF_FFFF for 256MB, 0x3FFF_FFFF for 1GB
 *
 * Remark:
 * Block size is 128KB. All partition sizes have been chosen to allow for at
 * least one bad block in addition to the required size of the partition. E.g.
 * UBoot is 384KB, but the UBoot partition is 512KB to allow for one bad block
 * (128KB) in this memory region.
 *
 * RAM layout (RAM starts at 0x80000000)
 * -------------------------------------------------------------------------
 * 0x0000_0000 - 0x0000_00FF: Free RAM
 * 0x0000_0100 - 0x0000_07FF: bi_boot_params (ATAGs) (not used if FDT active)
 * 0x0000_1000 - 0x0000_105F: NBoot Args
 * 0x0000_1060 - 0x0000_7FFF: Free RAM
 * 0x0000_8000 - 0x007F_FFFF: Linux BSS (decompressed kernel)
 * 0x0100_0000 - 0x01FF_FFFF: Linux zImage
 * 0x0200_0000 - 0x07FF_FFFF: FDT + Free RAM + U-Boot (if 128MB)
 * 0x0200_0000 - 0x0FFF_FFFF: FDT + Free RAM + U-Boot (if 256MB)
 * 0x0200_0000 - 0x1FFF_FFFF: FDT + Free RAM + U-Boot (if 512MB)
 *
 * NBoot loads U-Boot to a rather low RAM address. Then U-Boot computes its
 * final size and relocates itself to the end of RAM.
 *
 * Memory layout within U-Boot (from top to bottom, starting at
 * RAM-Top = CONFIG_SYS_SDRAM_BASE + gd->ram_size)
 *
 * Addr          Size                      Comment
 * -------------------------------------------------------------------------
 * RAM-Top       CONFIG_SYS_MEM_TOP_HIDE   Hidden memory (unused)
 *               LOGBUFF_RESERVE           Linux kernel logbuffer (unused)
 *               env_get("pram") (in KB)   Protected RAM set in env (unused)
 * gd->tlb_addr  16KB (16KB aligned)       MMU page tables (TLB)
 * gd->fb_base   lcd_setmen()              LCD framebuffer (unused?)
 *               gd->monlen (4KB aligned)  U-boot code, data and bss
 *               TOTAL_MALLOC_LEN          malloc heap
 * bd            sizeof(bd_t)              Board info struct
 * gd->irq_sp    sizeof(gd_t)              Global data
 *               ARM_STACKSIZE_IRQ         IRQ stack
 *               ARM_STACKSIZE_FIQ         FIQ stack
 *               12 (8-byte aligned)       Abort-stack
 *
 * Remark: TOTAL_MALLOC_LEN depends on CONFIG_SYS_MALLOC_LEN and CONFIG_ENV_SIZE
 */

#ifndef __FSVYBRID_CONFIG_H
#define __FSVYBRID_CONFIG_H

/************************************************************************
 * High Level Configuration Options
 ************************************************************************/
#define CONFIG_VYBRID			/* ### TODO: switch to CONFIG_VF610 */

#include <asm/arch/vybrid-regs.h>	/* IRAM_BASE_ADDR, IRAM_SIZE */


/************************************************************************
 * Memory Layout
 ************************************************************************/
/* Physical addresses of DDR and CPU-internal SRAM */
#define CFG_SYS_SDRAM_BASE	0x80000000

/* Vybrid has min. 256KB internal SRAM, mapped from 0x3F000000-0x3F03FFFF */
#define CFG_SYS_INIT_RAM_ADDR	(IRAM_BASE_ADDR)
#define CFG_SYS_INIT_RAM_SIZE	(IRAM_SIZE)

/* Init value for stack pointer, set at end of internal SRAM, keep room for
   global data behind stack. */
#define CFG_SYS_INIT_SP_OFFSET (CFG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)

/* Allocate 2048KB protected RAM at end of RAM (Framebuffers, etc.) */
#define CFG_PRAM		2048

/* Memory test checks all RAM before U-Boot (i.e. leaves last MB with U-Boot
   untested) ### If not set, test from beginning of RAM to before stack. */
#if 0
#define CONFIG_SYS_MEMTEST_START CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END	(CONFIG_SYS_SDRAM_BASE + OUR_UBOOT_OFFS)
#endif

/************************************************************************
 * Clock Settings and Timers
 ************************************************************************/

/* Timer */
#define FTM_BASE_ADDR		FTM0_BASE_ADDR


/************************************************************************
 * GPIO
 ************************************************************************/
#define CONFIG_FSVYBRID_GPIO


/************************************************************************
 * OTP Memory (Fuses)
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Serial Console (UART)
 ************************************************************************/
#define CFG_MXC_UART_BASE UART2_BASE	/* Default UART port; however we
					   always take the port from NBoot */


/************************************************************************
 * I2C
 ************************************************************************/
/* No I2C used in U-Boot on F&S Vybrid boards */


/************************************************************************
 * LEDs
 ************************************************************************/


/************************************************************************
 * PMIC
 ************************************************************************/
/* No PMIC on F&S Vybrid boards */


/************************************************************************
 * Real Time Clock (RTC)
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Ethernet
 ************************************************************************/

#if 0 //### Set in defconfig when device tree support for fsvybrid is available
#undef CONFIG_ID_EEPROM			/* No EEPROM for ethernet MAC */
#endif //###


/************************************************************************
 * USB Host
 ************************************************************************/
/* Use USB1 as host */
//#define CONFIG_USB_MAX_CONTROLLER_COUNT 1


/************************************************************************
 * USB Device
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Keyboard
 ************************************************************************/


/************************************************************************
 * SD/MMC Card
 ************************************************************************/
#define CFG_SYS_FSL_ESDHC_ADDR 0	/* Not used */


/************************************************************************
 * EMMC
 ************************************************************************/
/* No eMMC on F&S Vybrid boards */


/************************************************************************
 * NOR Flash
 ************************************************************************/
/* No NOR flash on F&S Vybrid boards */


/************************************************************************
 * SPI Flash
 ************************************************************************/
/* No QSPI flash on F&S Vybrid boards */


/************************************************************************
 * NAND Flash
 ************************************************************************/
/* To avoid that NBoot is erased inadvertently, we define a skip region in the
   first NAND device that can not be written and always reads as 0xFF. However
   if value CONFIG_SYS_MAX_NAND_DEVICE is set to 2, the NBoot region is shown
   as a second NAND device with just that size. This makes it easier to have a
   different ECC strategy and software write protection for NBoot. */
#if 1
//#define CONFIG_SYS_MAX_NAND_DEVICE	1
#else
//#define CONFIG_SYS_MAX_NAND_DEVICE	2
#endif

/* Our NAND layout has a continuous set of OOB data, so we only need one
   oobfree entry (plus one empty entry to mark the end of the list). And when
   using NAND flash with 2K pages (written in a single chunk), Vybrid has at
   most 60 ECC bytes: 1 chunk, ECC32, GF15: 1*32*15 bits = 60 bytes. So we can
   also set CONFIG_SYS_NAND_MAX_ECCPOS to >=60. By setting the two values
   below, we can reduce the size of struct nand_ecclayout considerably from
   2824 bytes to 280 bytes (see include/linux/mtd/mtd.h). Please note that
   these settings have to be modified if smaller chunks or NAND flashes with
   larger pages are used. But then we have to modify the driver code anyway. */


/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/


/************************************************************************
 * Command Definition
 ************************************************************************/


/************************************************************************
 * Display Commands (LCD)
 ************************************************************************/
#if 0					/* ### TODO */
#define CONFIG_CMD_LCD			/* Support lcd settings command */
#define CONFIG_CMD_WIN			/* Window layers, alpha blending */
#define CONFIG_CMD_CMAP			/* Support CLUT pixel formats */
#define CONFIG_CMD_DRAW			/* Support draw command */
#define CONFIG_CMD_ADRAW		/* Support alpha draw commands */
#define CONFIG_CMD_BMINFO		/* Provide bminfo command */
#define CONFIG_XLCD_PNG			/* Support for PNG bitmaps */
#define CONFIG_XLCD_BMP			/* Support for BMP bitmaps */
#define CONFIG_XLCD_JPG			/* Support for JPG bitmaps */
#define CONFIG_XLCD_EXPR		/* Allow expressions in coordinates */
#define CONFIG_XLCD_CONSOLE		/* Support console on LCD */
#define CONFIG_XLCD_CONSOLE_MULTI	/* Define a console on each window */
#define CONFIG_XLCD_FBSIZE 0x00100000	/* 1 MB default framebuffer pool */
#define CONFIG_S3C64XX_XLCD		/* Use S3C64XX lcd driver */
#define CONFIG_S3C64XX_XLCD_PWM 1	/* Use PWM1 for backlight */

/* Supported draw commands (see inlcude/cmd_xlcd.h) */
#define CONFIG_XLCD_DRAW \
	(XLCD_DRAW_PIXEL | XLCD_DRAW_LINE | XLCD_DRAW_RECT	\
	 | /*XLCD_DRAW_CIRC | XLCD_DRAW_TURTLE |*/ XLCD_DRAW_FILL	\
	 | XLCD_DRAW_TEXT | XLCD_DRAW_BITMAP | XLCD_DRAW_PROG	\
	 | XLCD_DRAW_TEST)

/* Supported test images (see include/cmd_xlcd.h) */
#define CONFIG_XLCD_TEST \
	(XLCD_TEST_GRID /*| XLCD_TEST_COLORS | XLCD_TEST_D2B | XLCD_TEST_GRAD*/)
#endif


/************************************************************************
 * Network Options
 ************************************************************************/
#if 0 //### Set in defconfig when device tree support for fsvybrid is available
#define CONFIG_BOOTP_DNS2
#define CONFIG_BOOTP_SEND_HOSTNAME
#define CONFIG_NET_RETRY_COUNT	5
#define CONFIG_ARP_TIMEOUT	2000UL
#endif //###

/************************************************************************
 * Filesystem Support
 ************************************************************************/


/************************************************************************
 * Generic MTD Settings
 ************************************************************************/

/* Define MTD partition info */
#if CONFIG_SYS_MAX_NAND_DEVICE > 1
#define MTDIDS_DEFAULT		"nand0=fsnand0,nand1=fsnand1"
#define MTDPART_DEFAULT		"nand0,0"
#define MTDPARTS_PART1		"fsnand1:256k(NBoot)ro\\\\;fsnand0:768k@256k(UserDef)"
#else
#define MTDIDS_DEFAULT		"nand0=NAND"
#define MTDPART_DEFAULT		"nand0,1"
#define MTDPARTS_PART1		"NAND:256k(NBoot)ro,768k(UserDef)"
#endif
#define MTDPARTS_PART2		"256k(Refresh)ro,512k(UBoot)ro,256k(UBootEnv)ro"
#define MTDPARTS_PART3		"4m(Kernel)ro,1792k(FDT)ro"
#define MTDPARTS_PART4		"-(TargetFS)"
#define MTDPARTS_DEFAULT	"mtdparts=" MTDPARTS_PART1 "," MTDPARTS_PART2 "," MTDPARTS_PART3 "," MTDPARTS_PART4
#define MTDPARTS_STD		"setenv mtdparts " MTDPARTS_DEFAULT
#define MTDPARTS_UBIONLY	"setenv mtdparts mtdparts=" MTDPARTS_PART1 "," MTDPARTS_PART2 "," MTDPARTS_PART4


/************************************************************************
 * Environment
 ************************************************************************/
/*
 * Environment size and location are now set in the defconfig. The environment
 * is held in the heap, so keep the real size small to not waste malloc space.
 * Use two blocks (0x40000, 256KB) for CONFIG_ENV_NAND_RANGE to have one spare
 * block in case of a bad first block. See also NAND layout above. If we ever
 * change this layout, we should also make room for a second environment and
 * activate CONFIG_SYS_REDUNDAND_ENVIRONMENT.
 */

#if 0 //### Set in defconfig when device tree support for fsvybrid is available
//####define CONFIG_ETHPRIME		"FEC0"
//####define CONFIG_NETMASK		255.255.255.0
//####define CONFIG_IPADDR		10.0.0.252
//####define CONFIG_SERVERIP		10.0.0.122
//####define CONFIG_GATEWAYIP	10.0.0.5
//####define CONFIG_BOOTFILE		"zImage"
//####define CONFIG_ROOTPATH		"/rootfs"
#endif //###

/* Add some variables that are not predefined in U-Boot. All entries with
   content "undef" will be updated with a board-specific value in
   board_late_init().

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

#ifdef CONFIG_CMD_UBI
#ifdef CONFIG_CMD_UBIFS
#define EXTRA_UBIFS \
	".kernel_ubifs=setenv kernel ubi part TargetFS\\\\; ubifsmount ubi0:rootfs\\\\; ubifsload . /boot/${bootfile}\0" \
	".fdt_ubifs=setenv fdt ubi part TargetFS\\\\; ubifsmount ubi0:rootfs\\\\; ubifsload ${fdtaddr} /boot/${bootfdt}" BOOT_WITH_FDT
#else
#define EXTRA_UBIFS
#endif
#define EXTRA_UBI EXTRA_UBIFS \
	".mtdparts_ubionly=" MTDPARTS_UBIONLY "\0" \
	".rootfs_ubifs=setenv rootfs rootfstype=ubifs ubi.mtd=TargetFS root=ubi0:rootfs\0" \
	".kernel_ubi=setenv kernel ubi part TargetFS\\\\; ubi read . kernel\0" \
	".fdt_ubi=setenv fdt ubi part TargetFS\\\\; ubi read ${fdtaddr} fdt" BOOT_WITH_FDT \
	".ubivol_std=ubi part TargetFS; ubi create rootfs\0" \
	".ubivol_ubi=ubi part TargetFS; ubi create kernel 400000 s; ubi create rootfs\0"
#else
#define EXTRA_UBI
#endif

#ifdef CONFIG_BOOTDELAY
#define FSBOOTDELAY
#else
#define FSBOOTDELAY "bootdelay=undef\0"
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"console=undef\0" \
	".console_none=setenv console\0" \
	".console_serial=setenv console console=${sercon},${baudrate}\0" \
	".console_display=setenv console console=tty1\0" \
	"login=undef\0" \
	".login_none=setenv login login_tty=null\0" \
	".login_serial=setenv login login_tty=${sercon},${baudrate}\0" \
	".login_display=setenv login login_tty=tty1\0" \
	"mtdids=undef\0" \
	"mtdparts=undef\0" \
	".mtdparts_std=" MTDPARTS_STD "\0" \
	".network_off=setenv network\0" \
	".network_on=setenv network ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}:${netdev}\0" \
	".network_dhcp=setenv network ip=dhcp\0" \
	"rootfs=undef\0" \
	".rootfs_nfs=setenv rootfs root=/dev/nfs nfsroot=${serverip}:${rootpath}\0" \
	".rootfs_mmc=setenv rootfs root=/dev/mmcblk0p1 rootwait\0" \
	".rootfs_usb=setenv rootfs root=/dev/sda1 rootwait\0" \
	"kernel=undef\0" \
	".kernel_nand=setenv kernel nboot Kernel\0" \
	".kernel_tftp=setenv kernel tftpboot . ${bootfile}\0" \
	".kernel_nfs=setenv kernel nfs . ${serverip}:${rootpath}/${bootfile}\0" \
	".kernel_mmc=setenv kernel mmc rescan\\\\; load mmc 0 . ${bootfile}\0" \
	".kernel_usb=setenv kernel usb start\\\\; load usb 0 . ${bootfile}\0" \
	"fdt=undef\0" \
	"fdtaddr=82000000\0" \
	".fdt_none=setenv fdt bootm\0" \
	".fdt_nand=setenv fdt nand read ${fdtaddr} FDT" BOOT_WITH_FDT \
	".fdt_tftp=setenv fdt tftpboot ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT \
	".fdt_nfs=setenv fdt nfs ${fdtaddr} ${serverip}:${rootpath}/${bootfdt}" BOOT_WITH_FDT \
	".fdt_mmc=setenv fdt mmc rescan\\\\; load mmc 0 ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT \
	".fdt_usb=setenv fdt usb start\\\\; load usb 0 ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT \
	EXTRA_UBI \
	"mode=undef\0" \
	".mode_rw=setenv mode rw\0" \
	".mode_ro=setenv mode ro\0" \
	"netdev=eth0\0" \
	"init=undef\0" \
	".init_init=setenv init\0" \
	".init_linuxrc=setenv init init=linuxrc\0" \
	"sercon=undef\0" \
	"installcheck=undef\0" \
	"updatecheck=undef\0" \
	"recovercheck=undef\0" \
	"platform=undef\0" \
	"arch=fsvybrid\0" \
	"bootfdt=undef\0" \
	FSBOOTDELAY \
	"set_bootfdt=setenv bootfdt ${platform}.dtb\0" \
	"set_bootargs=setenv bootargs ${console} ${login} ${mtdparts} ${network} ${rootfs} ${mode} ${init} ${extra}\0"


/************************************************************************
 * DFU (USB Device Firmware Update, requires USB device support)
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Tools
 ************************************************************************/


/************************************************************************
 * Libraries
 ************************************************************************/

#endif /* !__FSVYBRID_CONFIG_H */
