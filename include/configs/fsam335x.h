/*
 * Copyright (C) 2015 F&S Elektronik Systeme GmbH
 *
 * Configuration settings for all F&S boards based on TI AM335x. This is
 * the PicoCOMA8.

 * Activate with one of the following targets:
 *   make fsam335x_config       Configure for AM335x boards
 *   make fsam335x              Configure for AM335x boards and build
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * The following addresses are given as offsets of the device.
 *
 * NAND flash layout with separate Kernel MTD partition 
 * -------------------------------------------------------------------------
 * 0x0000_0000 - 0x0001_FFFF: NBoot: NBoot image, primary copy (128KB)
 * 0x0002_0000 - 0x0003_FFFF: NBoot: NBoot image, secondary copy (128KB)
 * 0x0004_0000 - 0x000F_FFFF: UserDef: User defined data (768KB)
 * 0x0010_0000 - 0x0013_FFFF: Refresh: Swap blocks for refreshing (256KB)
 * 0x0014_0000 - 0x001B_FFFF: UBoot: U-Boot image (512KB)
 * 0x001C_0000 - 0x001F_FFFF: UBootEnv: U-Boot environment (256KB)
 * 0x0020_0000 - 0x005F_FFFF: Kernel: Linux Kernel uImage (4MB)
 * 0x0060_0000 -         END: TargetFS: Root filesystem (Size - 6MB)
 *
 * NAND flash layout with UBI only, Kernel in rootfs or kernel volume
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
 * 0x0000_0100 - 0x0000_07FF: bi_boot_params (ATAGs)
 * 0x0000_1000 - 0x0000_105F: NBoot Args
 * 0x0000_1060 - 0x0000_7FFF: Free RAM
 * 0x0000_8000 - 0x007F_FFFF: Linux zImage
 * 0x0080_0000 - 0x00FF_FFFF: Linux BSS (decompressed kernel)
 * 0x0100_0000 - 0x07FF_FFFF: Free RAM + U-Boot (if 128MB)
 * 0x0100_0000 - 0x0FFF_FFFF: Free RAM + U-Boot (if 256MB)
 * 0x0100_0000 - 0x1FFF_FFFF: Free RAM + U-Boot (if 512MB)
 *
 * NBoot loads U-Boot to a rather low RAM address. Then U-Boot computes its
 * final size and relocates itself to the end of RAM. This new ARM specific
 * loader scheme was introduced in u-boot-2010.12. It only requires to set
 * gd->ram_base correctly and to privide a board-specific function dram_init()
 * that sets gd->ram_size to the actually available RAM. For more details see
 * arch/arm/lib/board.c.
 *
 * Memory layout within U-Boot (from top to bottom, starting at
 * RAM-Top = gd->ram_base + gd->ram_size)
 *
 * Addr          Size                      Comment
 * -------------------------------------------------------------------------
 * RAM-Top       CONFIG_SYS_MEM_TOP_HIDE   Hidden memory (unused)
 *               LOGBUFF_RESERVE           Linux kernel logbuffer (unused)
 *               getenv("pram") (in KB)    Protected RAM set in env (unused)
 * gd->tlb_addr  16KB (16KB aligned)       MMU page tables (TLB)
 * gd->fb_base   lcd_setmen()              LCD framebuffer (unused?)
 *               gd->monlen (4KB aligned)  U-boot code, data and bss
 *               TOTAL_MALLOC_LEN          malloc heap
 * bd            sizeof(bd_t)              Board info struct
 * gd->irq_sp    sizeof(gd_t)              Global data
 *               CONFIG_STACKSIZE_IRQ      IRQ stack
 *               CONFIG_STACKSIZE_FIQ      FIQ stack
 *               12 (8-byte aligned)       Abort-stack
 *
 * Remark: TOTAL_MALLOC_LEN depends on CONFIG_SYS_MALLOC_LEN and CONFIG_ENV_SIZE
 *
 */

#ifndef __FSAM335X_CONFIG_H
#define __FSAM335X_CONFIG_H

/************************************************************************
 * High Level Configuration Options
 ************************************************************************/
#define CONFIG_IDENT_STRING " for F&S"	/* We are on an F&S board */

/* CPU, family and board defines */
#define CONFIG_AM33XX			/* TI AM33xx Sitara CPU... */
#define CONFIG_OMAP			/* ...which is in fact an OMAP... */
#define CONFIG_OMAP_COMMON
#define CONFIG_FSAM33XX			/* ...on and F&S AM335x board */
#undef CONFIG_MP			/* No multi processor support */

#include <asm/arch/omap.h>		/* NON_SECURE_SRAM info */
#define CONFIG_SYS_CACHELINE_SIZE	64

