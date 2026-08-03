// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <env.h>
#include <efi_loader.h>
#include <init.h>
#include <mmc.h>
#include <asm/global_data.h>
#include <fdt_support.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <usb.h>
#include "../common/tcpc.h"
#include <dwc3-uboot.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <power/regulator.h>
#include <scmi_agent.h>
#include "../dts/upstream/src/arm64/freescale/imx95-power.h"
#include <i2c.h>
#include <asm/arch/sys_proto.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
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
	{	/* 0 (BT_IMX95EVK) */
		.name = "iMX95EVK",
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

#ifndef CONFIG_SPL_BUILD
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

	SET_BOARD_TYPE("iMX95EVK", BT_IMX95EVK, board_id, len);

	return -EINVAL;
}

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
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sd-a", NULL))
		features |= FEAT_SDIO_A;
	if(fs_image_getprop(fdt, offs, rev_offs, "have-sd-b", NULL))
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
#endif

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x2c4db6b3, 0x0b15, 0x4a36, 0xbe, 0xae, \
		 0x1e, 0xa1, 0x35, 0x46, 0x4f, 0x5b)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX95-EVK-RAW",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=flash-bin raw 0 0x2000 mmcpart 1",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_early_init_f(void)
{
#ifndef CONFIG_SPL_BUILD
	fs_setup_cfg_info();
#endif

	/* UART1: A55, UART2: M33, UART3: M7 */
	init_uart_clk(0);

	return 0;
}

#ifdef CONFIG_USB_TCPC
struct tcpc_port port;
#ifdef CONFIG_TARGET_IMX95_15X15_EVK
struct tcpc_port portpd;
struct tcpc_port_config port_config = {
	.i2c_bus = 2, /* i2c3 */
	.addr = 0x50,
	.port_type = TYPEC_PORT_DRP,
	.disable_pd = true,
};

struct tcpc_port_config portpd_config = {
	.i2c_bus = 2, /*i2c3*/
	.addr = 0x52,
	.port_type = TYPEC_PORT_UFP,
	.max_snk_mv = 20000,
	.max_snk_ma = 3000,
	.max_snk_mw = 15000,
	.op_snk_mv = 9000,
};
#else
struct tcpc_port_config port_config = {
	.i2c_bus = 6, /* i2c7 */
	.addr = 0x50,
	.port_type = TYPEC_PORT_DRP,
	.disable_pd = true,
};
#endif

ulong tca_base;

void tca_mux_select(enum typec_cc_polarity pol)
{
	u32 val;

	if (!tca_base)
		return;

	/* Set OP mode to System configure Mode */
	clrbits_le32(tca_base + 0x10, 0x3);

	val = readl(tca_base + 0x30);

	setbits_le32(tca_base + 0x18, BIT(3));
	udelay(1);

	if (pol == TYPEC_POLARITY_CC1)
		clrbits_le32(tca_base + 0x18, BIT(2));
	else
		setbits_le32(tca_base + 0x18, BIT(2));

	udelay(1);

	clrbits_le32(tca_base + 0x18, BIT(3));
}

static void setup_typec(void)
{
	int ret;

	tca_base = USB1_BASE_ADDR + 0xfc000;

#ifdef CONFIG_TARGET_IMX95_15X15_EVK
	struct gpio_desc ext_12v_desc;

	ret = tcpc_init(&portpd, portpd_config, NULL);
	if (ret) {
		printf("%s: tcpc portpd init failed, err=%d\n",
		       __func__, ret);
	} else if (tcpc_pd_sink_check_charging(&portpd)) {
		printf("Power supply on USB PD\n");

		/* Enable EXT 12V */
		ret = dm_gpio_lookup_name("gpio@22_1", &ext_12v_desc);
		if (ret) {
			printf("%s lookup gpio@22_1 failed ret = %d\n", __func__, ret);
			return;
		}

		ret = dm_gpio_request(&ext_12v_desc, "ext_12v_en");
		if (ret) {
			printf("%s request ext_12v_en failed ret = %d\n", __func__, ret);
			return;
		}

		/* Enable PER 12V regulator */
		dm_gpio_set_dir_flags(&ext_12v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	}
#endif

	ret = tcpc_init(&port, port_config, &tca_mux_select);
	if (ret) {
		printf("%s: tcpc init failed, err=%d\n", __func__, ret);
		return;
	}
}
#endif

static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	return scmi_pwd_state_set(dev, 0, domain, enable ? 0 : BIT(30));
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0 && init == USB_INIT_DEVICE) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_ufp_mode(&port);
		if (ret)
			return ret;
#endif
	} else if (index == 0 && init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_dfp_mode(&port);
#endif
		return ret;
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	if (index == 0 && init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_disable_src_vbus(&port);
#endif
	}

	return ret;
}

static void netc_phy_rst(const char *gpio_name, const char *label)
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

	/* assert the ENET_RST_B */
	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE | GPIOD_ACTIVE_LOW);
	udelay(10000);
	dm_gpio_set_value(&desc, 0); /* deassert the ENET_RST_B */
	udelay(80000);

}

