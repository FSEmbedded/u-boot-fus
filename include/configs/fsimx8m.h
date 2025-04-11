/*
 * Copyright (C) 2017 F&S Elektronik Systeme GmbH
 * Copyright (C) 2018-2019 F&S Elektronik Systeme GmbH
 *
 * Configuration settings for all F&S boards based on i.MX8M. This is
 * armStoneMX8M.
 *
 * Activate with one of the following targets:
 *   make fsimx8m_defconfig   Configure for i.MX8M boards
 *   make                     Build uboot-spl.bin, u-boot.bin and u-boot-nodtb.bin.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __FSIMX8M_H
#define __FSIMX8M_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#ifdef CONFIG_IMX_HAB
#define CONFIG_CSF_SIZE			0x2000 /* 8K region */
#endif

#define CONFIG_SYS_SERCON_NAME "ttymxc"	/* Base name for serial devices */

#ifdef CONFIG_SPL_BUILD

#define CFG_SYS_SPL_PTE_RAM_BASE    0x41580000

/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x182000

#undef CONFIG_DM_MMC
#undef CONFIG_DM_PMIC
#undef CONFIG_DM_PMIC_PFUZE100

#endif /* CONFIG_SPL_BUILD*/

#if defined(CONFIG_CMD_NET)
#define CFG_FEC_MXC_PHYADDR          4
#endif


/************************************************************************
 * Generic MTD Settings
 ************************************************************************/
#ifdef CONFIG_NAND_BOOT
/* Define MTD partition info */
#define MTDIDS_DEFAULT      "nand0=gpmi-nand"
#define MTDPART_DEFAULT     "nand0,1"
#define MTDPARTS_PART1      "gpmi-nand:4m(Spl)"
#define MTDPARTS_PART2	    "4m(UBoot),256k(UBootEnv)"
#define MTDPARTS_PART3	    "32m(Kernel)ro,1792k(FDT)ro"
#define MTDPARTS_PART4	    "-(TargetFS)"
#define MTDPARTS_DEFAULT    "mtdparts=" MTDPARTS_PART1 "," MTDPARTS_PART2 "," MTDPARTS_PART3 "," MTDPARTS_PART4
#define MTDPARTS_STD	    "setenv mtdparts " MTDPARTS_DEFAULT
#define MTDPARTS_UBIONLY    "setenv mtdparts mtdparts=" MTDPARTS_PART1 "," MTDPARTS_PART2 "," MTDPARTS_PART4

#endif

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
#define BOOT_WITH_FDT "\\\\; booti ${loadaddr} - ${fdtaddr}\0"

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
	".ubivol_ubi=ubi part TargetFS; ubi create kernel 800000 s; ubi create rootfs\0"
#else
#define EXTRA_UBI
#endif

#ifdef CONFIG_BOOTDELAY
#define FSBOOTDELAY
#else
#define FSBOOTDELAY "bootdelay=undef\0"
#endif

/* Initial environment variables */
#if defined(CONFIG_NAND_BOOT)
#define CONFIG_EXTRA_ENV_SETTINGS \
  	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
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
	".kernel_nand=setenv kernel nand read ${loadaddr} Kernel\0" \
	".kernel_tftp=setenv kernel tftpboot . ${bootfile}\0" \
	".kernel_nfs=setenv kernel nfs . ${serverip}:${rootpath}/${bootfile}\0" \
	".kernel_mmc=setenv kernel mmc rescan\\\\; load mmc 0 . ${bootfile}\0" \
	".kernel_usb=setenv kernel usb start\\\\; load usb 0 . ${bootfile}\0" \
	"fdt=undef\0" \
	"fdtaddr=0x43000000\0" \
	".fdt_none=setenv fdt booti\0" \
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
	"arch=fsimx8m\0" \
	"bootfdt=undef\0" \
	FSBOOTDELAY \
	"fdt_high=0xffffffffffffffff\0" \
	"set_bootfdt=setenv bootfdt ${platform}.dtb\0" \
	"set_bootargs=setenv bootargs ${console} ${login} ${mtdparts} ${network} ${rootfs} ${mode} ${init} ${extra}\0"
#endif

/* Link Definitions */
//####define CONFIG_SYS_LOAD_ADDR            0x40480000

#define CFG_SYS_INIT_RAM_ADDR 0x40000000
#define CFG_SYS_INIT_RAM_SIZE 0x80000
#define CFG_SYS_INIT_SP_OFFSET (CFG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#if 0 //###
#define CONFIG_SYS_INIT_SP_ADDR \
        (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)
#endif //###

/************************************************************************
 * Environment
 ************************************************************************/

/*
 * Environment size and location are now set in the device tree. However there
 * are fallback values set in the defconfig if values in the device tree are
 * missing or damaged. The environment is held in the heap, so keep the real
 * size small to not waste malloc space. Use two blocks (0x40000, 256KB) for
 * CONFIG_ENV_NAND_RANGE to have one spare block in case of a bad first block.
 * See also MMC and NAND layout above.
 */

#define CFG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC2 */

#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */

/* have to define for F&S serial_mxc driver */
#define UART1_BASE UART1_BASE_ADDR
#define UART2_BASE UART2_BASE_ADDR
#define UART3_BASE UART3_BASE_ADDR
#define UART4_BASE UART4_BASE_ADDR
#define UART5_BASE 0xFFFFFFFF

/* Not used on F&S boards. Detection depending on board type is preferred. */
#define CFG_MXC_UART_BASE		UART2_BASE_ADDR

/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif


/* USDHC */
#define CFG_SYS_FSL_USDHC_NUM	2
#define CFG_SYS_FSL_ESDHC_ADDR	0

#ifdef CONFIG_NAND_BOOT
#define CFG_SYS_NAND_BASE		0x40000000
#endif

/* Framebuffer */
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_IMXDCSS
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_IMX_VIDEO_SKIP
#endif

#endif
