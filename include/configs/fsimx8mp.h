/*
 * Copyright (C) 2017 F&S Elektronik Systeme GmbH
 * Copyright (C) 2018-2019 F&S Elektronik Systeme GmbH
 * Copyright (C) 2021 F&S Elektronik Systeme GmbH
 *
 * Configuration settings for all F&S boards based on i.MX8MP. This is
 * PicoCoreMX8MP.
 *
 * Activate with one of the following targets:
 *   make fsimx8mp_defconfig   Configure for i.MX8MP boards
 *   make                     Build uboot-spl.bin, u-boot.bin and u-boot-nodtb.bin.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
#define OCRAM_BASE		0x900000

 * TCM layout (SPL)
 * ----------------
 * unused
 *
 * OCRAM layout SPL                 U-Boot
 * ---------------------------------------------------------
 * 0x0090_0000: (Region reserved by ROM loader)(96KB)
 * 0x0091_8000: BOARD-CFG           BOARD-CFG (8KB)  CONFIG_FUS_BOARDCFG_ADDR
 * 0x0091_A000: BSS data            cfg_info  (2KB)  CONFIG_SPL_BSS_START_ADDR
 * 0x0091_A800: MALLOC_F pool       ---       (34KB) CONFIG_MALLOC_F_ADDR
 * 0x0092_3000: ---                 ---       (4KB)
 * 0x0092_7FF0: Global Data + Stack ---       (20KB) CONFIG_SPL_STACK
 * 0x0092_8000: SPL (<= ~208KB) (loaded by ROM-Loader, address defined by ATF)
 *     DRAM-FW: Training Firmware (up to 96KB, immediately behind end of SPL)
 * 0x0096_4000: DRAM Timing Data    ---       (16KB) CONFIG_SPL_DRAM_TIMING_ADDR
 * 0x0096_8000: ATF (8MP)           ATF       (96KB) CONFIG_SPL_ATF_ADDR
 * 0x0098_8000: (SPL Multi DTB)     ---       (32KB) (CONFIG_SPL_MULTI_DTB_FIT_USER_DEF_ADDR)
 * 0x0098_FFFF: END (8MP) #####!!!!!##### (MP hat 576KB OCRAM, nicht nur 512KB)
 *
 * The sum of SPL and DDR_FW must not exceed 240KB (0x3C000). However there is
 * still room to extend this region if SPL grows larger in the future, e.g. by
 * letting DRAM Timing Data overlap with ATF region.
 * After DRAM is available, SPL uses a MALLOC_R pool at 0x4220_0000.
 *
 * NAND flash layout
 * -----------------
 * There are no boards with NAND flash available. If we ever build one, we can
 * use settings similar to fsimx8mn.
 *
 * eMMC Layout
 * -----------
 * The boot process from eMMC can be configured to boot from a Boot partition
 * or from the User partition. In the latter case, there needs to be a reserved
 * area of 8MB at the beginning of the User partition.
 *
 * 0x0000_0000: Space for GPT (32KB)
 * 0x0000_8000: NBoot (see nboot/nboot-info.dtsi for details)
 * 0x0080_0000: End of reserved area, start of regular filesystem partitions
 *
 * Remarks:
 * - nboot-start[] in nboot-info is set to CONFIG_FUS_BOARDCFG_MMC0/1 by the
 *   Makefile. This is the only value where SPL and nboot-info must match.
 * - The reserved region size is the same as in NXP layouts (8MB).
 * - The space in the reserved region when booting from Boot partition, can be
 *   used to store an M4 image or as UserDef region.
 */

#ifndef __FSIMX8MP_H
#define __FSIMX8MP_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

/* disable FASTBOOT_USB_DEV so both ports can be used */
#undef CONFIG_FASTBOOT_USB_DEV

#define CFG_SYS_UBOOT_BASE	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/*
 * The memory layout on stack:  DATA section save + gd + early malloc
 * the idea is re-use the early malloc (CONFIG_SYS_MALLOC_F_LEN) with
 * CONFIG_SYS_SPL_MALLOC_START
 */
#define CONFIG_FUS_BOARDCFG_ADDR	0x00918000

#ifdef CONFIG_SPL_BUILD
/* Offsets in eMMC where BOARD-CFG and FIRMWARE are stored */
#define CONFIG_FUS_BOARDCFG_MMC0	0x00048000
#define CONFIG_FUS_BOARDCFG_MMC1	0x00448000

