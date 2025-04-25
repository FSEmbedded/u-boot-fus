// SPDX-License-Identifier: GPL-2.0+
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
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#if CONFIG_IS_ENABLED(IMX93)
#include <asm/arch-imx9/imx93_pins.h>
#elif CONFIG_IS_ENABLED(IMX91)
#include <asm/arch-imx9/imx91_pins.h>
#endif
#include <asm/arch/clock.h>
#include <power/pmic.h>
#include "../common/tcpc.h"
#include <dm/device.h>
#include <dm/uclass.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/gpio.h>
#include <fdt_support.h>
#include <hang.h>

#include "fsimx93.h"
#include "../common/fs_board_common.h"
#include "../common/fs_eth_common.h"
#include "../common/fs_image_common.h"
#include "../common/fs_cntr_common.h"
#include "../common/fs_fdt_common.h"

#if CONFIG_IS_ENABLED(IMX93)
#define LABEL_CPU_ALERT "cpu_alert"
#define LABEL_CPU_CRIT "cpu_crit"
#elif CONFIG_IS_ENABLED(IMX91)
#define LABEL_CPU_ALERT "cpu_alert0"
#define LABEL_CPU_CRIT "cpu_crit0"
#endif

DECLARE_GLOBAL_DATA_PTR;

/* +++ Environment defines +++ */

#define INSTALL_RAM "ram@84800000"

#if CONFIG_IS_ENABLED(MMC) && CONFIG_IS_ENABLED(USB_STORAGE) && CONFIG_IS_ENABLED(FS_FAT)
#define UPDATE_DEF "mmc,usb"
#define INSTALL_DEF INSTALL_RAM "," UPDATE_DEF
#elif CONFIG_IS_ENABLED(MMC) && CONFIG_IS_ENABLED(USB_STORAGE)
#define UPDATE_DEF "mmc"
#define INSTALL_DEF INSTALL_RAM "," UPDATE_DEF
#elif CONFIG_IS_ENABLED(USB_STORAGE) && CONFIG_IS_ENABLED(FS_FAT)
#define UPDATE_DEF "usb"
#define INSTALL_DEF INSTALL_RAM "," UPDATE_DEF
#else
#define UPDATE_DEF NULL
#define INSTALL_DEF INSTALL_RAM
#endif

#if CONFIG_IS_ENABLED(FS_UPDATE_SUPPORT)
#define INIT_DEF ".init_fs_updater"
#else
#define INIT_DEF ".init_init"
#endif

