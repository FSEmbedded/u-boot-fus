// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright 2026 F&S Elektronik Systeme GmbH
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

#include <command.h>
#include <env.h>
#include <init.h>
#include <mmc.h>
#include <asm/global_data.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <linux/delay.h>
#include <power/regulator.h>
#include <scmi_agent.h>
#include <scmi_nxp_protocols.h>
#include "../dts/upstream/src/arm64/freescale/imx95-power.h"
#include <asm/gpio.h>
#include <fdt_support.h>
#include <hang.h>

#include "fsimx95.h"
#include "../common/fs_board_common.h"
#include "../common/fs_eth_common.h"
#include "../common/fs_image_common.h"
#include "../common/fs_cntr_common.h"
#include "../common/fs_fdt_common.h"

DECLARE_GLOBAL_DATA_PTR;

extern int board_fix_fdt_fuse(void *fdt);

/* +++ Environment defines +++ */

#define INSTALL_RAM "ram@94800000"

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
	{	/* 0 (BT_FSSM95S) */
		.name = "FSSM95S",
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
	{	/* 1 (BT_PICOCOREMX95) */
		.name = "PicoCoreMX95",
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

	SET_BOARD_TYPE("SM95S", BT_FSSM95S, board_id, len);
	SET_BOARD_TYPE("PC95S", BT_PICOCOREMX95, board_id, len);

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
	if(fs_image_getprop(fdt, offs, rev_offs, "have-bt", NULL))
		features |= FEAT_BT;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sd-a", NULL))
		features |= FEAT_SDIO_A;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sd-b", NULL))
		features |= FEAT_SDIO_B;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sdio-c", NULL))
		features |= FEAT_SDIO_C;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-mipi-dsi", NULL))
		features |= FEAT_MIPI_DSI;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-mipi-csi", NULL))
		features |= FEAT_MIPI_CSI;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-lvds", NULL))
		features |= FEAT_LVDS;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-usb-hub", NULL))
		features |= FEAT_USB_HUB;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-temp", NULL))
		features |= FEAT_TEMP;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sec", NULL))
		features |= FEAT_SEC;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-gpio-exp", NULL))
		features |= FEAT_GPIO_EXP;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-edp", NULL))
		features |= FEAT_EDP;

	info->features = features;
}

int board_early_init_f(void)
{
	fs_setup_cfg_info();

	switch(gd->board_type) {
		case BT_FSSM95S:
			init_uart_clk(0);
			break;
		case BT_PICOCOREMX95:
			init_uart_clk(6);
			break;
		default:
			return -EINVAL;
			break;
	}

	return 0;
}

static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	return scmi_pwd_state_set(dev, 0, domain, enable ? 0 : BIT(30));
}

static void netc_phy_rst_io(const char *gpio_name, const char *label)
{
	int ret;
	struct gpio_desc desc;

	/* ENET_RST_B */
	ret = dm_gpio_lookup_name(gpio_name, &desc);
	if (ret) {
		printf("%s lookup %s failed ret = %d\n", __func__, gpio_name, ret);
		return;
	}

	ret = dm_gpio_request(&desc, label);
	if (ret) {
		printf("%s request %s failed ret = %d\n", __func__, label, ret);
		return;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	/* assert ETH_x_PHY_RST for 10ms*/
	dm_gpio_set_value(&desc, 0);
	udelay(10000);

	/* deassert ETH_x_PHY_RST again */
	dm_gpio_set_value(&desc, 1);

	/* Wait 100ms before accessing MDIO registers */
	udelay(100000);
}

static int scmi_brd_ctrlset(uint32_t ctrlId, uint32_t numval, uint32_t *val)
{
	struct udevice *dev;
	struct scmi_imx_misc_control_set_in msg_in = { 0 };
	s32 status = 0;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_IMX_MISC, \
										SCMI_IMX_MISC_CONTROL_SET, \
										msg_in, status);
	int i, ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret) {
		printf("%s: Failed to get udev\n", __func__);
		return ret;
	}

	if (numval >= MISC_MAX_VAL_T)
		return -EINVAL;

	msg_in.ctrlid = SCMI_BRD_FLAG | ctrlId;
	msg_in.numval = numval;
	for (i = 0; i < numval; i++)
		msg_in.val[i] = val[i];

	ret = devm_scmi_process_msg(dev, &msg);
	if (ret != 0 || status != 0) {
		printf("ERROR: Failed to set SCMI Control %d\n", ctrlId);
		printf("ret = %d, status=%d\n", ret, status);
		return -EINVAL;
	}

	return 0;
}