/* These addresses are hardcoded in ATF */
#define CONFIG_SPL_USE_ATF_ENTRYPOINT
#define CONFIG_SPL_ATF_ADDR		0x00968000
#define CONFIG_SPL_TEE_ADDR		0x56000000

/* TCM Address where DRAM Timings are loaded to */
#define CONFIG_SPL_DRAM_TIMING_ADDR	0x00964000

/* malloc f is used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x91A800

#endif /* CONFIG_SPL_BUILD */

#if defined(CONFIG_CMD_NET)
#define FDT_SEQ_MACADDR_FROM_ENV

//#define DWC_NET_PHYADDR					4
#ifdef CONFIG_DWC_ETH_QOS
#define CONFIG_SYS_NONCACHED_MEMORY     (1 * SZ_1M)     /* 1M */
#endif

#define PHY_ANEG_TIMEOUT 20000

#endif

#define CONFIG_BOOTCOMMAND	"run select_boot_mode"

#define SECURE_PARTITIONS	"UBoot", "Kernel", "FDT", "Images"

/************************************************************************
 * Environment
 ************************************************************************/
/* Define MTD partition info */
#define MTDIDS_DEFAULT		""
#define MTDPART_DEFAULT		""
#define MTDPARTS_STD		"setenv mtdparts "
#define MTDPARTS_UBIONLY	"setenv mtdparts "

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

/*
 * Unified boot configuration: A/B update and legacy boot are both available
 * at runtime, controlled by the "use_ab" environment variable. No separate
 * build configurations needed.
 *
 */

#define BOOT_FROM_NAND
#define BOOT_FROM_UBI
#define BOOT_FROM_UBIFS

/*
 * In case of (e)MMC, the rootfs is loaded from a separate partition. Kernel
 * and device tree are loaded as files from a different partition that is
 * typically formated with FAT.
 */
#ifdef CONFIG_CMD_MMC
#define BOOT_FROM_MMC							\
	".boot_part_A=1\0"						\
	".boot_part_B=2\0"						\
	".boot_part=1\0"						\
	".rootfs_part_A=3\0"						\
	".rootfs_part_B=4\0"						\
	".rootfs_part=2\0"						\
	".kernel_mmc=setenv kernel n=.boot_part\\\\${slot_}\\\\;"	\
	" mmc rescan\\\\; load mmc ${mmcdev}:\\\\${!n} . ${bootfile}\0"	\
	".fdt_mmc=setenv fdt n=.boot_part\\\\${slot_}\\\\; mmc rescan\\\\; " \
	" load mmc ${mmcdev}:\\\\${!n} ${fdtaddr} \\\\${bootfdt}" BOOT_WITH_FDT \
	".rootfs_mmc=setenv set_rootfs n=.rootfs_part\\\\${slot_}\\\\;" \
	" part uuid mmc ${mmcdev}:\\\\${!n} rootfsuuid\\\\;" \
	" setenv rootfs root=PARTUUID=\\\\${rootfsuuid} ${rootfstype} rootwait\0"
#else
#define BOOT_FROM_MMC
#endif

/* In case of USB, the layout is the same as on MMC (no A/B support). */
#define BOOT_FROM_USB							\
	".kernel_usb=setenv kernel usb start\\\\;"                      \
	" load usb 0 . ${bootfile}\0"                                   \
	".fdt_usb=setenv fdt usb start\\\\;"                            \
	" load usb 0 ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT               \
	".rootfs_usb=setenv rootfs root=/dev/sda1 rootwait\0"

/* In case of TFTP, kernel and device tree are loaded from TFTP server */
#define BOOT_FROM_TFTP							\
	".kernel_tftp=setenv kernel tftpboot . ${bootfile}\0"           \
	".fdt_tftp=setenv fdt tftpboot ${fdtaddr} ${bootfdt}" BOOT_WITH_FDT

/* In case of NFS, kernel, device tree and rootfs are loaded from NFS server */
#define BOOT_FROM_NFS							\
	".kernel_nfs=setenv kernel nfs ."                               \
	" ${serverip}:${rootpath}/${bootfile}\0"                        \
	".fdt_nfs=setenv fdt nfs ${fdtaddr}"                            \
	" ${serverip}:${rootpath}/${bootfdt}" BOOT_WITH_FDT             \
	".rootfs_nfs=setenv rootfs root=/dev/nfs"                       \
	" nfsroot=${serverip}:${rootpath},tcp,v3\0"

