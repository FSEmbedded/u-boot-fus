/*
 * fsimx8mm.c
 *
 * (C) Copyright 2020
 * Patrik Jakob, F&S Elektronik Systeme GmbH, jakob@fs-net.de
 * Anatol Derksen, F&S Elektronik Systeme GmbH, derksen@fs-net.de
 * Philipp Gerbach, F&S Elektronik Systeme GmbH, gerbach@fs-net.de
 *
 * Board specific functions for F&S boards based on Freescale i.MX8MM CPU
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <i2c.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <spl.h>
#include <asm/mach-imx/dma.h>
#include <power/bd71837.h>
#ifdef CONFIG_USB_TCPC
#include "../common/tcpc.h"
#endif
#include <usb.h>

#include <environment.h>		/* enum env_operation */
#include <serial.h>			/* get_serial_device() */
#include "../common/fs_fdt_common.h"	/* fs_fdt_set_val(), ... */
#include "../common/fs_board_common.h"	/* fs_board_*() */
#include "../common/fs_eth_common.h"	/* fs_eth_*() */
#include "../common/fs_image_common.h"	/* fs_image_*() */
#include <nand.h>

#include <power/regulator.h>

/* ------------------------------------------------------------------------- */

DECLARE_GLOBAL_DATA_PTR;

#define BT_PICOCOREMX8MM 	0
#define BT_PICOCOREMX8MX	1
#define BT_TBS2 		2

/* Board features; these values can be resorted and redefined at will */
#define FEAT_ETH_A	(1<<0)
#define FEAT_ETH_B	(1<<1)
#define FEAT_ETH_A_PHY	(1<<2)
#define FEAT_ETH_B_PHY	(1<<3)
#define FEAT_NAND	(1<<4)
#define FEAT_EMMC	(1<<5)
#define FEAT_SGTL5000	(1<<6)
#define FEAT_WLAN	(1<<7)
#define FEAT_LVDS	(1<<8)
#define FEAT_MIPI_DSI	(1<<9)
#define FEAT_RTC85063	(1<<10)
#define FEAT_RTC85263	(1<<11)
#define FEAT_SEC_CHIP	(1<<12)
#define FEAT_CAN	(1<<13)
#define FEAT_EEPROM	(1<<14)

#define FEAT_ETH_MASK 	(FEAT_ETH_A | FEAT_ETH_B)

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)
#define ENET_PAD_CTRL ( \
		PAD_CTL_PUE |	\
		PAD_CTL_DSE6   | PAD_CTL_HYS)