//###FIXME### Can we drop CONFIG_ARCH_CPU_INIT?
#define CONFIG_ARCH_CPU_INIT		/* Save OMAP boot params (unused) */
#define CONFIG_SKIP_LOWLEVEL_INIT	/* CPU and RAM setup done in NBoot */
#undef CONFIG_BOARD_EARLY_INIT_F	/* No early board specific stuff */
#define CONFIG_BOARD_LATE_INIT		/* Init board-specific environment */
#define CONFIG_DISPLAY_CPUINFO		/* Show CPU type and speed */
#define CONFIG_DISPLAY_BOARDINFO	/* Show board information */
#define CONFIG_ZERO_BOOTDELAY_CHECK	/* Allow entering U-Boot even if boot
					   delay is zero */
#undef CONFIG_USE_IRQ			/* No blinking LEDs yet */
#define CONFIG_SYS_LONGHELP		/* Undef to save memory */
#undef CONFIG_LOGBUFFER			/* No support for log files */
#undef CONFIG_OF_LIBFDT			/* No device tree (fdt) support yet */

/* The load address of U-Boot is now independent from the size. Just load it
   at some rather low address in RAM. It will relocate itself to the end of
   RAM automatically when executed. */
#define CONFIG_SYS_TEXT_BASE 0x80100000	/* Where NBoot loads U-Boot */
#define CONFIG_UBOOTNB0_SIZE	384	/* Size of uboot.nb0 (in kB) */
#define CONFIG_SYS_THUMB_BUILD		/* Build U-Boot in THUMB mode */

/* For the default load address, use an offset of 8MB. The final kernel (after
   decompressing the zImage) must be at offset 0x8000. But if we load the
   zImage there, the loader code will move it away to make room for the
   uncompressed image at this position. So we'll load it directly to a higher
   address to avoid this additional copying. */
#define CONFIG_SYS_LOAD_OFFS 0x00800000


/************************************************************************
 * Memory Layout
 ************************************************************************/
/* Physical addresses of DDR and CPU-internal SRAM */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_0		0x80000000	    /* DDR */
//####define PHYS_SDRAM_0_SIZE	(256 * 1024 * 1024) /* varying */

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_0
#define CONFIG_MAX_RAM_BANK_SIZE	(1024 << 20)	/* 1GB */

/* AM335X has ~128KB internal SRAM, mapped from 0x402F0400-0x40310000 */
#define CONFIG_SYS_INIT_RAM_ADDR	(NON_SECURE_SRAM_START)
#define CONFIG_SYS_INIT_RAM_SIZE	(NON_SECURE_SRAM_END - NON_SECURE_SRAM_START)

/* Init value for stack pointer, set at end of internal SRAM, keep room for
   global data behind stack. */
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* Size of malloc() pool (heap). Command "ubi part" needs quite a large heap
   if the source MTD partition is large. The size should be large enough to
   also contain a copy of the environment. */
#define CONFIG_SYS_MALLOC_LEN	(2 * 1024 * 1024)

/* Allocate 2048KB protected RAM at end of RAM (Framebuffers, etc.) */
#define CONFIG_PRAM		2048

/* Alignment mask for MMU pagetable: 16kB */
#define CONFIG_SYS_TLB_ALIGN	0xFFFFC000

/* The final stack sizes are set up in board.c using the settings below */
#define CONFIG_SYS_STACK_SIZE	(128*1024)
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)
#define CONFIG_STACKSIZE_FIQ	(128)
#endif

/* Memory test checks all RAM before U-Boot (i.e. leaves last MB with U-Boot
   untested) ### If not set, test from beginning of RAM to before stack. */
//####define CONFIG_SYS_MEMTEST_START CONFIG_SYS_SDRAM_BASE
//####define CONFIG_SYS_MEMTEST_END	(CONFIG_SYS_SDRAM_BASE + OUR_UBOOT_OFFS)


/************************************************************************
 * Clock Settings and Timers
 ************************************************************************/
/* Basic input clocks */
#define V_OSCK			24000000
#define V_SCLK			(V_OSCK)

/* Timer */
#define CONFIG_SYS_TIMERBASE	0x48040000	/* Use Timer2 */
#define CONFIG_SYS_PTV		2		/* Divisor: 2^(PTV+1) => 8 */

#define CONFIG_SYS_HZ		1000


/************************************************************************
 * GPIO
 ************************************************************************/
#define CONFIG_OMAP_GPIO


/************************************************************************
 * OTP Memory (Fuses)
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Serial Console (UART)
 ************************************************************************/