/*
 * Generic settings for booting with updates on A/B.
 * RAUC-aligned: iterates BOOT_ORDER, decrements counter, single saveenv.
 */
#define BOOT_SYSTEM							\
	".init_fs_updater=setenv init init=/sbin/preinit.sh\0"		\
	"BOOT_ORDER=A B\0"						\
	"BOOT_ORDER_OLD=A B\0"						\
	"BOOT_A_LEFT=3\0"						\
	"BOOT_B_LEFT=3\0"						\
	"update_reboot_state=0\0"					\
	"update=0000\0"							\
	"application=A\0"						\
	"rauc_cmd=rauc.slot=A\0"					\
	"selector="                                                     \
		"rootfstype=rootfstype=squashfs; " \
		"for slot in ${BOOT_ORDER}; do "                        \
			"n=BOOT_${slot}_LEFT;"				\
			"slot_cnt=${!n}; "                              \
			"if test ${slot_cnt} -gt 0; then "              \
				"slot_=_${slot}; "			\
				"setexpr BOOT_${slot}_LEFT ${slot_cnt} - 1; " \
				"setenv rauc_cmd rauc.slot=${slot}; "   \
				"saveenv; "                             \
				"echo \"Booting slot ${slot} (${slot_cnt} left)\"; " \
				"exit; "                                \
			"fi; "                                          \
		"done; "                                                \
		"echo \"Boot failed, system corrupted\"; "   \
		"setenv boot_failed 1;\0"                    \


/* Generic variables */

#ifdef CONFIG_BOOTDELAY
#define FSBOOTDELAY
#else
#define FSBOOTDELAY "bootdelay=undef\0"
#endif

/*
 * Boot mode dispatch: runtime selection between A/B update and legacy boot.
 * select_boot_mode is the single entry point called by CONFIG_BOOTCOMMAND.
 */
#define BOOT_MODE_DISPATCH						\
	"select_boot_mode="                         \
		"if test -n \"${use_ab}\"; then "           \
			"run .init_fs_updater selector; "   \
			"if test -z \"${boot_failed}\"; then "\
				"run set_bootargs kernel fdt; "	\
				"run failed_update_reset; "     \
			"fi; "                              \
		"else "                                 \
			"setenv rauc_cmd; "                 \
			"run .init_init; "                  \
			"run set_bootargs kernel fdt; "     \
		"fi\0"

#if defined(CONFIG_ENV_IS_IN_MMC)
	#define FILESIZE2BLOCKCOUNT "block_size=200\0" 	\
		"filesize2blockcount=" \
			"setexpr test_rest \\${filesize} % \\${block_size}; " \
			"if test \\${test_rest} = 0; then " \
				"setexpr blockcount \\${filesize} / \\${block_size}; " \
			"else " \
				"setexpr blockcount \\${filesize} / \\${block_size}; " \
				"setexpr blocckount \\${blockcount} + 1; " \
			"fi;\0"
#else
	#define FILESIZE2BLOCKCOUNT
#endif

