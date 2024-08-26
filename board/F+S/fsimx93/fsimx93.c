/*
* Copyright 2024 F&S Elektronik Systeme GmbH
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <common.h>
#include <env.h>
#include <efi_loader.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/imx93_pins.h>
#include <asm/arch/clock.h>
#include <power/pmic.h>
#include "../common/tcpc.h"
#include <dm/device.h>
#include <dm/uclass.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/gpio.h>
#include "../common/fs_fdt_common.h"	/* fs_fdt_set_val(), ... */
#include "../common/fs_board_common.h"	/* fs_board_*() */
#include "../common/fs_eth_common.h"	/* fs_eth_*() */

DECLARE_GLOBAL_DATA_PTR;

#define BT_PICOCOREMX93 	0
#define BT_OSM93 	1

#define FEAT_ETH_A 	(1<<0)	/* 0: no LAN0,  1: has LAN0 */
#define FEAT_ETH_B	(1<<1)	/* 0: no LAN1,  1: has LAN1 */
#define FEAT_DISP_A	(1<<2)	/* 0: MIPI-DSI, 1: LVDS lanes 0-3 */
#define FEAT_DISP_B	(1<<3)	/* 0: HDMI,     1: LVDS lanes 0-4 or (4-7 if DISP_A=1) */
#define FEAT_AUDIO 	(1<<4)	/* 0: no Audio, 1: Analog Audio Codec */
#define FEAT_WLAN	(1<<5)	/* 0: no WLAN,  1: has WLAN */
#define FEAT_EXT_RTC	(1<<6)	/* 0: internal RTC, 1: external RTC */
#define FEAT_NAND	(1<<7)	/* 0: no NAND,  1: has NAND */
#define FEAT_EMMC	(1<<8)	/* 0: no EMMC,  1: has EMMC */
#define FEAT_SEC_CHIP	(1<<9)	/* 0: no SE050,  1: has SE050 */
#define FEAT_EEPROM	(1<<10)	/* 0: no EEPROM,  1: has EEPROM */
#define FEAT_ADC	(1<<11)	/* 0: no ADC,  1: has ADC */
#define FEAT_DISP_RGB	(1<<12)	/* 0: no RGB Display,  1: has RGB Display */
#define FEAT_SD_A	(1<<13)	/* 0: no SD_A,  1: has SD_A */
#define FEAT_SD_B	(1<<14)	/* 0: no SD_B,  1: has SD_B */

/* TODO: Should be overworked by configurations */
/* features for picocoremx93-fert4 */
#define PICOCOREMX93_FERT4_FEAT \
	( FEAT_SD_A | FEAT_ADC | FEAT_EMMC | FEAT_EXT_RTC | \
	FEAT_WLAN | FEAT_AUDIO | FEAT_ETH_B | FEAT_ETH_A )


#define INSTALL_RAM "ram@84800000"
#if defined(CONFIG_MMC) && defined(CONFIG_USB_STORAGE) && defined(CONFIG_FS_FAT)
#define UPDATE_DEF "mmc,usb"
#define INSTALL_DEF INSTALL_RAM "," UPDATE_DEF
#elif defined(CONFIG_MMC) && defined(CONFIG_FS_FAT)
#define UPDATE_DEF "mmc"
#define INSTALL_DEF INSTALL_RAM "," UPDATE_DEF
#elif defined(CONFIG_USB_STORAGE) && defined(CONFIG_FS_FAT)
#define UPDATE_DEF "usb"
#define INSTALL_DEF INSTALL_RAM "," UPDATE_DEF
#else
#define UPDATE_DEF NULL
#define INSTALL_DEF INSTALL_RAM
#endif

#ifdef CONFIG_FS_UPDATE_SUPPORT
#define INIT_DEF ".init_fs_updater"
#else
#define INIT_DEF ".init_init"
#endif