#define CONFIG_SYS_NS16550		/* Use NS16550 uart driver */
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)
#define CONFIG_SYS_NS16550_CLK	48000000
#define CONFIG_SYS_NS16550_COM1	0x44e09000	/* Base EVM has UART0 */
#define CONFIG_SYS_NS16550_COM2	0x48022000	/* UART1 */
#define CONFIG_SYS_NS16550_COM3	0x48024000	/* UART2 */
#define CONFIG_SYS_NS16550_COM4	0x481a6000	/* UART3 */
#define CONFIG_SYS_NS16550_COM5	0x481a8000	/* UART4 */
#define CONFIG_SYS_NS16550_COM6	0x481aa000	/* UART5 */

#define CONFIG_SYS_UART_PORT	0	/* Default UART port; however we
					   always take the port from NBoot */
#define CONFIG_CONS_INDEX	(CONFIG_SYS_UART_PORT + 1)
#undef CONFIG_CONSOLE_MUX		/* Just one console at a time */
#define CONFIG_SYS_SERCON_NAME "ttyO"	/* Base name for serial devices */
#define CONFIG_BAUDRATE		115200	/* Default baudrate */
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}


/************************************************************************
 * I2C
 ************************************************************************/
#if 0 //####
#define CONFIG_I2C
#define CONFIG_HARD_I2C
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		1
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_DRIVER_OMAP24XX_I2C
#define CONFIG_CMD_I2C
#define CONFIG_CMD_EEPROM
#define CONFIG_ENV_EEPROM_IS_ON_I2C
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x50	/* Main EEPROM */
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	2
#define CONFIG_SYS_I2C_MULTI_EEPROMS
#endif //0###


/************************************************************************
 * LEDs
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * PMIC
 ************************************************************************/
#if 0 //###
#define CONFIG_POWER_TPS65217
#define CONFIG_POWER_TPS65910
#endif //0###


/************************************************************************
 * Real Time Clock (RTC)
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Ethernet
 ************************************************************************/
#define CONFIG_DRIVER_TI_CPSW		/* Driver for IP block */

/* PHY */
#define CONFIG_PHY_GIGE
#define CONFIG_PHYLIB
#define CONFIG_PHY_SMSC
#define CONFIG_SYS_DISCOVER_PHY
#define CONFIG_SYS_FAULT_ECHO_LINK_DOWN

#undef CONFIG_ID_EEPROM			/* No EEPROM for ethernet MAC */


/************************************************************************
 * USB Host
 ************************************************************************/
/* Use USB1 as host */
#define CONFIG_ARCH_MISC_INIT		/* arch_misc_init() used for USB */
#define CONFIG_MUSB_HOST
#define CONFIG_USB_MUSB_AM35X
#define CONFIG_AM335X_USB1
#define CONFIG_AM335X_USB1_MODE MUSB_HOST
#define CONFIG_USB_MUSB_DSPS
#define CONFIG_MUSB_PIO_ONLY

#define CONFIG_USB_STORAGE


/************************************************************************
 * USB Device
 ************************************************************************/
#if 0 //###TODO###
#define CONFIG_SPL_MUSB_NEW_SUPPORT	/* USB gadget RNDIS */
#define CONFIG_AM335X_USB0
#define CONFIG_AM335X_USB0_MODE	MUSB_PERIPHERAL
#define CONFIG_ARCH_MISC_INIT
#define CONFIG_MUSB_GADGET
#define CONFIG_MUSB_PIO_ONLY
#define CONFIG_MUSB_DISABLE_BULK_COMBINE_SPLIT
#define CONFIG_USB_GADGET
#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_VBUS_DRAW	2
#define CONFIG_USB_ETHER
#define CONFIG_USB_ETH_RNDIS
#define CONFIG_USBNET_HOST_ADDR	"de:ad:be:af:00:00"

/* USB TI's IDs */
#define CONFIG_G_DNL_VENDOR_NUM 0x0403
#define CONFIG_G_DNL_PRODUCT_NUM 0xBD00
#define CONFIG_G_DNL_MANUFACTURER "Texas Instruments"
#endif //0###


/************************************************************************
 * Keyboard
 ************************************************************************/
#if 0 //###
#define CONFIG_USB_KEYBOARD
#define CONFIG_SYS_DEVICE_DEREGISTER	/* Required for CONFIG_USB_KEYBOARD */
#endif //0###


/************************************************************************
 * SD/MMC Card
 ************************************************************************/
#define CONFIG_MMC			/* SD/MMC support */
#define CONFIG_GENERIC_MMC		/* with the generic driver model */
#define CONFIG_OMAP_HSMMC		/* OMAP high speed MMC */


/************************************************************************
 * EMMC
 ************************************************************************/
#if 0 //###TODO###
#undef CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		1
#define CONFIG_SYS_MMC_ENV_PART		2
#define CONFIG_ENV_OFFSET		0x0
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#endif //0###


/************************************************************************
 * NOR Flash
 ************************************************************************/
/* No NOR flash on F&S AM335X boards */
#define CONFIG_SYS_NO_FLASH


/************************************************************************
 * SPI Flash
 ************************************************************************/