#define INSTALL_RAM "ram@43800000"
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
	{	/* 0 (BT_PICOCOREMX8MM) */
		.name = "PicoCoreMX8MM-LPDDR4",
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
	{	/* 1 (BT_PICOCOREMX8MX) */
		.name = "PicoCoreMX8MM-DDR3L",
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
	{	/* 2 (BT_TBS2) */
		.name = "TBS2",
		.bootdelay = "3",
		.updatecheck = "mmc0,mmc2",
		.installcheck = INSTALL_DEF,
		.recovercheck = UPDATE_DEF,
		.console = ".console_serial",
		.login = ".login_serial",
		.mtdparts = ".mtdparts_std",
		.network = ".network_off",
		.init = INIT_DEF,
		.flags = 0,
	},
	{	/* (last) (unknown board) */
		.name = "unknown",
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

/* ---- Stage 'f': RAM not valid, variables can *not* be used yet ---------- */

#ifdef CONFIG_NAND_MXS
//###static void setup_gpmi_nand(void);
#endif

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

/* Parse the FDT of the BOARD-CFG in OCRAM and create binary info in OCRAM */
static void fs_spl_setup_cfg_info(void)
{
	void *fdt = fs_image_get_cfg_addr(false);
	int offs = fs_image_get_cfg_offs(fdt);
	int i;
	struct cfg_info *cfg = fs_board_get_cfg_info();
	const char *tmp;
	unsigned int features;

	memset(cfg, 0, sizeof(struct cfg_info));

	//###nbootargs.dwDbgSerPortPA = UART1_BASE_ADDR;

	tmp = fdt_getprop(fdt, offs, "board-name", NULL);
	for (i = 0; i < ARRAY_SIZE(board_info) - 1; i++) {
		if (!strcmp(tmp, board_info[i].name))
			break;
	}
	cfg->board_type = i;

	tmp = fdt_getprop(fdt, offs, "boot-dev", NULL);
	cfg->boot_dev = fs_board_get_boot_dev_from_name(tmp);

	cfg->board_rev = fdt_getprop_u32_default_node(fdt, offs, 0,
						      "board-rev", 100);
	cfg->dram_chips = fdt_getprop_u32_default_node(fdt, offs, 0,
						       "dram-chips", 1);
	cfg->dram_size = fdt_getprop_u32_default_node(fdt, offs, 0,
						      "dram-size", 0x400);

	features = 0;
	if (fdt_getprop(fdt, offs, "have-nand", NULL))
		features |= FEAT_NAND;
	if (fdt_getprop(fdt, offs, "have-emmc", NULL))
		features |= FEAT_EMMC;
	if (fdt_getprop(fdt, offs, "have-sgtl5000", NULL))
		features |= FEAT_SGTL5000;
	if (fdt_getprop(fdt, offs, "have-eth-phy", NULL)) {
		features |= FEAT_ETH_A;
		if (cfg->board_type == BT_PICOCOREMX8MX)
			features |= FEAT_ETH_B;
	}
	if (fdt_getprop(fdt, offs, "have-wlan", NULL))
		features |= FEAT_WLAN;
	if (fdt_getprop(fdt, offs, "have-lvds", NULL))
		features |= FEAT_LVDS;
	if (fdt_getprop(fdt, offs, "have-mipi-dsi", NULL))
		features |= FEAT_MIPI_DSI;
	if (fdt_getprop(fdt, offs, "have-rtc-pcf85063", NULL))
		features |= FEAT_RTC85063;
	if (fdt_getprop(fdt, offs, "have-rtc-pcf85263", NULL))
		features |= FEAT_RTC85263;
	if (fdt_getprop(fdt, offs, "have-security", NULL))
		features |= FEAT_SEC_CHIP;
	if (fdt_getprop(fdt, offs, "have-can", NULL))
		features |= FEAT_CAN;
	if (fdt_getprop(fdt, offs, "have-eeprom", NULL))
		features |= FEAT_EEPROM;
	cfg->features = features;
}

/* Do some very early board specific setup */
int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs*) WDOG1_BASE_ADDR;

	fs_spl_setup_cfg_info();

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

#ifdef CONFIG_NAND_MXS
//###	setup_gpmi_nand(); /* SPL will call the board_early_init_f */
#endif

	return 0;
}

/* Return the appropriate environment depending on the fused boot device */
enum env_location env_get_location(enum env_operation op, int prio)
{
	if (prio == 0) {
		switch (fs_board_get_boot_dev()) {
		case NAND_BOOT:
			return ENVL_NAND;
		case MMC3_BOOT:
			return ENVL_MMC;
		default:
			break;
		}
	}

	return ENVL_UNKNOWN;
}

/* Check board type */
int checkboard(void)
{
	unsigned int board_type = fs_board_get_type();
	unsigned int board_rev = fs_board_get_rev();
	unsigned int features = fs_board_get_features();

	printf ("Board: %s Rev %u.%02u (", board_info[board_type].name,
		board_rev / 100, board_rev % 100);
	if ((features & FEAT_ETH_MASK) == FEAT_ETH_MASK)
		puts ("2x ");
	if (features & FEAT_ETH_MASK)
		puts ("LAN, ");
	if (features & FEAT_WLAN)
		puts ("WLAN, ");
	if (features & FEAT_EMMC)
		puts ("eMMC, ");
	if (features & FEAT_NAND)
		puts("NAND, ");

	printf ("%dx DRAM)\n", fs_board_get_cfg_info()->dram_chips);

	return 0;
}

/* ---- Stage 'r': RAM valid, U-Boot relocated, variables can be used ------ */
#ifdef CONFIG_USB_TCPC
#define  USB_INIT_UNKNOWN (USB_INIT_DEVICE + 1)
static int setup_typec(void);
#endif
static int setup_fec(void);
void fs_ethaddr_init(void);
static int board_setup_ksz9893r(void);

int board_init(void)
{
	unsigned int board_type = fs_board_get_type();

	/* Prepare command prompt string */
	fs_board_init_common(&board_info[board_type]);

#ifdef CONFIG_USB_TCPC
	setup_typec();
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

	if (board_type == BT_PICOCOREMX8MX) {
		board_setup_ksz9893r();
	}

/* TODO KM: Is this generally a better way to initialize all the fixed GPIOs? */

	/* Enable regulators defined in device tree */
	regulators_enable_boot_on(false);

	return 0;
}

extern int mxs_nand_register(struct nand_chip *nand);

int board_nand_init(struct nand_chip *nand)
{
	if (fs_board_get_features() & FEAT_NAND)
		return mxs_nand_register(nand);

	return -ENODEV;
}

/* Return the HW partition where U-Boot environment is on eMMC */
unsigned int mmc_get_env_part(struct mmc *mmc)
{
	unsigned int boot_part;

	boot_part = (mmc->part_config >> 3) & PART_ACCESS_MASK;
	if (boot_part == 7)
		boot_part = 0;

	return boot_part;
}

#ifdef CONFIG_USB_TCPC
struct tcpc_port port;

struct tcpc_port_config port_config = {
	.i2c_bus = 0,
	.addr = 0x52,
	.port_type = TYPEC_PORT_UFP,
	.max_snk_mv = 5000,
	.max_snk_ma = 3000,
	.max_snk_mw = 40000,
	.op_snk_mv = 9000,
	.switch_setup_func = NULL,
};

static int setup_typec(void)
{
	int ret;

	switch (fs_board_get_type())
	{
	case BT_PICOCOREMX8MM:
		port_config.i2c_bus = 3;
		break;
	case BT_PICOCOREMX8MX:
		port_config.i2c_bus = 0;
		break;
	}

	debug("tcpc_init port\n");
	ret = tcpc_init(&port, port_config, NULL);
	if (ret) {
		port.i2c_dev = NULL;
	}
	return ret;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;
	struct tcpc_port *port_ptr = &port;

	debug("board_usb_init %d, type %d\n", index, init);

	imx8m_usb_power(index, true);

	if (index == 0) {
		if (port.i2c_dev) {
			if (init == USB_INIT_HOST)
				tcpc_setup_dfp_mode(port_ptr);
			else
				tcpc_setup_ufp_mode(port_ptr);
		}
	}

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	struct tcpc_port *port_ptr = &port;

	debug("board_usb_cleanup %d, type %d\n", index, init);

	if (index == 0) {
		if (port.i2c_dev) {
			if (init == USB_INIT_HOST)
				ret = tcpc_disable_src_vbus(port_ptr);
		}
	}

	imx8m_usb_power(index, false);
	return ret;
}

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int ret = 0;
	enum typec_cc_polarity pol;
	enum typec_cc_state state;
	struct tcpc_port *port_ptr = &port;

	if (port.i2c_dev) {
		if (dev->seq == 0) {

			tcpc_setup_ufp_mode(port_ptr);

			ret = tcpc_get_cc_status(port_ptr, &pol, &state);
			if (!ret) {
				if (state == TYPEC_STATE_SRC_RD_RA || state == TYPEC_STATE_SRC_RD)
					return USB_INIT_HOST;
			}
		}

		return USB_INIT_DEVICE;
	}
	else
		return USB_INIT_UNKNOWN;
}
#else
/*
 * USB Host support.
 *
 * USB0 is OTG. By default this is used as device port. However on some F&S
 * boards this port may optionally be configured as a second host port. So if
 * environment variable usb0mode is set to "host" on these boards, or if it is
 * set to "otg" and the ID pin is low when usb is started, use host mode.
 *
 *    Board               USB_OTG_PWR              USB_OTG_ID
 *    --------------------------------------------------------------------
 *    PicoCoreMX8MM       GPIO1_12 (GPIO1_IO12)(*) -
 *    PicoCoreMX8MN       GPIO1_12 (GPIO1_IO12)(*) -
 *    TBS2                GPIO1_12 (GPIO1_IO12)    -
 *
 * (*) Signal on SKIT is active low, usually USB_OTG_PWR is active high
 *
 * USB1 is a host-only port (USB_H1). It is used on all boards. Some boards
 * may have an additional USB hub with a reset signal connected to this port.
 *
 *    Board               USB_H1_PWR               Hub Reset
 *    -------------------------------------------------------------------------
 *    PicoCoreMX8MM       GPIO1_14 (GPIO1_IO14)(*) -
 *    PicoCoreMX8MN       GPIO1_14 (GPIO1_IO14)(*) -
 *    TBS2                -                        -
 *
 * (*) Signal on SKIT is active low, usually USB_HOST_PWR is active high
 *
 * The polarity for the VBUS power can be set with environment variable
 * usbxpwr, where x is the port index (0 or 1). If this variable is set to
 * "low", the power pin is active low, if it is set to "high", the power pin
 * is active high. Default is board-dependent, so that when F&S SKITs are
 * used, only usbxmode must be set.
 *
 * Example: setenv usb1pwr low
 *
 * Usually the VBUS power for a host port is connected to a dedicated pin, i.e.
 * USB_H1_PWR or USB_OTG_PWR. Then the USB controller can switch power
 * automatically and we only have to tell the controller whether this signal is
 * active high or active low. In all other cases, VBUS power is simply handled
 * by a regular GPIO.
 *
 * If CONFIG_FS_USB_PWR_USBNC is set, the dedicated PWR function of the USB
 * controller will be used to switch host power (where available). Otherwise
 * the host power will be switched by using the pad as GPIO.
 */
