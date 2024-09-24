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
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/imx8ulp-pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/pcc.h>
#include <asm/arch/sys_proto.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/gpio.h>
#include <i2c.h>
// #include <dm/uclass.h>
// #include <dm/uclass-internal.h>
#include <power-domain.h>
#include <dt-bindings/power/imx8ulp-power.h>
#include <fdt_support.h>
#include <hang.h>

#include "fsimx8ulp.h"
#include "../common/fs_board_common.h"
#include "../common/fs_eth_common.h"
#include "../common/fs_image_common.h"
#include "../common/fs_cntr_common.h"
#include "../common/fs_fdt_common.h"

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
	{	/* 0 (BT_PICOCOREMX8ULP) */
		.name = "PicoCoreMX8ULP",
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
	{	/* 1 (BT_OSMSFMX8ULP) */
		.name = "FS-OSM-SF-MX8ULP",
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
	{	/* 2 (BT_SOLDERCOREMX8ULP) */
		.name = "SolderCoreMX8ULP",
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
	{	/* 3 (BT_ARMSTONEMX8ULP) */
		.name = "armStoneMX8ULP",
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

	SET_BOARD_TYPE("PCoreMX8ULP", BT_PICOCOREMX8ULP, board_id, len);
	SET_BOARD_TYPE("OSMSFMX8ULP", BT_OSMSFMX8ULP, board_id, len);
	SET_BOARD_TYPE("SCoreMX8ULP", BT_SOLDERCOREMX8ULP, board_id, len);
	SET_BOARD_TYPE("ARMSTONEMX8ULP", BT_SOLDERCOREMX8ULP, board_id, len);

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
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eth", NULL))
		features |= FEAT_ETH;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-eth-phy", NULL))
		features |= FEAT_ETH_PHY;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-audio-apd", NULL))
		features |= FEAT_AUDIO_APD;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-audio-rtd", NULL))
		features |= FEAT_AUDIO_RTD;
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
	if(fs_image_getprop(fdt, offs, rev_offs, "have-rgb", NULL))
		features |= FEAT_RGB;

	info->features = features;
}

int board_early_init_f(void)
{
	fs_setup_cfg_info();

	return 0;
}

static void fdt_pcore_fixup(void *fdt)
{
}

static void fdt_osm_fixup(void *fdt)
{
}

static void fdt_astone_fixup(void *fdt)
{
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

	if(!(features & FEAT_EMMC))
		fs_fdt_enable(fdt, "emmc", 0);

	if(!(features & FEAT_EXT_RTC))
		fs_fdt_enable(fdt, "ext_rtc", 0);

	if(!(features & FEAT_EEPROM))
		fs_fdt_enable(fdt, "eeprom", 0);

	if(!(features & FEAT_ETH))
		fs_fdt_enable(fdt, "ethernet0", 0);

	if(!(features & FEAT_MIPI_DSI)){
		fs_fdt_enable(fdt, "dsi", 0);
		fs_fdt_enable(fdt, "dphy", 0);
	}

	if(!(features & (FEAT_RGB | FEAT_MIPI_DSI)))
		fs_fdt_enable(fdt, "dcnano", 0);

	if(gd->board_type == BT_PICOCOREMX8ULP)
		fdt_pcore_fixup(fdt);

	if(gd->board_type == BT_OSMSFMX8ULP)
		fdt_osm_fixup(fdt);

	if(gd->board_type == BT_ARMSTONEMX8ULP)
		fdt_astone_fixup(fdt);
}

#if CONFIG_IS_ENABLED(OF_BOARD_FIXUP)
int board_fix_fdt(void *fdt_blob)
{
	fdt_common_fixup(fdt_blob);
	return 0;
}
#endif

#if CONFIG_IS_ENABLED(OF_BOARD_SETUP)
int ft_board_setup(void *fdt_blob, struct bd_info *bd)
{
	fdt_common_fixup(fdt_blob);
	return fdt_fixup_memory(fdt_blob,
			CFG_SYS_SDRAM_BASE, gd->bd->bi_dram[0].size);
}
#endif

#if CONFIG_IS_ENABLED(FEC_MXC)
static int setup_fec(void)
{
	/* Select enet time stamp clock: 001 - External Timestamp Clock */
	cgc1_enet_stamp_sel(1);

	/* enable FEC PCC */
	pcc_clock_enable(4, ENET_PCC4_SLOT, true);
	pcc_reset_peripheral(4, ENET_PCC4_SLOT, false);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

void fs_ethaddr_init(void)
{
	int eth_id = 0;

	/* Set MAC addresses as environment variables */
	switch (gd->board_type)
	{
	case BT_PICOCOREMX8ULP:
	case BT_OSMSFMX8ULP:
	case BT_ARMSTONEMX8ULP:
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

static const char* fsimx8ulp_get_board_name(void)
{
	return board_info[gd->board_type].name;
}

int board_late_init(void)
{
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

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", fsimx8ulp_get_board_name());
	env_set("board_rev", "fsimx93");
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

void board_quiesce_devices(void)
{
	/* Disable the power domains may used in u-boot before entering kernel */
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	struct udevice *scmi_devpd;
	int ret, i;
	struct power_domain pd;
	ulong ids[] = {
		IMX8ULP_PD_FLEXSPI2, IMX8ULP_PD_USB0, IMX8ULP_PD_USDHC0,
		IMX8ULP_PD_USDHC1, IMX8ULP_PD_USDHC2_USB1, IMX8ULP_PD_DCNANO,
		IMX8ULP_PD_MIPI_DSI};

	ret = uclass_get_device(UCLASS_POWER_DOMAIN, 0, &scmi_devpd);
	if (ret) {
		printf("Cannot get scmi devpd: err=%d\n", ret);
		return;
	}

	pd.dev = scmi_devpd;

	for (i = 0; i < ARRAY_SIZE(ids); i++) {
		pd.id = ids[i];
		ret = power_domain_off(&pd);
		if (ret)
			printf("power_domain_off %lu failed: err=%d\n", ids[i], ret);
	}
#endif
}