#if 0 //####
#define CONFIG_SPI
#define CONFIG_OMAP3_SPI
#define CONFIG_CMD_SPI

/* SPI flash. */
#define CONFIG_SPL_SPI_SUPPORT
#define CONFIG_SPL_SPI_FLASH_SUPPORT
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SPL_SPI_BUS		0
#define CONFIG_SPL_SPI_CS		0
#define CONFIG_SYS_SPI_U_BOOT_OFFS	0x20000
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SF_DEFAULT_SPEED		24000000
#endif //0###


/************************************************************************
 * NAND Flash
 ************************************************************************/
/* Use TI implementation of NAND driver */
#define CONFIG_NAND_OMAP_GPMC
#define CONFIG_NAND_OMAP_ELM

#define CONFIG_SYS_NAND_PAGE_SIZE		2048
#define CONFIG_SYS_NAND_OOBSIZE			64
#define CONFIG_SYS_NAND_BLOCK_SIZE		(128*1024)
#define CONFIG_SPL_NAND_DEVICE_WIDTH		8
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_PAGE_COUNT		(CONFIG_SYS_NAND_BLOCK_SIZE / \
						 CONFIG_SYS_NAND_PAGE_SIZE)
#define CONFIG_SYS_NAND_BASE			0x8000000
#define CONFIG_SYS_NAND_BAD_BLOCK_POS		NAND_LARGE_BADBLOCK_POS
#define CONFIG_SYS_NAND_ECCPOS		      { 2, 3, 4, 5, 6, 7, 8, 9, \
					       10, 11, 12, 13, 14, 15, 16, 17, \
					       18, 19, 20, 21, 22, 23, 24, 25, \
					       26, 27, 28, 29, 30, 31, 32, 33, \
					       34, 35, 36, 37, 38, 39, 40, 41, \
					       42, 43, 44, 45, 46, 47, 48, 49, \
					       50, 51, 52, 53, 54, 55, 56, 57, }
#define CONFIG_SYS_NAND_ECCSIZE			512
#define CONFIG_SYS_NAND_ECCBYTES		14
#define CONFIG_NAND_OMAP_ECCSCHEME		OMAP_ECC_BCH8_CODE_HW

/* Use our own initialization code */
//####define CONFIG_SYS_NAND_SELF_INIT

/* To avoid that NBoot is erased inadvertently, we define a skip region in the
   first NAND device that can not be written and always reads as 0xFF. However
   if value CONFIG_SYS_MAX_NAND_DEVICE is set to 2, the NBoot region is shown
   as a second NAND device with just that size. This makes it easier to have a
   different ECC strategy and software write protection for NBoot. */
#define CONFIG_SYS_MAX_NAND_DEVICE	1
//#define CONFIG_SYS_MAX_NAND_DEVICE	2

/* Chips per device; all chips must be the same type; if different types
   are necessary, they must be implemented as different NAND devices */
#define CONFIG_SYS_NAND_MAX_CHIPS	1

/* Define if you want to support nand chips that comply to ONFI spec */
#define CONFIG_SYS_NAND_ONFI_DETECTION

/* Actually perform block refresh if pages degrade too much. Use the given
   blocks in decreasing order. */
//####define CONFIG_NAND_REFRESH
//####define CONFIG_SYS_NAND_BACKUP_START_BLOCK	9
//####define CONFIG_SYS_NAND_BACKUP_END_BLOCK	8


/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/
#define CONFIG_SYS_HUSH_PARSER		/* Use "hush" command parser */
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

/* Allow editing (scroll between commands, etc.) */
#define CONFIG_CMDLINE_EDITING
#undef CONFIG_AUTO_COMPLETE

/* Input and print buffer sizes */
#define CONFIG_SYS_CBSIZE	512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	640	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE /* Boot Arg Buffer Size */


/************************************************************************
 * Command Definition
 ************************************************************************/
/* We don't include
     #include <config_cmd_default.h>
   but instead we define all the commands that we want ourselves. However
   <config_cmd_defaults.h> (note the s) is always included nonetheless. This
   always sets the following defines: CONFIG_CMD_BOOTM, CONFIG_CMD_CRC32,
   CONFIG_CMD_GO, CONFIG_CMD_EXPORTENV, CONFIG_CMD_IMPORTENV. */