int board_usb_init(int index, enum usb_init_type init)
{
	debug("board_usb_init %d, type %d\n", index, init);

	imx8m_usb_power (index, true);
	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	debug("board_usb_cleanup %d, type %d\n", index, init);

	imx8m_usb_power (index, false);
	return 0;
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
/*
 * Use this slot to init some final things before the network is started. The
 * F&S configuration heavily depends on this to set up the board specific
 * environment, i.e. environment variables that can't be defined as a constant
 * value at compile time.
 */

int board_late_init(void)
{
	/* Remove 'fdtcontroladdr' env. because we are using
	 * compiled-in version. In this case it is not possible
	 * to use this env. as saved in NAND flash. (s. readme for fdt control)
	 */
	env_set("fdtcontroladdr", "");

#if 0 //###
	/* TODO: Set here because otherwise platform would be generated from
         * name.
         */
	if (fs_board_get_type() == BT_PICOCOREMX8MX)
		env_set("platform", "picocoremx8mx");
#endif

	/* Set up all board specific variables */
	fs_board_late_init_common("ttymxc");

	/* Set mac addresses for corresponding boards */
	fs_ethaddr_init();

	return 0;
}
#endif /* CONFIG_BOARD_LATE_INIT */

#ifdef CONFIG_FEC_MXC
#define FEC_RST_PAD IMX_GPIO_NR(1, 5)
#define FEC_SIM_PAD IMX_GPIO_NR(1, 26)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
	IMX8MM_PAD_GPIO1_IO05_GPIO1_IO5 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_ENET_RD0_GPIO1_IO26 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
					 ARRAY_SIZE (fec1_rst_pads));

	/* before resetting the ethernet switch for PCoreMX8MX revision 1.10 we
	 * have to configure a strapping pin to use the Serial Interface Mode
	 * "I2C". The eth node in device tree will overwrite the mux option for
	 * ENET_RD0 so we don´t have to change it back to dedicated function.
	 */
	if(fs_board_get_rev() == 110) {
		gpio_request(FEC_SIM_PAD, "SerialInterfaceMode");
		gpio_direction_output(FEC_SIM_PAD, 1);
		gpio_free(FEC_SIM_PAD);
	}
	gpio_request(FEC_RST_PAD, "fec1_rst");
	fs_board_issue_reset(11000, 1000, FEC_RST_PAD, ~0, ~0);
}