const struct fs_board_info board_info[] = {
	{	/* 0 (BT_PICOCOREMX93) */
		.name = "PicoCoreMX93",
		.bootdelay = "3",
		.updatecheck = UPDATE_DEF,
		.installcheck = INSTALL_DEF,
		.recovercheck = UPDATE_DEF,
		.console = ".console_serial",
		.login = ".login_serial",
		.mtdparts = ".mtdparts_std",
		.network = ".network_off",
		.init = INIT_DEF,
		.flags = 0,
	},
	{	/* 1 (BT_OSM93) */
		.name = "OSM93",
		.bootdelay = "3",
		.updatecheck = UPDATE_DEF,
		.installcheck = INSTALL_DEF,
		.recovercheck = UPDATE_DEF,
		.console = ".console_serial",
		.login = ".login_serial",
		.mtdparts = ".mtdparts_std",
		.network = ".network_off",
		.init = INIT_DEF,
		.flags = 0,
	},
};

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0xbc550d86, 0xda26, 0x4b70, 0xac, 0x05, \
		 0x2a, 0x44, 0x8e, 0xda, 0x6f, 0x21)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"PICOCOREMX93-RAW",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=flash-bin raw 0 0x2000 mmcpart 1",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};

#endif /* EFI_HAVE_CAPSULE_SUPPORT */

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX93_PAD_GPIO_IO09__LPUART7_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX93_PAD_GPIO_IO08__LPUART7_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(LPUART7_CLK_ROOT);

	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP)
#define FDT_CPU_TEMP_ALERT	"/thermal-zones/cpu-thermal/trips/cpu-alert"
#define FDT_CPU_TEMP_CRIT	"/thermal-zones/cpu-thermal/trips/cpu-crit"

/* Do all fixups that are done on both, U-Boot and Linux device tree */
static int do_fdt_board_setup_common(void *fdt, bool verbose)
{
	int offs;
	int minc, maxc;
	__maybe_unused uint32_t temp_range;

	/* get CPU temp grade from the fuses */
	temp_range = get_cpu_temp_grade(&minc, &maxc);

	/* Sanity check for get_cpu_temp_grade() */
	if ((minc > -500) && maxc < 500) {
		u32 tmp_val;
		tmp_val = (maxc - 10) * 1000;
		offs = fs_fdt_path_offset(fdt, FDT_CPU_TEMP_ALERT);
		fs_fdt_set_u32(fdt, offs, "temperature", tmp_val, 1, verbose);
		tmp_val = maxc * 1000;
		offs = fs_fdt_path_offset(fdt, FDT_CPU_TEMP_CRIT);
		fs_fdt_set_u32(fdt, offs, "temperature", tmp_val, 1, verbose);
	} else {
		printf("## Wrong cpu temp grade values read! Keeping defaults from device tree\n");
	}
	return 0;
}

/* Do any board-specific modifications on U-Boot device tree before starting */
int board_fix_fdt(void *fdt)
{
	/* Make some room in the FDT */
	fdt_shrink_to_minimum(fdt, 8192);
	return do_fdt_board_setup_common(fdt, false);
}

/* Do any additional board-specific modifications on Linux device tree */
int ft_board_setup(void *fdt, struct bd_info *bd)
{
	do_fdt_board_setup_common(fdt, true);
	return	fdt_fixup_memory(fdt, CFG_SYS_SDRAM_BASE, gd->bd->bi_dram[0].size);
}
#endif

static int setup_fec(void)
{
	return set_clk_enet(ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

void fs_ethaddr_init(void)
{
	int eth_id = 0;
	/* TODO: */
	unsigned int board_type = BT_PICOCOREMX93;

	/* Set MAC addresses as environment variables */
	switch (board_type)
	{
	case BT_PICOCOREMX93:
		fs_eth_set_ethaddr(eth_id++);
		fs_eth_set_ethaddr(eth_id++);
		break;
	default:
		break;
	}
}

int board_init(void)
{
	unsigned int board_type = BT_PICOCOREMX93;
	/* Copy NBoot args to variables and prepare command prompt string */
	fs_board_init_common(&board_info[board_type]);

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	return 0;
}

int board_late_init(void)
{
	struct fs_nboot_args *pargs = fs_board_get_nboot_args();
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
	/* TODO: use 0 default */
	pargs->dwNBOOT_VER = 0;
	/* Set up all board specific variables */
	fs_board_late_init_common("ttymxc");	/* Set up all board specific variables */

	/* Set mac addresses for corresponding boards */
	fs_ethaddr_init();

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif
	return 0;
}

/* TODO: functions to compile fs_board_common */
enum boot_device fs_board_get_boot_dev(void)
{
	return MMC1_BOOT;
}

int serial_get_alias_seq(void)
{
	int seq, err;

	if (!gd->cur_serial_dev)
		return -ENXIO;

	err = fdtdec_get_alias_seq(gd->fdt_blob, "serial",
				   dev_of_offset(gd->cur_serial_dev), &seq);
	if (err < 0)
		return err;

	return seq;
}