#undef CONFIG_CMD_AMBAPP	/* No support to show AMBA plug&play devices */
#define CONFIG_CMD_ASKENV	/* Ask user for variable */
#define CONFIG_CMD_BDI		/* Board information (bdinfo) */
#undef CONFIG_CMD_BEDBUG	/* No PPC bedbug debugging support */
#undef CONFIG_CMD_BLINK		/* No support for blinking LEDs */
#undef CONFIG_CMD_BMP		/* No old BMP, use new display support */
#define CONFIG_CMD_BOOTD	/* Boot default target */
#undef CONFIG_CMD_BOOTLDR	/* No ldr support for blackfin */
#undef CONFIG_CMD_BOOTSTAGE	/* No bootstage command */
#define CONFIG_CMD_BOOTZ	/* Boot zImage */
#define CONFIG_CMD_CACHE	/* Switch cache on and off */
#undef CONFIG_CMD_CBFS		/* No support for coreboot filesystem */
#undef CONFIG_CMD_CDP		/* No support for CISCOs CDP network config */
#define CONFIG_CMD_CONSOLE	/* Console information (coninfo) */
#undef CONFIG_CMD_CPLBINFO	/* No display of PPC CPLB tables */
#undef CONFIG_CMD_CRAMFS	/* No support for CRAMFS filesystem */
#undef CONFIG_CMD_DATE		/* No date command */
#undef CONFIG_CMD_DFU		/* No support for device firmware update */ 
#define CONFIG_CMD_DHCP		/* Support TFTP boot after DHCP request */
#undef CONFIG_CMD_DIAG		/* No support for board selftest */
#undef CONFIG_CMD_DNS		/* No lookup of IP via a DNS name server */
#undef CONFIG_CMD_DTT		/* No digital thermometer and thermostat */
#define CONFIG_CMD_ECHO		/* Have echo command */
#define CONFIG_CMD_EDITENV	/* Allow editing of environment variables */
#undef CONFIG_CMD_EEPROM	/* No EEPROM support */
#undef CONFIG_CMD_ELF		/* No support to boot ELF images */
#define CONFIG_CMD_EXT2		/* Support for EXT2 commands */
#define CONFIG_CMD_EXT4		/* Support for EXT4 commands */
#define CONFIG_CMD_FAT		/* Support for FAT commands */
#undef CONFIG_CMD_FDC		/* No floppy disc controller */
#undef CONFIG_CMD_FDOS		/* No support for DOS from floppy disc */
#undef CONFIG_CMD_FITUPD	/* No update from FIT image */
#undef CONFIG_CMD_FLASH		/* No NOR flash (flinfo, erase, protect) */
#undef CONFIG_CMD_FPGA		/* No FPGA configuration support */
#define CONFIG_CMD_FS_GENERIC	/* Filesystem-independent ls and load */
#define CONFIG_CMD_GETTIME	/* Have gettime command */
#define CONFIG_CMD_GPIO		/* Support to set GPIO pins */
#undef CONFIG_CMD_GPT		/* No support for GPT partition tables */
#undef CONFIG_CMD_GREPENV	/* No support to search in environment */
#undef CONFIG_CMD_HASH		/* No hash command */
#undef CONFIG_CMD_HWFLOW	/* No switching of serial flow control */
#undef CONFIG_CMD_I2C		/* No I2C support */
#undef CONFIG_CMD_IDE		/* No IDE disk support */
#define CONFIG_CMD_IMI		/* Image information (iminfo) */
#undef CONFIG_CMD_IMLS		/* No support to list all found images */
#undef CONFIG_CMD_IMMAP		/* No support for PPC immap table */
#define CONFIG_CMD_INI		/* Support INI files to init environment */
#undef CONFIG_CMD_IO		/* No I/O space commands iod and iow */
#undef CONFIG_CMD_IRQ		/* No interrupt support */
#define CONFIG_CMD_ITEST	/* Integer (and string) test */
#undef CONFIG_CMD_JFFS2		/* No support for JFFS2 filesystem */
#undef CONFIG_CMD_LDRINFO	/* No ldr support for blackfin */
#define CONFIG_CMD_LED		/* LED support */
#undef CONFIG_CMD_LICENSE	/* No support to show GPL license */
#undef CONFIG_CMD_LOADB		/* No serial load of binaries (loadb) */
#undef CONFIG_CMD_LOADS		/* No serial load of s-records (loads) */
#undef CONFIG_CMD_MD5SUM	/* No support for md5sum checksums */
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#undef CONFIG_CMD_MFSL		/* No support for Microblaze FSL */
#define CONFIG_CMD_MII		/* Support for listing MDIO busses */
#define CONFIG_CMD_MISC		/* Miscellaneous commands (sleep) */
#define CONFIG_CMD_MMC		/* Support for SD/MMC cards */
#undef CONFIG_CMD_MMC_SPI	/* No access of MMC cards in SPI mode */
#undef CONFIG_CMD_MOVI		/* No support for MOVI NAND flash memories */
#define CONFIG_CMD_MTDPARTS	/* Support MTD partitions (mtdparts, chpart) */
#define CONFIG_CMD_NAND		/* Support for common NAND flash memories */
#define CONFIG_CMD_NET		/* Support BOOTP and TFTP (bootp, tftpboot) */
#define CONFIG_CMD_NFS		/* Support download via NFS */
#undef CONFIG_CMD_ONENAND	/* No support for ONENAND flash memories */
#undef CONFIG_CMD_OTP		/* No support for one-time-programmable mem */
#undef CONFIG_CMD_PART		/* No support for partition info */
#undef CONFIG_CMD_PCI		/* No PCI support */
#undef CONFIG_CMD_PCMCIA	/* No support for PCMCIA cards */
#define CONFIG_CMD_PING		/* Support ping command */
#undef CONFIG_CMD_PORTIO	/* No port commands (in, out) */
#undef CONFIG_CMD_PXE		/* No support for PXE files from pxelinux */
#undef CONFIG_CMD_RARP		/* No support for booting via RARP */
#define CONFIG_CMD_READ		/* Raw read from media without filesystem */
#undef CONFIG_CMD_REGINFO	/* No register support on ARM, only PPC */
#undef CONFIG_CMD_REISER	/* No support for reiserfs filesystem */
#define CONFIG_CMD_RUN		/* Run command in env variable */
#undef CONFIG_CMD_SATA		/* No support for SATA disks */
#define CONFIG_CMD_SAVEENV	/* Allow saving environment to NAND */
#undef CONFIG_CMD_SAVES		/* No support for serial uploads (saving) */
#undef CONFIG_CMD_SCSI		/* No support for SCSI disks */
#undef CONFIG_CMD_SDRAM		/* Support SDRAM chips via I2C */
#define CONFIG_CMD_SETEXPR	/* Set variable by evaluating an expression */
#undef CONFIG_CMD_SETGETDCR	/* No support for PPC DCR register */
#undef CONFIG_CMD_SF		/* No support for serial SPI flashs */
#undef CONFIG_CMD_SHA1SUM	/* No support for sha1sum checksums */
#undef CONFIG_CMD_SNTP		/* No synchronizing of RTC via network */
#undef CONFIG_CMD_SOUND		/* No sound command */
#define CONFIG_CMD_SOURCE	/* Source support (was autoscr)	*/
#undef CONFIG_CMD_SPI		/* No SPI support */
#undef CONFIG_CMD_SPIBOOTLDR	/* No ldr support over SPI for blackfin */
#undef CONFIG_CMD_SPL		/* No SPL support (kernel parameter images) */
#undef CONFIG_CMD_STRINGS	/* No support to show strings */
#undef CONFIG_CMD_TERMINAL	/* No terminal emulator */
#undef CONFIG_CMD_TFTPPUT	/* No sending of TFTP files (tftpput) */
#undef CONFIG_CMD_TFTPSRV	/* No acting as TFTP server (tftpsrv) */
#undef CONFIG_CMD_TIME		/* No support to time command execution */
#define CONFIG_CMD_TIMER	/* Support system timer access */
#undef CONFIG_CMD_TPM		/* No support for TPM */
#undef CONFIG_CMD_TSI148	/* No support for Turndra Tsi148 */
#define CONFIG_CMD_UBI		/* Support for unsorted block images (UBI) */
#define CONFIG_CMD_UBIFS	/* Support for UBIFS filesystem */
#undef CONFIG_CMD_UNIVERSE	/* No support for Turndra Universe */
#define CONFIG_CMD_UNZIP	/* Have unzip command */
#define CONFIG_CMD_UPDATE	/* Support automatic update/install */
#define CONFIG_CMD_USB		/* USB host support */
#undef CONFIG_CMD_XIMG		/* No support to load part of Multi Image */
#undef CONFIG_CMD_ZFS		/* No support for ZFS filesystem */
#undef CONFIG_CMD_ZIP		/* No support to zip memory region */


