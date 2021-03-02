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
#include <usb.h>
#include <serial.h>			/* get_serial_device() */
#include "../common/fs_fdt_common.h"	/* fs_fdt_set_val(), ... */
#include "../common/fs_board_common.h"	/* fs_board_*() */
#include <nand.h>


/* ------------------------------------------------------------------------- */

DECLARE_GLOBAL_DATA_PTR;

#define BT_PICOCOREMX8MM 	0
#define BT_PICOCOREMX8MX	1

/* Features set in fs_nboot_args.chFeature2 (available since NBoot VN27) */
#define FEAT2_8MM_ETH_A  	(1<<0)	/* 0: no LAN0, 1; has LAN0 */
#define FEAT2_8MM_ETH_B		(1<<1)	/* 0: no LAN1, 1; has LAN1 */
#define FEAT2_8MM_EMMC   	(1<<2)	/* 0: no eMMC, 1: has eMMC */
#define FEAT2_8MM_WLAN   	(1<<3)	/* 0: no WLAN, 1: has WLAN */
#define FEAT2_8MM_HDMICAM	(1<<4)	/* 0: LCD-RGB, 1: HDMI+CAM (PicoMOD) */
#define FEAT2_8MM_AUDIO   	(1<<5)	/* 0: Codec onboard, 1: Codec extern */
#define FEAT2_8MM_SPEED   	(1<<6)	/* 0: Full speed, 1: Limited speed */
#define FEAT2_8MM_LVDS    	(1<<7)	/* 0: MIPI DSI, 1: LVDS */
#define FEAT2_8MM_ETH_MASK 	(FEAT2_8MM_ETH_A | FEAT2_8MM_ETH_B)

#define FEAT2_8MX_DDR3L_X2 	(1<<0)	/* 0: DDR3L x1, 1; DDR3L x2 */
#define FEAT2_8MX_NAND_EMMC	(1<<1)	/* 0: NAND, 1: has eMMC */
#define FEAT2_8MX_CAN		(1<<2)	/* 0: no CAN, 1: has CAN */
#define FEAT2_8MX_SEC_CHIP	(1<<3)	/* 0: no Security Chip, 1: has Security Chip */
#define FEAT2_8MX_AUDIO 	(1<<4)	/* 0: no Audio, 1: Audio */
#define FEAT2_8MX_EXT_RTC   	(1<<5)	/* 0: internal RTC, 1: external RTC */
#define FEAT2_8MX_LVDS   	(1<<6)	/* 0: MIPI DSI, 1: LVDS */
#define FEAT2_8MX_ETH   	(1<<7)	/* 0: no LAN, 1; has LAN */

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

#ifdef CONFIG_ENV_IS_IN_MMC
#define ROOTFS ".rootfs_mmc"
#define KERNEL ".kernel_mmc"
#define FDT ".fdt_mmc"
#define SET_ROOTFS ".set_rootfs_mmc"
#define SELECTOR ".selector_mmc"
#define BOOT_PARTITION ".boot_partition_mmc"
#define ROOTFS_PARTITION ".rootfs_partition_mmc"
#elif CONFIG_ENV_IS_IN_NAND
#define ROOTFS ".rootfs_ubifs"
#define KERNEL ".kernel_nand"
#define FDT ".fdt_nand"
#define SET_ROOTFS ".set_rootfs_nand"
#define SELECTOR ".selector_nand"
#define BOOT_PARTITION ".boot_partition_nand"
#define ROOTFS_PARTITION ".rootfs_partition_nand"
#else /* Default = Nand */
#define ROOTFS ".rootfs_ubifs"
#define KERNEL ".kernel_nand"
#define FDT ".fdt_nand"
#define SET_ROOTFS ".set_rootfs_nand"
#define SELECTOR ".selector_nand"
#define BOOT_PARTITION ".boot_partition_nand"
#define ROOTFS_PARTITION ".rootfs_partition_nand"
#endif