static void netc_phy_rst_scmi(void) {
	uint32_t value;

	/* Perform Phy Reset */
	value = 0;
	scmi_brd_ctrlset(SCMI_FUS_MISC_IO_ETH_A_PHY_RST, 1, &value);
	scmi_brd_ctrlset(SCMI_FUS_MISC_IO_ETH_B_PHY_RST, 1, &value);

	udelay(10000);

	value = 1;
	scmi_brd_ctrlset(SCMI_FUS_MISC_IO_ETH_A_PHY_RST, 1, &value);
	scmi_brd_ctrlset(SCMI_FUS_MISC_IO_ETH_B_PHY_RST, 1, &value);

	/* Wait 100ms before accessing MDIO registers */
	udelay(100000);
}

void netc_init(void)
{
	int ret;

	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, false);
	udelay(10000);

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

	/* Reset PHYs */
	switch (gd->board_type)
	{
	case BT_FSSM95S:
		netc_phy_rst_io("gpio@21_5", "eth_a_phy_rst");
		netc_phy_rst_io("gpio@21_6", "eth_b_phy_rst");
		break;
	case BT_PICOCOREMX95:
		netc_phy_rst_scmi();
		break;
	default:
		break;
	}
}

void fs_ethaddr_init(void)
{
	int eth_id = 0;

	/* Set MAC addresses as environment variables */
	switch (gd->board_type)
	{
	case BT_FSSM95S:
	case BT_PICOCOREMX95:
		fs_eth_set_ethaddr(eth_id++);
		fs_eth_set_ethaddr(eth_id++);
		break;
	default:
		break;
	}
}

int board_init(void)
{
	int ret;
	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for USB\n");
		return ret;
	}

	imx9_scmi_power_domain_enable(IMX95_PD_DISPLAY, false);
	imx9_scmi_power_domain_enable(IMX95_PD_CAMERA, false);

	netc_init();

	//power_on_m7("mx95evkrpmsg");

	/* Copy NBoot args to variables and prepare command prompt string */
	fs_board_init_common(&board_info[gd->board_type]);

	return 0;
}

static const char* fsimx95_get_board_name(void)
{
	return board_info[gd->board_type].name;
}

static void fsimx95_get_board_rev(char *str, int len)
{
	uint rev = fs_image_get_board_rev();

	snprintf(str, len, "REV%01d.%02d", rev / 100, rev % 100);
}

int mmc_map_to_kernel_blk(int devno)
{
	return devno;
}