/************************************************************************
 * Display Commands (LCD)
 ************************************************************************/
/* ###TODO### */


/************************************************************************
 * Network Options
 ************************************************************************/
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_DNS		/* Configurable parts of CMD_DHCP */
#define CONFIG_BOOTP_DNS2
#define CONFIG_BOOTP_SEND_HOSTNAME
#define CONFIG_NET_RETRY_COUNT	10
#define CONFIG_ARP_TIMEOUT	200UL
#define CONFIG_MII			/* Required in net/eth.c */


/************************************************************************
 * Filesystem Support
 ************************************************************************/
/* FAT */
#define CONFIG_FS_FAT			/* Support FAT... */
#define CONFIG_SUPPORT_VFAT		/* ...with VFAT */
#define CONFIG_DOS_PARTITION

/* EXT4 */
#define CONFIG_FS_EXT4			/* Support EXT2/3/4 */

/* JFFS2 */
#define CONFIG_JFFS2_NAND		/* Support JFFS2 in NAND */

/* YAFFS */
#undef CONFIG_CMD_NAND_YAFFS		/* No nand read/write.yaffs */
#undef CONFIG_YAFFS2			/* No support for YAFFS2 commands */

/* UBI/UBIFS */
#define CONFIG_RBTREE			/* Required for UBI */
#define CONFIG_LZO			/* Required for UBI */