const struct fs_board_info board_info[2] = {
	{	/* 0 (BT_PICOCOREMX8MM) */
		.name = "PicoCoreMX8MM",
		.bootdelay = "3",
		.updatecheck = UPDATE_DEF,
		.installcheck = INSTALL_DEF,
		.recovercheck = UPDATE_DEF,
		.console = ".console_serial",
		.login = ".login_serial",
		.mtdparts = ".mtdparts_std",
		.network = ".network_off",
		.init = ".init_init",
		.rootfs = ROOTFS,
		.kernel = KERNEL,
		.fdt = FDT,
#ifdef CONFIG_FS_UPDATE_SUPPORT
		.set_rootfs = SET_ROOTFS,
		.selector = SELECTOR,
		.boot_partition = BOOT_PARTITION,
		.rootfs_partition = ROOTFS_PARTITION
#endif
	},
	{	/* 1 (BT_PICOCOREMX8MX) */
		.name = "PicoCoreMX8MM",
		.bootdelay = "3",
		.updatecheck = UPDATE_DEF,
		.installcheck = INSTALL_DEF,
		.recovercheck = UPDATE_DEF,
		.console = ".console_serial",
		.login = ".login_serial",
		.mtdparts = ".mtdparts_std",
		.network = ".network_off",
		.init = ".init_init",
		.rootfs = ROOTFS,
		.kernel = KERNEL,
		.fdt = FDT,
#ifdef CONFIG_FS_UPDATE_SUPPORT
		.set_rootfs = SET_ROOTFS,
		.selector = SELECTOR,
		.boot_partition = BOOT_PARTITION,
		.rootfs_partition = ROOTFS_PARTITION
#endif
	},
};

/* ---- Stage 'f': RAM not valid, variables can *not* be used yet ---------- */

#ifdef CONFIG_NAND_MXS
static void setup_gpmi_nand(void);
#endif

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

/* Do some very early board specific setup */
int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs*) WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand(); /* SPL will call the board_early_init_f */
#endif

	return 0;
}

/* Check board type */
int checkboard(void)
{
	struct fs_nboot_args *pargs = fs_board_get_nboot_args();
	unsigned int board_type = fs_board_get_type();
	unsigned int board_rev = fs_board_get_rev();
	unsigned int features2;

	features2 = pargs->chFeatures2;

	printf ("Board: %s Rev %u.%02u (", board_info[board_type].name,
		board_rev / 100, board_rev % 100);
	switch (board_type) 
	{
	case BT_PICOCOREMX8MM:
		if ((features2 & FEAT2_8MM_ETH_MASK) == FEAT2_8MM_ETH_MASK)
			puts ("2x ");
		if (features2 & FEAT2_8MM_ETH_MASK)
			puts ("LAN, ");
		if (features2 & FEAT2_8MM_WLAN)
			puts ("WLAN, ");
		if (features2 & FEAT2_8MM_EMMC)
			puts ("eMMC, ");
		else
			puts("NAND, ");
		break;
	case BT_PICOCOREMX8MX:
		if (features2 & FEAT2_8MX_ETH)
		puts ("LAN, ");
		if (features2 & FEAT2_8MX_NAND_EMMC)
			puts ("eMMC, ");
		else
			puts("NAND, ");
		break;
	}

	printf ("%dx DRAM)\n", pargs->dwNumDram);

	//fs_board_show_nboot_args(pargs);

	return 0;
}

/* ---- Stage 'r': RAM valid, U-Boot relocated, variables can be used ------ */
static int setup_fec(void);

int board_init(void)
{
	unsigned int board_type = fs_board_get_type();

	/* Copy NBoot args to variables and prepare command prompt string */
	fs_board_init_common(&board_info[board_type]);

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

	return 0;
}