/* Reset update process if uncaught error drops to u-boot shell */
#define FAILED_UPDATE_RESET                                             \
	"failed_update_reset="                                          \
		"if test \"x${BOOT_ORDER_OLD}\" != \"x${BOOT_ORDER}\"; then " \
			"reset; "                                       \
		"fi;\0"

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS                                          \
	"bd_kernel=undef\0"                                             \
	"bd_fdt=undef\0"                                                \
	"bd_rootfs=undef\0"                                             \
	"initrd_addr=0x43800000\0"                                      \
	"initrd_high=0xffffffffffffffff\0"                              \
	"console=undef\0"                                               \
	".console_none=setenv console\0"                                \
	".console_serial=setenv console console=${sercon},${baudrate}\0" \
	".console_display=setenv console console=tty1\0"                \
	"login=undef\0"                                                 \
	".login_none=setenv login login_tty=null\0"                     \
	".login_serial=setenv login login_tty=${sercon},${baudrate}\0"  \
	".login_display=setenv login login_tty=tty1\0"                  \
	"mode=undef\0"                                                  \
	".mode_rw=setenv mode rw\0"                                     \
	".mode_ro=setenv mode ro\0"                                     \
	"init=undef\0"                                                  \
	".init_init=setenv init\0"                                      \
	".init_linuxrc=setenv init init=linuxrc\0"                      \
	"mtdids=undef\0"                                                \
	"mtdparts=undef\0"                                              \
	"netdev=eth0\0"                                                 \
	"mmcdev=undef\0"                                                \
	".network_off=setenv network\0"                                 \
	".network_on=setenv network ip=${ipaddr}:${serverip}:"          \
	"${gatewayip}:${netmask}:${hostname}:${netdev}\0"               \
	".network_dhcp=setenv network ip=dhcp\0"                        \
	"rootfs=undef\0"                                                \
	"kernel=undef\0"                                                \
	"fdt=undef\0"                                                   \
	"fdtaddr=0x43100000\0"                                          \
	".fdt_none=setenv fdt booti\0"                                  \
	BOOT_FROM_MMC                                                \
	BOOT_FROM_USB                                                \
	BOOT_FROM_TFTP                                               \
	BOOT_FROM_NFS                                                \
	BOOT_SYSTEM                                                  \
	BOOT_MODE_DISPATCH                                           \
	FILESIZE2BLOCKCOUNT                                             \
	FSBOOTDELAY                                                     \
	FAILED_UPDATE_RESET                                             \
	"sercon=undef\0"                                                \
	"installcheck=undef\0"                                          \
	"updatecheck=undef\0"                                           \
	"recovercheck=undef\0"                                          \
	"platform=undef\0"                                              \
	"arch=fsimx8mp\0"                                               \
	"bootfdt=undef\0"                                               \
	"m4_uart4=disable\0"                                            \
	"fdt_high=0xffffffffffffffff\0"                                 \
	"set_bootfdt=setenv bootfdt ${platform}.dtb\0"                  \
	"set_bootargs=run set_rootfs\\; setenv bootargs ${console} ${login} ${mtdparts}"  \
	" ${network} ${rootfs} ${mode} ${init} ${extra} ${rauc_cmd}\0"

/* Link Definitions */
#define CFG_SYS_INIT_RAM_ADDR 0x40000000
#define CFG_SYS_INIT_RAM_SIZE 0x00080000
#define CFG_SYS_INIT_SP_OFFSET CONFIG_SYS_INIT_RAM_SIZE


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

/* Fallback values if values in the device tree are missing/damaged */
//#define CONFIG_ENV_OFFSET_REDUND 0x104000

/* Totally 1GB LPDDR4 */
#define CFG_SYS_SDRAM_BASE		0x40000000

/* F&S: Location of BOARD-CFG in OCRAM and how far to search if not found */
#define CFG_SYS_OCRAM_BASE		0x00900000
#define CFG_SYS_OCRAM_SIZE		0x00090000

/* have to define for F&S serial_mxc driver */
#define UART1_BASE			UART1_BASE_ADDR
#define UART2_BASE			UART2_BASE_ADDR
#define UART3_BASE			UART3_BASE_ADDR
#define UART4_BASE			UART4_BASE_ADDR
#define UART5_BASE			0xFFFFFFFF

/* Not used on F&S boards. Detection depending on board type is preferred. */
#define CFG_MXC_UART_BASE		0

/* Monitor Command Prompt */

/************************************************************************
 * Command Line Editor (Shell)
 ************************************************************************/
#ifdef CONFIG_SYS_HUSH_PARSER
//####define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

#define CFG_SYS_FSL_ESDHC_ADDR	0

/* Kann das weg? Die Defines werden nirgendwo genutzt */
#ifdef CONFIG_FSL_FSPI
#define FSL_FSPI_FLASH_SIZE		SZ_256M
#define FSL_FSPI_FLASH_NUM		1
#define FSPI0_BASE_ADDR			0x30bb0000
#define FSPI0_AMBA_BASE			0x0
#define CONFIG_FSPI_QUAD_SUPPORT

#define CONFIG_SYS_FSL_FSPI_AHB
#endif

#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_LOGO
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_BMP_24BPP
#define CONFIG_BMP_32BPP
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_BMP_LOGO
#endif

#endif