void fs_ethaddr_init(void)
{
	unsigned int features = fs_board_get_features();
	int eth_id = 0;

	if (features & FEAT_ETH_A)
		fs_eth_set_ethaddr(eth_id++);
	if (features & FEAT_ETH_B)
		fs_eth_set_ethaddr(eth_id++);
	/* All fsimx8mm boards have a WLAN module
	 * which have an integrated mac address. So we don´t
	 * have to set an own mac address for the module.
	 */
//	if (features & FEAT_WLAN)
//		fs_eth_set_ethaddr(eth_id++);
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs =
		(struct iomuxc_gpr_base_regs*) IOMUXC_GPR_BASE_ADDR;
	unsigned int board_type = fs_board_get_type();
	enum enet_freq freq;

	if(fs_board_get_type() == BT_PICOCOREMX8MX)
		setup_iomux_fec();


	switch(board_type) {
	case BT_TBS2:
		/* external 25 MHz oscilator REF_CLK => 50 MHz */
		clrbits_le32(&iomuxc_gpr_regs->gpr[1],
				IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK);
		freq = ENET_50MHZ;
		break;
	default:
		/* Use 125M anatop REF_CLK1 for ENET1, not from external */
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
				IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_SHIFT, 0);
		freq = ENET_125MHZ;
		break;
	}
	return set_clk_enet(freq);
}