/* nand flash pads  */
#ifdef CONFIG_NAND_MXS
#define NAND_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL2 | PAD_CTL_HYS)
#define NAND_PAD_READY0_CTRL (PAD_CTL_DSE6 | PAD_CTL_FSEL2 | PAD_CTL_PUE)
static iomux_v3_cfg_t const gpmi_pads[] = {
	IMX8MM_PAD_NAND_ALE_RAWNAND_ALE | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_CE0_B_RAWNAND_CE0_B | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_CLE_RAWNAND_CLE | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA00_RAWNAND_DATA00 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA01_RAWNAND_DATA01 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA02_RAWNAND_DATA02 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA03_RAWNAND_DATA03 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA04_RAWNAND_DATA04 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA05_RAWNAND_DATA05	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA06_RAWNAND_DATA06	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA07_RAWNAND_DATA07	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_RE_B_RAWNAND_RE_B | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_READY_B_RAWNAND_READY_B | MUX_PAD_CTRL(NAND_PAD_READY0_CTRL),
	IMX8MM_PAD_NAND_WE_B_RAWNAND_WE_B | MUX_PAD_CTRL(NAND_PAD_CTRL),
	IMX8MM_PAD_NAND_WP_B_RAWNAND_WP_B | MUX_PAD_CTRL(NAND_PAD_CTRL),
};

static void setup_gpmi_nand(void)
{
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));
}
#endif /* CONFIG_NAND_MXS */


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
	/* TODO: Set here because otherwise platform would be generated from
         * name.
         */
	if (fs_board_get_type() == BT_PICOCOREMX8MX)
		env_set("platform", "picocoremx8mx");
	/* Set up all board specific variables */
	fs_board_late_init_common("ttymxc");
#ifdef CONFIG_VIDEO
//	tc358764_init();
#endif
	return 0;
}
#endif /* CONFIG_BOARD_LATE_INIT */