/************************************************************************
 * Generic MTD Settings
 ************************************************************************/
#define CONFIG_MTD_DEVICE		/* Create MTD device */
#define CONFIG_MTD_PARTITIONS		/* Required for UBI */

/* Define MTD partition info */
#if CONFIG_SYS_MAX_NAND_DEVICE > 1
#define MTDIDS_DEFAULT		"nand0=nand.0,nand1=nand.1"
#define MTDPART_DEFAULT		"nand0,0"
#define MTDPARTS_PART1		"nand.1:256k(NBoot)ro\\\\;nand.0:768k@256k(UserDef)"
#else
#define MTDIDS_DEFAULT		"nand0=nand.0"
#define MTDPART_DEFAULT		"nand0,1"
#define MTDPARTS_PART1		"nand.0:256k(NBoot)ro,768k(UserDef)"
#endif
#define MTDPARTS_PART2		"256k(Refresh)ro,512k(UBoot)ro,256k(UBootEnv)ro"
#define MTDPARTS_PART3		"4m(Kernel)ro"
#define MTDPARTS_PART4		"-(TargetFS)"
#define MTDPARTS_STD		"setenv mtdparts mtdparts=" MTDPARTS_PART1 "," MTDPARTS_PART2 "," MTDPARTS_PART3 "," MTDPARTS_PART4
#define MTDPARTS_UBIONLY	"setenv mtdparts mtdparts=" MTDPARTS_PART1 "," MTDPARTS_PART2 "," MTDPARTS_PART4


/************************************************************************
 * Environment
 ************************************************************************/
#define CONFIG_ENV_IS_IN_NAND		/* Environment is in NAND flash */
#define CONFIG_ENV_OVERWRITE		/* Allow overwriting serial/ethaddr */
#define CONFIG_SYS_CONSOLE_IS_IN_ENV	/* Console can be saved in env */

/* Environment settings for large blocks (128KB). The environment is held in
   the heap, so keep the real env size small to not waste malloc space. */
#define ENV_SIZE_DEF_LARGE   0x00004000	/* 16KB */
#define ENV_RANGE_DEF_LARGE  0x00040000 /* 2 blocks = 256KB */
#define ENV_OFFSET_DEF_LARGE 0x001C0000 /* See NAND layout above */

/* When saving the environment, we usually have a short period of time between
   erasing the NAND region and writing the new data where no valid environment
   is available. To avoid this time, we can save the environment alternatively
   to two different locations in the NAND flash. Then at least one of the
   environments is always valid. Currently we don't use this feature. */
//#define CONFIG_SYS_ENV_OFFSET_REDUND   0x001C0000

//#define CONFIG_ETHADDR_BASE	00:05:51:07:55:83
#define CONFIG_ETHADDR		00:05:51:07:55:83
#define CONFIG_ETHPRIME		"FEC0"
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_IPADDR		10.0.0.252
#define CONFIG_SERVERIP		10.0.0.122
#define CONFIG_GATEWAYIP	10.0.0.5
#define CONFIG_BOOTFILE		"zImage"
#define CONFIG_ROOTPATH		"/rootfs"
#define CONFIG_MODE		"ro"
#define CONFIG_BOOTDELAY	undef
#define CONFIG_PREBOOT
#define CONFIG_BOOTARGS		"undef"
#define CONFIG_BOOTCOMMAND	"run set_bootargs; run kernel; bootm"

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
#ifdef CONFIG_CMD_UBI
#ifdef CONFIG_CMD_UBIFS
#define EXTRA_UBIFS \
	".kernel_ubifs=setenv kernel ubi part TargetFS\\\\; ubifsmount rootfs\\\\; ubifsload . /boot/${bootfile}\0"
#else
#define EXTRA_UBIFS
#endif
#define EXTRA_UBI EXTRA_UBIFS \
	".mtdparts_ubionly=" MTDPARTS_UBIONLY "\0" \
	".rootfs_ubifs=setenv rootfs rootfstype=ubifs ubi.mtd=TargetFS root=ubi0:rootfs\0" \
	".kernel_ubi=setenv kernel ubi part TargetFS\\\\; ubi read . kernel\0" \
	".ubivol_std=ubi part TargetFS; ubi create rootfs\0" \
	".ubivol_ubi=ubi part TargetFS; ubi create kernel 400000 s; ubi create rootfs\0"
