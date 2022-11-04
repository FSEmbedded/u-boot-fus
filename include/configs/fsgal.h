/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019, 2021 NXP
 */

#ifndef __FSGAL_H
#define __FSGAL_H

#include "ls1028a_common.h"

#define CONFIG_SYS_CLK_FREQ		100000000
#define CONFIG_DDR_CLK_FREQ		100000000
#define COUNTER_FREQUENCY_REAL		(CONFIG_SYS_CLK_FREQ / 4)

#define CONFIG_SYS_RTC_BUS_NUM         0

/* Store environment at top of flash */

#define CONFIG_DIMM_SLOTS_PER_CTLR          1

#define CONFIG_SYS_MONITOR_BASE CONFIG_SYS_TEXT_BASE

/* Initial environment variables */
#ifndef SPL_NO_ENV
#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS		\
	"board=ls1028ardb\0"			\
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
	"boot_scripts=ls1028ardb_boot.scr\0"    \
	"boot_script_hdr=hdr_ls1028ardb_bs.out\0"	\
	"scan_dev_for_boot_part="               \
		"part list ${devtype} ${devnum} devplist; "   \
		"env exists devplist || setenv devplist 1; "  \
		"for distro_bootpart in ${devplist}; do "     \
		  "if fstype ${devtype} "                  \
			"${devnum}:${distro_bootpart} "      \
			"bootfstype; then "                  \
			"run scan_dev_for_boot; "            \
		  "fi; "                                   \
		"done\0"                                   \
	"boot_a_script="				  \
		"load ${devtype} ${devnum}:${distro_bootpart} "  \
			"${scriptaddr} ${prefix}${script}; "    \
		"env exists secureboot && load ${devtype} "     \
			"${devnum}:${distro_bootpart} "		\
			"${scripthdraddr} ${prefix}${boot_script_hdr} " \
			"&& esbc_validate ${scripthdraddr};"    \
		"source ${scriptaddr}\0"	  \
	"xspi_bootcmd=echo Trying load from FlexSPI flash ...;" \
		"sf probe 0:0 && sf read $load_addr " \
		"$kernel_start $kernel_size ; env exists secureboot &&" \
		"sf read $kernelheader_addr_r $kernelheader_start " \
		"$kernelheader_size && esbc_validate ${kernelheader_addr_r}; "\
		" bootm $load_addr#$board\0" \
	"xspi_hdploadcmd=echo Trying load HDP firmware from FlexSPI...;" \
		"sf probe 0:0 && sf read $load_addr 0x940000 0x30000 " \
		"&& hdp load $load_addr 0x2000\0"			\
	"sd_bootcmd=echo Trying load from SD ...;" \
		"mmc dev 0;mmcinfo; mmc read $load_addr "		\
		"$kernel_addr_sd $kernel_size_sd && "	\
		"env exists secureboot && mmc read $kernelheader_addr_r " \
		"$kernelhdr_addr_sd $kernelhdr_size_sd "		\
		" && esbc_validate ${kernelheader_addr_r};"	\
		"bootm $load_addr#$board\0"		\
	"sd_hdploadcmd=echo Trying load HDP firmware from SD..;"	\
		"mmc dev 0;mmcinfo;mmc read $load_addr 0x4a00 0x200 "	\
		"&& hdp load $load_addr 0x2000\0"	\
	"emmc_bootcmd=echo Trying load from EMMC ..;"	\
		"mmc dev 1;mmcinfo; mmc read $load_addr "		\
		"$kernel_addr_sd $kernel_size_sd && "	\
		"env exists secureboot && mmc read $kernelheader_addr_r " \
		"$kernelhdr_addr_sd $kernelhdr_size_sd "		\
		" && esbc_validate ${kernelheader_addr_r};"	\
		"bootm $load_addr#$board\0"			\
	"emmc_hdploadcmd=echo Trying load HDP firmware from EMMC..;"      \
		"mmc dev 1;mmcinfo;mmc read $load_addr 0x4a00 0x200 "	\
		"&& hdp load $load_addr 0x2000\0"
#endif
#endif /* __FSGAL_H */