void board_late_mmc_env_init(void)
{
	char cmd[32];
	char mmcblk[32];
	u32 dev_no = mmc_get_env_dev();

	env_set_ulong("mmcdev", dev_no);

	/**
	 * TODO: consider F&S U-BOOT-ENV $rootfs_partition_mmc
	 * This section will be replaced
	*/
	sprintf(mmcblk, "/dev/mmcblk%dp2 rootwait rw", mmc_map_to_kernel_blk(dev_no));
	env_set("mmcroot", mmcblk);

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
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

	/* Skip autoboot during USB-Boot*/
	if(boot_dev == USB_BOOT || boot_dev == USB2_BOOT)
		env_set_ulong("bootdelay", 0);

#if !CONFIG_IS_ENABLED(FUS_FORCE_DEFAULT_BOOTDELAY)
	/* Disable Shell access, if board is closed*/
	if(fs_board_is_closed()){

		env_set_ulong("bootdelay", -2);
		/* TODO: Maybe check images within all boot commands? */
		if (boot_dev == USB_BOOT || boot_dev == USB2_BOOT){
			printf("WARNING: USB Boot detected on closed board!\n");
			printf("\tEnable FASTBOOT access with CONFIG_FUS_FORCE_DEFAULT_BOOTDELAY\n");
			hang();
		}
	}
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	char brev[MAX_DESCR_LEN] = {0};
	fsimx95_get_board_rev(brev, MAX_DESCR_LEN);

	env_set("board_name", fsimx95_get_board_name());
	env_set("board_rev", brev);
#endif

	debug("FEATURES=0x%x\n", info->features);
	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	char *p, *b, *s;
	char *token = NULL;
	int i, ret = 0;
	u64 base[CONFIG_NR_DRAM_BANKS] = {0};
	u64 size[CONFIG_NR_DRAM_BANKS] = {0};

	p = env_get("jh_root_mem");
	if (!p)
		return 0;

	i = 0;
	token = strtok(p, ",");
	while (token) {
		if (i >= CONFIG_NR_DRAM_BANKS) {
			printf("Error: The number of size@base exceeds CONFIG_NR_DRAM_BANKS.\n");
			return -EINVAL;
		}

		b = token;
		s = strsep(&b, "@");
		if (!s) {
			printf("The format of jh_root_mem is size@base[,size@base...].\n");
			return -EINVAL;
		}
		base[i] = simple_strtoull(b, NULL, 16);
		size[i] = simple_strtoull(s, NULL, 16);
		token = strtok(NULL, ",");
		i++;
	}

	ret = fdt_fixup_memory_banks(blob, base, size, CONFIG_NR_DRAM_BANKS);
	if (ret)
		return ret;

	return 0;
}
#endif

void board_quiesce_devices(void)
{
	int ret;

	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, false);
	if (ret) {
		printf("%s: Failed for HSIO MIX: %d\n", __func__, ret);
		return;
	}

	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, false);
	if (ret) {
		printf("%s: Failed for NETC MIX: %d\n", __func__, ret);
		return;
	}
}

#if IS_ENABLED(CONFIG_OF_BOARD_FIXUP)
static void disable_fdt_resources(void *fdt)
{
	int i = 0;
	int nodeoff, ret;
	const char *status = "disabled";
	static const char * const dsi_nodes[] = {
		"/soc/bus@42000000/i2c@426b0000",
		"/soc/bus@42000000/i2c@426d0000",
		"/soc/system-controller@4cde0000"
	};

	for (i = 0; i < ARRAY_SIZE(dsi_nodes); i++) {
		nodeoff = fdt_path_offset(fdt, dsi_nodes[i]);
		if (nodeoff > 0) {
set_status:
			ret = fdt_setprop(fdt, nodeoff, "status", status,
					  strlen(status) + 1);
			if (ret == -FDT_ERR_NOSPACE) {
				ret = fdt_increase_size(fdt, 512);
				if (!ret)
					goto set_status;
			}
		}
	}
}

static int board_fix_19x19_evk(void *fdt)
{
	char cfgname[SCMI_MISC_MAX_CFGNAME];
	u32 msel;
	int ret;
	const char *netcfg = "mx95netc";

	ret = scmi_misc_cfginfo(&msel, cfgname);
	if (!ret) {
		debug("SM: %s\n", cfgname);
		if (!strcmp(netcfg, cfgname))
			disable_fdt_resources(fdt);
	}

	return 0;
}

int board_fix_fdt(void *fdt)
{
	/* Remove nodes based on fuses. */
	board_fix_fdt_fuse(fdt);
	
	return board_fix_19x19_evk(fdt);
}
#endif
#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