#define KSZ9893R_SLAVE_ADDR		0x5F
#define KSZ9893R_CHIP_ID_MSB		0x1
#define KSZ9893R_CHIP_ID_LSB		0x2
#define KSZ9893R_CHIP_ID		0x9893
#define KSZ9893R_REG_PORT_3_CTRL_1	0x3301
#define KSZ9893R_XMII_MODES		BIT(2)
#define KSZ9893R_RGMII_ID_IG		BIT(4)
static int ksz9893r_check_id(struct udevice *ksz9893_dev)
{
	uint8_t val = 0;
	uint16_t chip_id = 0;
	int ret;

	ret = dm_i2c_read(ksz9893_dev, KSZ9893R_CHIP_ID_MSB, &val, sizeof(val));
	if (ret != 0) {
		printf("%s: Cannot access ksz9893r %d\n", __func__, ret);
		return ret;
	}
	chip_id |= val << 8;
	ret = dm_i2c_read(ksz9893_dev, KSZ9893R_CHIP_ID_LSB, &val, sizeof(val));
	if (ret != 0) {
		printf("%s: Cannot access ksz9893r %d\n", __func__, ret);
		return ret;
	}
	chip_id |= val;

	if (KSZ9893R_CHIP_ID == chip_id) {
		return 0;
	} else {
		printf("%s: Device with ID register %x is not a ksz9893r\n", __func__,
			   chip_id);
		return 1;
	}
}

static int board_setup_ksz9893r(void)
{
	struct udevice *bus = 0;
	struct udevice *ksz9893_dev = NULL;
	int ret;
	int i2c_bus = 4;
	uint8_t val = 0;

	ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
	if (ret)
	{
		printf("%s: No bus %d\n", __func__, i2c_bus);
		return -EINVAL;
	}

	ret = dm_i2c_probe(bus, KSZ9893R_SLAVE_ADDR, 0, &ksz9893_dev);
	if (ret)
	{
		printf("%s: No device id=0x%x, on bus %d, ret %d\n",
		       __func__, KSZ9893R_SLAVE_ADDR, i2c_bus, ret);
		return -ENODEV;
	}

	/* offset - 16-bit address */
	i2c_set_chip_offset_len(ksz9893_dev, 2);

	/* check id if ksz9893 is available */
	ret = ksz9893r_check_id(ksz9893_dev);
	if (ret != 0)
		return ret;

	/* Set ingress delay (on TXC) to 1.5ns and disable In-Band Status */
	ret = dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PORT_3_CTRL_1, &val,
					  sizeof(val));
	if (ret != 0) {
		printf("%s: Cannot access register %x of ksz9893r %d\n",
		       __func__, KSZ9893R_REG_PORT_3_CTRL_1, ret);
		return ret;
	}
	val |= KSZ9893R_RGMII_ID_IG;
	val &= ~KSZ9893R_XMII_MODES;
	ret = dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PORT_3_CTRL_1, &val,
					   sizeof(val));
	if (ret != 0) {
		printf("%s: Cannot access register %x of ksz9893r %d\n",
		       __func__, KSZ9893R_REG_PORT_3_CTRL_1, ret);
		return ret;
	}

	return ret;
}