#else
#define EXTRA_UBI
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"console=undef\0" \
	".console_none=setenv console\0" \
	".console_serial=setenv console console=${sercon},${baudrate}\0" \
	".console_display=setenv console console=/dev/tty1\0" \
	"login=undef\0" \
	".login_none=setenv login login_tty=null\0" \
	".login_serial=setenv login login_tty=${sercon},${baudrate}\0" \
	".login_display=setenv login login_tty=/dev/tty1\0" \
	"mtdparts=undef\0" \
	".mtdparts_std=" MTDPARTS_STD "\0" \
	".network_off=setenv network\0"					\
	".network_on=setenv network ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}:${netdev}\0" \
	".network_dhcp=setenv network ip=dhcp\0" \
	"rootfs=undef\0" \
	".rootfs_nfs=setenv rootfs root=/dev/nfs nfsroot=${rootpath}\0" \
	".rootfs_mmc=setenv rootfs root=/dev/mmcblk0p1\0" \
	".rootfs_usb=setenv rootfs root=/dev/sda1\0" \
	"kernel=undef\0" \
	".kernel_nand=setenv kernel nboot Kernel\0" \
	".kernel_tftp=setenv kernel tftpboot . ${bootfile}\0" \
	".kernel_nfs=setenv kernel nfs . ${serverip}:${rootpath}/${bootfile}\0" \
	".kernel_mmc=setenv kernel mmc rescan\\\\; load mmc 0 . ${bootfile}\0" \
	".kernel_usb=setenv kernel usb start\\\\; load usb 0 . ${bootfile}\0" \
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
	"arch=fsam335x\0" \
	"set_bootargs=setenv bootargs ${console} ${login} ${mtdparts} ${network} ${rootfs} ${mode} ${init} ${extra}\0"


/************************************************************************
 * DFU (USB Device Firmware Update, requires USB device support)
 ************************************************************************/
#if 0 //###
#define CONFIG_DFU_FUNCTION
#define CONFIG_DFU_MMC
#define CONFIG_CMD_DFU
#define DFU_ALT_INFO_MMC \
	"boot part 0 1;" \
	"rootfs part 0 2;" \
	"MLO fat 0 1;" \
	"MLO.raw mmc 100 100;" \
	"u-boot.img.raw mmc 300 400;" \
	"spl-os-args.raw mmc 80 80;" \
	"spl-os-image.raw mmc 900 2000;" \
	"spl-os-args fat 0 1;" \
	"spl-os-image fat 0 1;" \
	"u-boot.img fat 0 1;" \
	"uEnv.txt fat 0 1"
#ifdef CONFIG_NAND
#define CONFIG_DFU_NAND

#ifdef DFU_ALT_INFO_NAND
#undef DFU_ALT_INFO_NAND
#endif
#define DFU_ALT_INFO_NAND \
	"NAND.SPL part 0 1;" \
	"NAND.SPL.backup1 part 0 2;" \
	"NAND.SPL.backup2 part 0 3;" \
	"NAND.SPL.backup3 part 0 4;" \
	"NAND.u-boot-spl-os part 0 5;" \
	"NAND.u-boot part 0 6;" \
	"NAND.u-boot-env part 0 7;" \
	"NAND.u-boot-env.backup1 part 0 8;" \
	"NAND.kernel part 0 9;" \
	"NAND.rootfs part 0 10"
#endif
#define CONFIG_DFU_RAM
#define DFU_ALT_INFO_RAM \
	"kernel ram 0x80200000 0xD80000;" \
	"fdt ram 0x80F80000 0x80000;" \
	"ramdisk ram 0x81000000 0x4000000"
#endif //0###


/************************************************************************
 * Linux Support
 ************************************************************************/
#define CONFIG_ZIMAGE_BOOT
#define CONFIG_IMAGE_BOOT

/* Try to patch serial debug port in image within first 16KB of zImage */
#define CONFIG_SYS_PATCH_TTY	0x4000

/* ATAGs passed to Linux */
#define CONFIG_SETUP_MEMORY_TAGS	/* Memory setup */
#define CONFIG_CMDLINE_TAG		/* Command line */
#undef CONFIG_INITRD_TAG		/* No initrd */
#define CONFIG_REVISION_TAG		/* Board revision */
#define CONFIG_FSHWCONFIG_TAG		/* Hardware config (NBoot-Args) */


/************************************************************************
 * Tools
 ************************************************************************/
#define CONFIG_ADDFSHEADER


/************************************************************************
 * Libraries
 ************************************************************************/
//#define USE_PRIVATE_LIBGCC
#define CONFIG_SYS_64BIT_VSPRINTF	/* Needed for nand_util.c */
#define CONFIG_USE_ARCH_MEMCPY
#define CONFIG_USE_ARCH_MEMMOVE
#define CONFIG_USE_ARCH_MEMSET
#define CONFIG_USE_ARCH_MEMSET32

#endif	/* ! __FSAM335X_CONFIG_H */