static void __maybe_unused netc_regulator_enable(const char *devname, bool enable)
{
	int ret;
	struct udevice *dev;

	ret = regulator_get_by_devname(devname, &dev);
	if (ret) {
		printf("Get %s regulator failed %d\n", devname, ret);
		return;
	}

	ret = regulator_set_enable_if_allowed(dev, enable);
	if (ret) {
		printf("%s %s regulator %d\n",
			enable ? "Enable": "Disable", devname, ret);
		return;
	}
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

#ifdef CONFIG_TARGET_IMX95_15X15_EVK
	netc_phy_rst("gpio@22_4", "ENET1_RST_B");
	netc_phy_rst("gpio@22_5", "ENET2_RST_B");
#else
	netc_phy_rst("i2c5_io@21_2", "ENET1_RST_B");

	/* Enable in SW count */
	netc_regulator_enable("regulator-aqr-stby", true);
	netc_regulator_enable("regulator-mac-stby", true);
	netc_regulator_enable("regulator-aqr-en", true);
	netc_regulator_enable("regulator-mac-en", true);

	/* Disable regulator to have explicit reset to AQR PHY and clock generator */
	udelay(10000);
	netc_regulator_enable("regulator-aqr-stby", false);
	netc_regulator_enable("regulator-mac-stby", false);
	netc_regulator_enable("regulator-aqr-en", false);
	netc_regulator_enable("regulator-mac-en", false);

	udelay(10000);
	netc_regulator_enable("regulator-aqr-stby", true);
	netc_regulator_enable("regulator-mac-stby", true);

#endif
}

static void flexspi_nor_steup(void)
{
	struct gpio_desc desc;
	int ret;

	if (!IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK))
		return;

	/* Power cycle the M2 3V3 */
	ret = dm_gpio_lookup_name("gpio@22_10", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "M2_PWREN");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(100000);
	dm_gpio_set_value(&desc, 1);

	/* Enable 1.8V LDO */
	ret = dm_gpio_lookup_name("gpio@22_11", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "M2_DIS1");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);

	/* Deassert M2_SD3_nRST */
	ret = dm_gpio_lookup_name("GPIO5_9", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "M2_SD3_nRST");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);
}

void lvds_backlight_on(void)
{
	struct udevice *dev;
	int ret;
	u8 reg;

	if (!IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK))
		return;

	ret = i2c_get_chip_for_busnum(2, 0x62, 1, &dev);
	if (ret) {
		printf("%s: Cannot find pca9632 led dev\n",
		       __func__);
		return;
	}

	reg = 1;
	dm_i2c_write(dev, 0x1, &reg, 1);

	reg = 5;
	dm_i2c_write(dev, 0x8, &reg, 1);
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

#if defined(CONFIG_USB_TCPC)
	setup_typec();
#endif

#if IS_ENABLED(CONFIG_TARGET_IMX95_19X19_EVK)
	netc_regulator_enable("regulator-m2-pwr", true);
#endif

	netc_init();

	flexspi_nor_steup();

	power_on_m7("mx95evkrpmsg");

	lvds_backlight_on();

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

void fs_ethaddr_init(void)
{
	int eth_id = 0;

	/* Set MAC addresses as environment variables */
	switch (gd->board_type)
	{
	case BT_IMX95EVK:
		fs_eth_set_ethaddr(eth_id++);
		fs_eth_set_ethaddr(eth_id++);
		break;
	default:
		break;
	}
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

	if (IS_ENABLED(CONFIG_ENV_IS_IN_MMC))
		board_late_mmc_env_init();

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
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
	struct uclass *uc_dev;

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

	ret = uclass_get(UCLASS_SPI_FLASH, &uc_dev);
	if (uc_dev)
		ret = uclass_destroy(uc_dev);
	if (ret)
		printf("couldn't remove SPI FLASH devices\n");
}

#if IS_ENABLED(CONFIG_OF_BOARD_FIXUP)

#if IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK)
static void change_fdt_mido_pins(void *fdt)
{
	int nodeoff, ret;
	u32 enet1_pins[12] = { 0x00B8, 0x02BC, 0x0424, 0x00, 0x00, 0x57e,
		0x00BC, 0x02C0, 0x0428, 0x00, 0x00, 0x97e};

	nodeoff = fdt_path_offset(fdt, "/firmware/scmi/protocol@19/emdiogrp");
	if (nodeoff > 0) {

		int i;
		for (i = 0; i < 12; i++) {
			enet1_pins[i] = cpu_to_fdt32(enet1_pins[i]);
		}

		ret = fdt_setprop(fdt, nodeoff, "fsl,pins", enet1_pins, 12 * sizeof(u32));
		if (ret)
			printf("fdt_setprop fsl,pins error %d\n", ret);
		else
			debug("Update MDIO pins ok\n");
	}
}

static int board_fix_15x15_evk(void *fdt)
{
	int ret;
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;

	ret = uclass_get_device_by_seq(UCLASS_I2C, 2, &bus);
	if (ret) {
		printf("%s: Can't find I2C bus 2\n", __func__);
		return 0;
	}

	ret = dm_i2c_probe(bus, 0x50, 0, &i2c_dev);
	if (ret) {
		ret = dm_i2c_probe(bus, 0x20, 0, &i2c_dev);
		if (!ret) {
			debug("Find Audio board\n");
			change_fdt_mido_pins(fdt);
		}
	}

	return 0;
}

#else
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
#endif

int board_fix_fdt(void *fdt)
{
	/* Remove nodes based on fuses. */
	board_fix_fdt_fuse(fdt);
	
#if IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK)
	return board_fix_15x15_evk(fdt);
#else
	return board_fix_19x19_evk(fdt);
#endif
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