int board_phy_config(struct phy_device *phydev)
{
	unsigned int board_type = fs_board_get_type();
	u16 reg = 0;

	switch(board_type) {
	case BT_TBS2:
		/* do not use KSZ8081RNA specific config funcion.
		 * This function says clock input to XI is 50 MHz, but
		 * we have an 25 MHz oscilator, so we need to set
		 * bit 7 to 0 (register 0x1f)
		 */
		reg = phy_read(phydev, 0x0, 0x1f);
		reg &= 0xff7f;
		phy_write(phydev, 0x0, 0x1f, reg);
		break;
	default:
		if (fs_board_get_type() != BT_PICOCOREMX8MX) {
			/* enable rgmii rxc skew and phy mode select to RGMII copper */
			phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
			phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

			phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
			phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);
		}

		if (phydev->drv->config)
			phydev->drv->config(phydev);
		break;
	}

	return 0;
}
#endif /* CONFIG_FEC_MXC */

#define RDC_PDAP70      0x303d0518
#define RDC_PDAP105     0x303d05A4
#define FDT_UART_C      "serial3"
#define FDT_NAND        "nand"
#define FDT_EMMC        "emmc"
#define FDT_CMA         "/reserved-memory/linux,cma"
#define FDT_RTC85063    "rtcpcf85063"
#define FDT_RTC85263    "rtcpcf85263"
#define FDT_EEPROM      "eeprom"
#define FDT_CAN         "mcp2518fd"
#define FDT_SGTL5000    "sgtl5000"
#define FDT_I2C_SWITCH  "i2c4"
#define FDT_TEMP_ALERT   "/thermal-zones/cpu-thermal/trips/trip0"
#define FDT_TEMP_CRIT    "/thermal-zones/cpu-thermal/trips/trip1"
/* Do all fixups that are done on both, U-Boot and Linux device tree */
static int do_fdt_board_setup_common(void *fdt)
{
	unsigned int features = fs_board_get_features();

	/* Disable NAND if it is not available */
	if (!(features & FEAT_NAND))
		fs_fdt_enable(fdt, FDT_NAND, 0);

	/* Disable eMMC if it is not available */
	if (!(features & FEAT_EMMC))
		fs_fdt_enable(fdt, FDT_EMMC, 0);

	return 0;
}

/* Do any board-specific modifications on U-Boot device tree before starting */
int board_fix_fdt(void *fdt)
{
	/* Make some room in the FDT */
	fdt_shrink_to_minimum(fdt, 8192);

	return do_fdt_board_setup_common(fdt);
}