#ifdef CONFIG_FEC_MXC
/* enet pads definition */
static iomux_v3_cfg_t const enet_8mm_pads_rgmii[] = {
	IMX8MM_PAD_ENET_MDIO_ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_MDC_ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TXC_ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TX_CTL_ENET1_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD0_ENET1_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD1_ENET1_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD2_ENET1_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD3_ENET1_RGMII_TD3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RXC_ENET1_RGMII_RXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RX_CTL_ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD0_ENET1_RGMII_RD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD1_ENET1_RGMII_RD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD2_ENET1_RGMII_RD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD3_ENET1_RGMII_RD3 | MUX_PAD_CTRL(ENET_PAD_CTRL),

	/* Phy Interrupt */
	IMX8MM_PAD_GPIO1_IO04_GPIO1_IO4 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const enet_8mx_pads_rgmii[] = {
	IMX8MM_PAD_ENET_MDIO_ENET1_MDIO | MUX_PAD_CTRL(PAD_CTL_DSE6),
	IMX8MM_PAD_ENET_MDC_ENET1_MDC | MUX_PAD_CTRL(PAD_CTL_DSE6 | PAD_CTL_ODE),
	IMX8MM_PAD_ENET_TXC_ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TX_CTL_ENET1_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD0_ENET1_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD1_ENET1_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD2_ENET1_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_TD3_ENET1_RGMII_TD3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RXC_ENET1_RGMII_RXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RX_CTL_ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD0_ENET1_RGMII_RD0 | MUX_PAD_CTRL(ENET_PAD_CTRL | PAD_CTL_PE ),
	IMX8MM_PAD_ENET_RD1_ENET1_RGMII_RD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD2_ENET1_RGMII_RD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MM_PAD_ENET_RD3_ENET1_RGMII_RD3 | MUX_PAD_CTRL(ENET_PAD_CTRL),

	/* Phy Interrupt */
	IMX8MM_PAD_GPIO1_IO11_GPIO1_IO11 | MUX_PAD_CTRL(PAD_CTL_PUE | PAD_CTL_DSE2 | PAD_CTL_ODE),
};

#define FEC_RST_PAD IMX_GPIO_NR(1, 5)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
	IMX8MM_PAD_GPIO1_IO05_GPIO1_IO5 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	switch (fs_board_get_type()) 
	{
	case BT_PICOCOREMX8MM:
		imx_iomux_v3_setup_multiple_pads (enet_8mm_pads_rgmii,
					 	 ARRAY_SIZE (enet_8mm_pads_rgmii));
		break;
	case BT_PICOCOREMX8MX:
		imx_iomux_v3_setup_multiple_pads (enet_8mx_pads_rgmii,
						  ARRAY_SIZE (enet_8mx_pads_rgmii));	
		break;
	}



	imx_iomux_v3_setup_multiple_pads (fec1_rst_pads, ARRAY_SIZE (fec1_rst_pads));

	gpio_request (FEC_RST_PAD, "fec1_rst");
	gpio_direction_output (FEC_RST_PAD, 0);
	udelay (10000);
	gpio_direction_output (FEC_RST_PAD, 1);
	udelay (1000);
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs =
		(struct iomuxc_gpr_base_regs*) IOMUXC_GPR_BASE_ADDR;

	setup_iomux_fec();

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32 (&iomuxc_gpr_regs->gpr[1],
			 IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_SHIFT,
			 0);
	return set_clk_enet (ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write (phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write (phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	if (fs_board_get_type() == BT_PICOCOREMX8MX) {
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
	}
	phy_write (phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write (phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config (phydev);

	return 0;
}
#endif /* CONFIG_FEC_MXC */

#ifdef CONFIG_OF_BOARD_SETUP

#define RDC_PDAP70      0x303d0518
#define FDT_UART_C	"serial3"
#define FDT_NAND        "nand"
#define FDT_EMMC        "mmc2"
#define FDT_CMA 	"/reserved-memory/linux,cma"

/* Do any additional board-specific device tree modifications */
int ft_board_setup(void *fdt, bd_t *bd)
{
	int offs;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args ();
	const char *envvar;

	/* Set bdinfo entries */
	offs = fs_fdt_path_offset (fdt, "/bdinfo");
	switch (fs_board_get_type()) 
	{
	case BT_PICOCOREMX8MM:
		if (offs >= 0)
		{
			int id = 0;
			/* Set common bdinfo entries */
			fs_fdt_set_bdinfo (fdt, offs);

			/* MAC addresses */
			if (pargs->chFeatures2 & FEAT2_8MM_ETH_A)
				fs_fdt_set_macaddr (fdt, offs, id++);

			if (pargs->chFeatures2 & FEAT2_8MM_WLAN)
				fs_fdt_set_macaddr (fdt, offs, id++);
		}

		if(pargs->chFeatures2 & FEAT2_8MM_EMMC)
		{
			/* enable emmc node  */
			fs_fdt_enable(fdt, FDT_EMMC, 1);

			/* disable nand node  */
			fs_fdt_enable(fdt, FDT_NAND, 0);
		
		}
		break;
	case BT_PICOCOREMX8MX:
		if (offs >= 0)
		{
			int id = 0;
			/* Set common bdinfo entries */
			fs_fdt_set_bdinfo (fdt, offs);

			/* MAC addresses */
			if (pargs->chFeatures2 & FEAT2_8MX_ETH)
				fs_fdt_set_macaddr (fdt, offs, id++);
		}


		if(pargs->chFeatures2 & FEAT2_8MX_NAND_EMMC)
		{
			/* enable emmc node  */
			fs_fdt_enable(fdt, FDT_EMMC, 1);

			/* disable nand node  */
			fs_fdt_enable(fdt, FDT_NAND, 0);

		}	
		break;
	}
	

	/*TODO: Its workaround to use UART4 */
	envvar = env_get("m4_uart4");

	if (!envvar || !strcmp(envvar, "disable")) {
		/* Disable UART4 for M4. Enabled by ATF. */
		writel(0xff, RDC_PDAP70);
	}else{
		/* Disable UART_C in DT */
		fs_fdt_enable(fdt, FDT_UART_C, 0);
	}

	/* Set linux,cma size depending on RAM size. Default is 320MB. */
	offs = fs_fdt_path_offset(fdt, FDT_CMA);
	if (fdt_get_property(fdt, offs, "no-uboot-override", NULL) == NULL) {
		if (pargs->dwMemSize==1023 || pargs->dwMemSize==1024){
			fdt32_t tmp[2];
			tmp[0] = cpu_to_fdt32(0x0);
			tmp[1] = cpu_to_fdt32(0x28000000);
			fs_fdt_set_val(fdt, offs, "size", tmp, sizeof(tmp), 1);
		}
	}

	return 0;
}
#endif /* CONFIG_OF_BOARD_SETUP */

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
