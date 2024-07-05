/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2022 F&S Elektronik Systeme GmbH
 *
 */

#ifndef __FSGAL_H
#define __FSGAL_H

#include "ls1028a_common.h"

//Unset some defines
#undef CONFIG_ID_EEPROM
#undef CONFIG_SYS_I2C_EEPROM_NXID
#undef CONFIG_SYS_EEPROM_BUS_NUM
#undef CONFIG_SYS_I2C_EEPROM_ADDR
#undef CONFIG_SYS_I2C_EEPROM_ADDR_LEN
#undef CONFIG_SYS_EEPROM_PAGE_WRITE_BITS
#undef CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS
#undef I2C_MUX_PCA_ADDR_PRI
#undef I2C_MUX_CH_DEFAULT
#undef DP_PWD_EN_DEFAULT_MASK
#undef SD_BOOTCOMMAND
#undef SD2_BOOTCOMMAND

#if defined(CONFIG_BOOTCOMMAND)
#define SD_BOOTCOMMAND CONFIG_BOOTCOMMAND
#define SD2_BOOTCOMMAND CONFIG_BOOTCOMMAND
#else
#define SD_BOOTCOMMAND	\
"run distro_bootcmd; " \
"env exists secureboot && esbc_halt;"
#define SD2_BOOTCOMMAND	\
"run distro_bootcmd;" \
"env exists secureboot && esbc_halt;"
#endif

#define CONFIG_SYS_CLK_FREQ		100000000
#define CONFIG_DDR_CLK_FREQ		100000000
#define COUNTER_FREQUENCY_REAL		(CONFIG_SYS_CLK_FREQ / 4)

#define CONFIG_SYS_RTC_BUS_NUM         0

/* Store environment at top of flash */

#define CONFIG_DIMM_SLOTS_PER_CTLR          1

#define CONFIG_SYS_MONITOR_BASE CONFIG_SYS_TEXT_BASE

/* SecureBoot Env additions */
#define ESBC_SCRIPTHDR	"env exists secureboot && esbc_validate $scripthdraddr || esbc_halt;"
#define SECUREBOOT_VALIDATE_IMAGE "env exists secureboot && esbc_validate $kernelheader_addr_r || esbc_halt;" \
	"env exists secureboot && esbc_validate $fdtheader_addr_r || esbc_halt;"

/* Initial environment variables */
#ifndef SPL_NO_ENV
#undef CFG_EXTRA_ENV_SETTINGS
#define CFG_EXTRA_ENV_SETTINGS \
	"board=GAL1\0"			\
	"hwconfig=fsl_ddr:bank_intlv=auto\0"	\
	"ramdisk_addr=0x800000\0"		\
	"ramdisk_size=0x2000000\0"		\
	"bootm_size=0x10000000\0"		\
	"kernel_addr=0x01000000\0"              \
	"scriptaddr=0x80000000\0"               \
	"scripthdraddr=0x80080000\0"		\
	"fdtheader_addr_r=0x80100000\0"         \
	"kernelheader_addr_r=0x80200000\0"      \
	"load_addr=0xa0000000\0"            \
	"kernel_addr_r=0x81000000\0"            \
	"fdt_addr_r=0x90000000\0"               \
	"fdt_addr=0x90000000\0"                 \
	"ramdisk_addr_r=0xa0000000\0"           \
	"kernel_start=0x1000000\0"		\
	"kernelheader_start=0x600000\0"		\
	"kernel_load=0xa0000000\0"		\
	"kernel_size=0x2800000\0"		\
	"kernelheader_size=0x40000\0"		\
	"kernel_addr_sd=0x8000\0"		\
	"kernel_size_sd=0x14000\0"		\
	"kernelhdr_addr_sd=0x3000\0"		\
	"kernelhdr_size_sd=0x20\0"		\
	"console=ttyS0,115200\0"                \
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0"	\
	BOOTENV					\
	"boot_scripts=fsgal_boot.scr\0"    \
	"boot_script_hdr=hdr_bootscr.out\0" \
	"kernel_image=Image\0" \
	"dtb=fs-gal1.dtb\0" \
	\
	"scan_dev_for_boot_part=" \
		"part list ${devtype} ${devnum} devplist; " \
		"env exists devplist || setenv devplist 1; " \
		"for distro_bootpart in ${devplist}; do " \
		  "if fstype ${devtype} " \
			"${devnum}:${distro_bootpart} " \
			"bootfstype; then " \
			"run scan_dev_for_boot; " \
		  "fi; " \
		"done\0" \
	\
	"scan_dev_for_boot=" \
	"echo Scanning ${devtype} " \
		"${devnum}:${distro_bootpart}...; " \
	"for prefix in ${boot_prefixes}; do " \
		"run scan_dev_for_scripts; " \
		"run scan_dev_for_images; " \
		"run scan_dev_for_extlinux; " \
	"done;" \
	SCAN_DEV_FOR_EFI \
	"\0" \
	\
	"boot_a_script=" \
		"load ${devtype} ${devnum}:${distro_bootpart} " \
			"${scriptaddr} ${prefix}${script}; " \
		"env exists secureboot && load ${devtype} " \
			"${devnum}:${distro_bootpart} " \
			"${scripthdraddr} ${prefix}${boot_script_hdr};" \
			ESBC_SCRIPTHDR \
		"source ${scriptaddr};\0" \
	\
	"scan_dev_for_images=" \
	"if test -e ${devtype} ${devnum}:${distro_bootpart} ${kernel_image}; then " \
		"echo Kernel ${kernel_image} found; " \
		"if test -e ${devtype} ${devnum}:${distro_bootpart} ${dtb}; then " \
			"echo DeviceTree ${dtb} found; " \
			"run boot_a_image; " \
		"else " \
			"echo ${dtb} not found!; " \
		"fi; " \
	"else " \
		"echo ${kernel_image} not found!; " \
	"fi;\0" \
	\
	"boot_a_image=echo Trying load from ${devtype} ${devnum}:${distro_bootpart} ...;" \
		"env exists devpart_root || setenv devpart_root 2;" \
		"part uuid $devtype $devnum:$devpart_root partuuidr;" \
		"setenv bootargs "\
			"console=ttyS0,115200 earlycon=uart8250,mmio,0x21c0500 " \
			"root=PARTUUID=$partuuidr rw rootwait cma=64M " \
			"iommu.passthrough=1 arm-smmu.disable_bypass=0 $othbootargs; " \
		"load $devtype $devnum:$devpart_boot $kernel_addr_r $kernel_image; " \
		"load $devtype $devnum:$devpart_boot $fdt_addr_r $dtb; " \
		"env exists secureboot && " \
			"echo Load CSF-Header ... && " \
			"load $devtype $devnum:$devpart_boot " \
				"$kernelheader_addr_r /secboot_hdrs/hdr_linux.out && " \
			"load $devtype $devnum:$devpart_boot " \
				"$fdtheader_addr_r /secboot_hdrs/hdr_dtb.out; " \
		SECUREBOOT_VALIDATE_IMAGE \
		"booti $kernel_addr_r - $fdt_addr_r;\0" 

#endif
#endif /* __FSGAL_H */