/* Do any additional board-specific modifications on Linux device tree */
int ft_board_setup(void *fdt, bd_t *bd)
{
	const char *envvar;
	int offs;
	unsigned int board_type = fs_board_get_type();
	unsigned int features = fs_board_get_features();
	int minc, maxc;
	int id = 0;
	fdt32_t tmp_val[1];

	/* The following stuff is only set in Linux device tree */
	/* Disable RTC85063 if it is not available */
	if (!(features & FEAT_RTC85063))
		fs_fdt_enable(fdt, FDT_RTC85063, 0);

	/* Disable RTC85263 if it is not available */
	if (!(features & FEAT_RTC85263))
		fs_fdt_enable(fdt, FDT_RTC85263, 0);

	/* Disable EEPROM if it is not available */
	if (!(features & FEAT_EEPROM))
		fs_fdt_enable(fdt, FDT_EEPROM, 0);

	/* Disable CAN-FD if it is not available */
	if (!(features & FEAT_CAN))
		fs_fdt_enable(fdt, FDT_CAN, 0);

	/* Disable SGTL5000 if it is not available */
	if (!(features & FEAT_SGTL5000))
		fs_fdt_enable(fdt, FDT_SGTL5000, 0);

	/* Disable I2C for switch if it is not available */
	if (!(features & FEAT_ETH_A) && (board_type == BT_PICOCOREMX8MX))
		fs_fdt_enable(fdt, FDT_I2C_SWITCH, 0);

	/* Set bdinfo entries */
	offs = fs_fdt_path_offset(fdt, "/bdinfo");
	if (offs >= 0) {
		/* Set common bdinfo entries */
		fs_fdt_set_bdinfo(fdt, offs);

		/* MAC addresses */
		if (features & FEAT_ETH_A)
			fs_fdt_set_macaddr(fdt, offs, id++);
		if (features & FEAT_ETH_B)
			fs_fdt_set_macaddr(fdt, offs, id++);
		/* All fsimx8mm boards have a WLAN module
		 * which have an integrated mac address. So we don´t
		 * have to set an own mac address for the module.
		 */
//		if (features & FEAT_WLAN)
//			fs_fdt_set_macaddr(fdt, offs, id++);
	}

	/*TODO: Its workaround to use UART4 */
	envvar = env_get("m4_uart4");
	if (!envvar || !strcmp(envvar, "disable")) {
		/* Disable UART4 for M4. Enabled by ATF. */
		writel(0xff, RDC_PDAP70);
		writel(0xff, RDC_PDAP105);
	} else {
		/* Disable UART_C in DT */
		fs_fdt_enable(fdt, FDT_UART_C, 0);
	}

	/* Set linux,cma size depending on RAM size. Default is 320MB. */
	offs = fs_fdt_path_offset(fdt, FDT_CMA);
	if (fdt_get_property(fdt, offs, "no-uboot-override", NULL) == NULL) {
		unsigned int dram_size = fs_board_get_cfg_info()->dram_size;
		if ((dram_size == 1023) || (dram_size == 1024)) {
			fdt32_t tmp[2];
			tmp[0] = cpu_to_fdt32(0x0);
			tmp[1] = cpu_to_fdt32(0x28000000);
			fs_fdt_set_val(fdt, offs, "size", tmp, sizeof(tmp), 1);
		}
	}

	/* Set CPU temp grade */
	get_cpu_temp_grade(&minc, &maxc);
	/* Sanity check for get_cpu_temp_grade() */
	if ((minc > -500) && maxc < 500) {

		tmp_val[0]=cpu_to_fdt32((maxc-10)*1000);
		offs = fs_fdt_path_offset(fdt, FDT_TEMP_ALERT);
		if (fdt_get_property(fdt, offs, "no-uboot-override", NULL) == NULL) {
			fs_fdt_set_val(fdt, offs, "temperature",tmp_val , sizeof(tmp_val), 1);
		}
		tmp_val[0]=cpu_to_fdt32(maxc*1000);
		offs = fs_fdt_path_offset(fdt, FDT_TEMP_CRIT);
		if (fdt_get_property(fdt, offs, "no-uboot-override", NULL) == NULL) {
			fs_fdt_set_val(fdt, offs, "temperature", tmp_val, sizeof(tmp_val), 1);
		}
	} else {
		printf("## Wrong cpu temp grade values read! Keeping defaults from device tree\n");
	}

	return do_fdt_board_setup_common(fdt);
}

#ifdef CONFIG_FASTBOOT_STORAGE_MMC
int mmc_map_to_kernel_blk(int devno)
{
	return devno + 1;
}
#endif /* CONFIG_FASTBOOT_STORAGE_MMC */

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	/* TODO */
	return 0;
}
#endif /* CONFIG_BOARD_POSTCLK_INIT */