/* --- Environment defines --- */
const struct fs_board_info board_info[] = {
	{	/* 0 (BT_PICOCOREMX93) */
		.name = "PicoCoreMX93",
		.bootdelay = __stringify(CONFIG_BOOTDELAY),
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
	{	/* 1 (BT_OSMSFMX93) */
		.name = "FS-OSM-SF-MX93",
		.bootdelay = __stringify(CONFIG_BOOTDELAY),
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
	{	/* 2 (BT_EFUSMX93) */
		.name = "efusMX93",
		.bootdelay = __stringify(CONFIG_BOOTDELAY),
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
	{	/* 3 (BT_PICOCOREMX91) */
		.name = "PicoCoreMX91",
		.bootdelay = __stringify(CONFIG_BOOTDELAY),
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
	{	/* 4 (BT_OSMSFMX91) */
		.name = "FS-OSM-SF-MX91",
		.bootdelay = __stringify(CONFIG_BOOTDELAY),
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
	{	/* 5 (BT_EFUSMX91) */
		.name = "efusMX91",
		.bootdelay = __stringify(CONFIG_BOOTDELAY),
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

/* ---- Stage 'f': RAM not valid, variables can *not* be used yet ---------- */

static int set_gd_board_type(void)
{
	struct fs_header_v1_0 *cfg_fsh;
	const char *board_id;
	const char *ptr;
	int len;

	cfg_fsh = fs_image_get_regular_cfg_addr();
	board_id = cfg_fsh->param.descr;
	ptr = strchr(board_id, '-');
	len = (int)(ptr - board_id);

	SET_BOARD_TYPE("PCoreMX93", BT_PICOCOREMX93, board_id, len);
	SET_BOARD_TYPE("OSM93", BT_OSMSFMX93, board_id, len);
	SET_BOARD_TYPE("efusMX93", BT_EFUSMX93, board_id, len);

	SET_BOARD_TYPE("OSM91", BT_OSMSFMX91, board_id, len);
	SET_BOARD_TYPE("efusMX91", BT_EFUSMX91, board_id, len);

	return -EINVAL;
}

#if CONFIG_IS_ENABLED(MULTI_DTB_FIT)
/* definition for U-BOOT */
int board_fit_config_name_match(const char *name)
{
	void *fdt;
	int offs;
	const char *board_fdt;

	fdt = fs_image_get_cfg_fdt();
	offs = fs_image_get_board_cfg_offs(fdt);
	board_fdt = fs_image_getprop(fdt, offs, 0, "board-fdt", NULL);

	if(board_fdt && !strncmp(name, board_fdt, 64))
		return 0;

	return -EINVAL;
}
#endif

static void fs_setup_cfg_info(void)
{
	void *fdt;
	int offs;
	int rev_offs;
	unsigned int features;
	struct cfg_info *info;
	const char *string;
	u32 flags = 0;

	/**
	 * If the BOARD-CFG cannot be found in OCRAM or it is corrupted, this
	 * is fatal. However no output is possible this early, so simply stop.
	 * If the BOARD-CFG is not at the expected location in OCRAM but is
	 * found somewhere else, output a warning later in board_late_init().
	 */
	if(!fs_image_find_cfg_in_ocram())
		hang();

	if (!fs_image_is_ocram_cfg_valid())
		hang();

	info = fs_board_get_cfg_info();
	memset(info, 0, sizeof(struct cfg_info));

	fdt = fs_image_get_cfg_fdt();
	offs = fs_image_get_board_cfg_offs(fdt);
	rev_offs = fs_image_get_board_rev_subnode_f(fdt, offs,
						    &info->board_rev);
	
	set_gd_board_type();
	info->board_type = gd->board_type;

	string = fs_image_getprop(fdt, offs, rev_offs, "boot-dev", NULL);
	info->boot_dev = fs_board_get_boot_dev_from_name(string);

	info->dram_chips = fs_image_getprop_u32(fdt, offs, rev_offs, 0,
						"dram-chips", 1);

	info->dram_size = fs_image_getprop_u32(fdt, offs, rev_offs, 0,
					       "dram-size", 0x400);

	info->flags = flags;

	features = 0;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-emmc", NULL))
		features |= FEAT_EMMC;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-ext-rtc", NULL))
		features |= FEAT_EXT_RTC;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eeprom", NULL))
		features |= FEAT_EEPROM;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eth-a", NULL))
		features |= FEAT_ETH_A;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eth-b", NULL))
		features |= FEAT_ETH_B;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eth-phy-a", NULL))
		features |= FEAT_ETH_PHY_A;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eth-phy-b", NULL))
		features |= FEAT_ETH_PHY_B;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-audio", NULL))
		features |= FEAT_AUDIO;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-wlan", NULL))
		features |= FEAT_WLAN;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sd-a;", NULL))
		features |= FEAT_SDIO_A;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sd-b;", NULL))
		features |= FEAT_SDIO_B;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-mipi-dsi", NULL))
		features |= FEAT_MIPI_DSI;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-mipi-csi", NULL))
		features |= FEAT_MIPI_CSI;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-lvds", NULL))
		features |= FEAT_LVDS;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-rgb", NULL))
		features |= FEAT_RGB;

	info->features = features;
}

int board_early_init_f(void)
{
	fs_setup_cfg_info();

	switch(gd->board_type) {
		case BT_PICOCOREMX93:
		case BT_PICOCOREMX91:
			imx_iomux_v3_setup_multiple_pads(lpuart2_pads, ARRAY_SIZE(lpuart2_pads));
			init_uart_clk(LPUART2_CLK_ROOT);
			break;
		case BT_OSMSFMX93:
		case BT_EFUSMX93:
		case BT_OSMSFMX91:
		case BT_EFUSMX91:
			imx_iomux_v3_setup_multiple_pads(lpuart1_pads, ARRAY_SIZE(lpuart1_pads));
			init_uart_clk(LPUART1_CLK_ROOT);
			break;
		default:
			return -EINVAL;
			break;
	}
	return 0;
}

static void fdt_fsboard_fixup(void *fdt)
{
	uint features = fs_board_get_features();

	switch(gd->board_type){
		case BT_PICOCOREMX91:
		case BT_PICOCOREMX93:
			if(!(features & FEAT_ETH_PHY_A)){
				fs_fdt_enable(fdt, "ethphy0", 0);
			}

			if(!(features & FEAT_ETH_PHY_B)){
				fs_fdt_enable(fdt, "ethphy1", 0);
			}

			if(!(features & FEAT_AUDIO)){
				fs_fdt_enable(fdt, "sound_sgtl5000", 0);
				fs_fdt_enable(fdt, "sgtl5000", 0);
			}

			if(!(features & FEAT_WLAN)){
				fs_fdt_enable(fdt, "wlan_wake", 0);
			}

			if(!(features & FEAT_SDIO_A))
				fs_fdt_enable(fdt, "pc_sdio_a", 0);

			if(!(features & (FEAT_SDIO_B | FEAT_WLAN)))
				fs_fdt_enable(fdt, "pc_sdio_b", 0);
			break;
		case BT_EFUSMX91:
		case BT_EFUSMX93:
			if(!(features & FEAT_WLAN)) {
				fs_fdt_enable(fdt, "mwifiex", 0);
				fs_fdt_enable(fdt, "wlan_wake", 0);
			}

			if(!(features & FEAT_ETH_PHY_A))
				fs_fdt_enable(fdt, "ethphy1", 0);

			if(!(features & FEAT_ETH_PHY_B))
				fs_fdt_enable(fdt, "ethphy2", 0);

			if(!(features & FEAT_SDIO_A))
				fs_fdt_enable(fdt, "fs_sdio_a", 0);

			if(!(features & (FEAT_SDIO_B | FEAT_WLAN)))
				fs_fdt_enable(fdt, "fs_sdio_b", 0);
			break;
		default:
			break;
	}
}

static void fdt_osm_fixup(void *fdt)
{
}

static void fdt_thermal_fixup(void *fdt, bool verbose)
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
		offs = fs_fdt_path_offset(fdt, LABEL_CPU_ALERT);
		fs_fdt_set_u32(fdt, offs, "temperature", tmp_val, 1, verbose);
		tmp_val = maxc * 1000;
		offs = fs_fdt_path_offset(fdt, LABEL_CPU_CRIT);
		fs_fdt_set_u32(fdt, offs, "temperature", tmp_val, 1, verbose);
	} else {
		printf("## Wrong cpu temp grade values read! Keeping defaults from device tree\n");
	}
}

static void fdt_common_fixup(void *fdt)
{
	uint features = fs_board_get_features();
	int ret;

	/* Realloc FDT-Blob to next full page-size.
	 * If NOSPACE Error appiers, increase extrasize.
	 */
	ret = fdt_shrink_to_minimum(fdt, 0x400);
	if(ret < 0){
		printf("failed to shrink FDT-Blob: %s\n", fdt_strerror(ret));
	}

	fdt_thermal_fixup(fdt, 0);

	if(!(features & FEAT_EMMC))
		fs_fdt_enable(fdt, "emmc", 0);

	if(!(features & FEAT_EXT_RTC))
		fs_fdt_enable(fdt, "rtc0", 0);

	if(!(features & FEAT_EEPROM))
		fs_fdt_enable(fdt, "eeprom", 0);

	if(!(features & FEAT_ETH_A))
		fs_fdt_enable(fdt, "ethernet0", 0);

	if(!(features & FEAT_ETH_B))
		fs_fdt_enable(fdt, "ethernet1", 0);

#if CONFIG_IS_ENABLED(IMX93)
	if(!(features & FEAT_MIPI_DSI)){
		fs_fdt_enable(fdt, "dsi", 0);
		fs_fdt_enable(fdt, "dphy", 0);
	}

	if(!(features & FEAT_LVDS)){
		fs_fdt_enable(fdt, "ldb", 0);
		fs_fdt_enable(fdt, "ldb_phy", 0);
	}
#endif
	
	if(!(features & FEAT_RGB)){
		fs_fdt_enable(fdt, "parallel_disp_fmt", 0);
	}

	if(!(features & (FEAT_LVDS | FEAT_MIPI_DSI | FEAT_RGB)))
		fs_fdt_enable(fdt, "lcdif", 0);

	switch(gd->board_type){
		case BT_PICOCOREMX91:
		case BT_PICOCOREMX93:
		case BT_EFUSMX91:
		case BT_EFUSMX93:
			fdt_fsboard_fixup(fdt);
			break;
		case BT_OSMSFMX91:
		case BT_OSMSFMX93:
			fdt_osm_fixup(fdt);
			break;
		default:
			break;
	}
}

#if CONFIG_IS_ENABLED(OF_BOARD_FIXUP)
int checkcpu()
{
	fdt_thermal_fixup((void *)gd->fdt_blob, 0);
	return 0;
}

int board_fix_fdt(void *fdt_blob)
{
	fdt_common_fixup(fdt_blob);
	return 0;
}
#endif

#if CONFIG_IS_ENABLED(OF_BOARD_SETUP)
int ft_board_setup(void *fdt_blob, struct bd_info *bd)
{
	u64 dram_base[CONFIG_NR_DRAM_BANKS];
	u64 dram_size[CONFIG_NR_DRAM_BANKS];
	int i, ret, offs;

	for(i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		dram_base[i] = gd->bd->bi_dram[i].start;
		dram_size[i] = gd->bd->bi_dram[i].size;
	}

	fdt_common_fixup(fdt_blob);
	ret = fdt_fixup_memory_banks(fdt_blob, dram_base, dram_size, CONFIG_NR_DRAM_BANKS);

	if(ret)
		return ret;

	/* fixup bdinfo */
	offs = fs_fdt_path_offset(fdt_blob, "/bdinfo");
	if (offs >= 0) {
		/* Set common bdinfo entries */
		fs_fdt_set_bdinfo(fdt_blob, offs);
	}

	return 0;
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

	/* Set MAC addresses as environment variables */
	switch (gd->board_type)
	{
	case BT_PICOCOREMX93:
	case BT_EFUSMX93:
	case BT_OSMSFMX93:
	case BT_PICOCOREMX91:
	case BT_EFUSMX91:
	case BT_OSMSFMX91:
		fs_eth_set_ethaddr(eth_id++);
		fs_eth_set_ethaddr(eth_id++);
		break;
	default:
		break;
	}
}

int board_init(void)
{
#if CONFIG_IS_ENABLED(FEC_MXC)
	setup_fec();
#endif

	/* Copy NBoot args to variables and prepare command prompt string */
	fs_board_init_common(&board_info[gd->board_type]);

	return 0;
}

static const char* fsimx93_get_board_name(void)
{
	return board_info[gd->board_type].name;
}

static void fsimx93_get_board_rev(char *str, int len)
{
	uint rev = fs_image_get_board_rev();

	snprintf(str, len, "REV%01d.%02d", rev / 100, rev % 100);
}

int board_late_init(void)
{
	enum boot_device boot_dev = get_boot_device();
	struct cfg_info *info = fs_board_get_cfg_info();
	void *fdt;
	int offs;
	const char *board_fdt;

	fdt = fs_image_get_cfg_fdt();
	offs = fs_image_get_board_cfg_offs(fdt);
	board_fdt = fs_image_getprop(fdt, offs, 0, "board-fdt", NULL);

	fs_image_set_board_id_from_cfg();

#if CONFIG_IS_ENABLED(ENV_IS_IN_MMC)
	board_late_mmc_env_init();
#endif

	if(board_fdt)
		env_set("platform", board_fdt);

	/* Set up all board specific variables */
	fs_board_late_init_common("ttyLP");	/* Set up all board specific variables */

	/* Set mac addresses for corresponding boards */
	fs_ethaddr_init();

	if(fs_board_is_closed())
		env_set("sec_boot", "yes");
	else
		env_set("sec_boot", "no");

	/* Skip autoboot during USB-Boot*/
	if(boot_dev == USB_BOOT || boot_dev == USB2_BOOT)
		env_set_ulong("bootdelay", 0);

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	char brev[MAX_DESCR_LEN] = {0};
	fsimx93_get_board_rev(brev, MAX_DESCR_LEN);

	env_set("board_name", fsimx93_get_board_name());
	env_set("board_rev", brev);
#endif

	debug("FEATURES=0x%x\n", info->features);
	return 0;
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
