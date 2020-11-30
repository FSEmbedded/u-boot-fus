/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <serial.h>
#include "pca953x.h"

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <asm/mach-imx/dma.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>
#include <cdns3-uboot.h>
#include <asm/arch/lpcg.h>

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPMI_NAND_PAD_CTRL	 ((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) \
				  | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))


#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define FSPI_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define I2C_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#ifdef CONFIG_NAND_MXS
static iomux_cfg_t gpmi_nand_pads[] = {
	SC_P_EMMC0_CLK | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_CMD | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),

};

static void setup_iomux_gpmi_nand(void)
{
	imx8_iomux_setup_multiple_pads(gpmi_nand_pads, ARRAY_SIZE(gpmi_nand_pads));
}

static void imx8qxp_gpmi_nand_initialize(void)
{
	int ret;
#ifdef CONFIG_SPL_BUILD
	sc_ipc_t ipcHndl = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_DMA_4_CH0, SC_PM_PW_MODE_ON);
                        if (ret != SC_ERR_NONE)
                                return;


    ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_NAND, SC_PM_PW_MODE_ON);
                        if (ret != SC_ERR_NONE)
                                return;
#else
	struct power_domain pd;

	if (!power_domain_lookup_name("conn_dma4_ch0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("conn_dma4_ch0 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("conn_nand", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("conn_nand Power up failed! (error = %d)\n", ret);
	}
#endif

	init_clk_gpmi_nand();
	setup_iomux_gpmi_nand();
	mxs_dma_init();

}
#endif

static iomux_cfg_t uart2_pads[] = {
	SC_P_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart2_pads, ARRAY_SIZE(uart2_pads));
}

int board_early_init_f(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* This works around that having only UART2 up the baudrate is 1.2M
	 * instead of 115.2k. Set UART0 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_0, SC_PM_CLK_PER, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Power up UART2 */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_2, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set UART2 clock root to 80 MHz */

	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_2, 2, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable UART2 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_2, 2, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	LPCG_AllClockOn(LPUART_2_LPCG);

	setup_iomux_uart();

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_NAND_MXS
	imx8qxp_gpmi_nand_initialize();
#endif
#endif

	return 0;
}

#ifdef CONFIG_FSL_ESDHC

#define USDHC1_CD_GPIO	IMX_GPIO_NR(4, 22)

#ifndef CONFIG_SPL_BUILD
static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC1_BASE_ADDR, 0, 8},
#ifndef CONFIG_TARGET_IMX8X_17X17_VAL
	{USDHC2_BASE_ADDR, 0, 4},
#endif
};

static iomux_cfg_t emmc0[] = {
	SC_P_EMMC0_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_EMMC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t usdhc1_sd[] = {
	SC_P_USDHC1_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC1_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_WP    | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for WP */
	SC_P_USDHC1_CD_B  | MUX_MODE_ALT(4) | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for CD,  GPIO4 IO22 */
	SC_P_USDHC1_VSELECT | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	struct power_domain pd;

#ifdef CONFIG_NAND_MXS
	return 0;
#endif

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			if (!power_domain_lookup_name("conn_sdhc0", &pd))
				power_domain_on(&pd);
			imx8_iomux_setup_multiple_pads(emmc0, ARRAY_SIZE(emmc0));
			init_clk_usdhc(0);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			if (!power_domain_lookup_name("conn_sdhc1", &pd))
				power_domain_on(&pd);
			imx8_iomux_setup_multiple_pads(usdhc1_sd, ARRAY_SIZE(usdhc1_sd));
			init_clk_usdhc(1);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			gpio_request(USDHC1_CD_GPIO, "sd1_cd");
			gpio_direction_input(USDHC1_CD_GPIO);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}
		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret) {
			printf("Warning: failed to initialize mmc dev %d\n", i);
			return ret;
		}
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1; /* eMMC */
		break;
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	}

	return ret;
}

#endif /* CONFIG_FSL_ESDHC */
#endif /* CONFIG_SPL_BUILD */

#ifdef CONFIG_FEC_MXC
#include <miiphy.h>

static iomux_cfg_t pad_enet1[] = {
	SC_P_SPDIF0_TX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_SPDIF0_RX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX3_RX2 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX2_RX3 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_SCKR | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_TX4_RX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_TX5_RX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_FST  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_SCKT | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_FSR  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	/* Shared MDIO */
	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
};

static iomux_cfg_t pad_enet0[] = {
	SC_P_ENET0_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD0 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD1 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD2 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD3 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXC | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD0 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD1 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD2 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD3 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	/* Shared MDIO */
	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	if (0 == CONFIG_FEC_ENET_DEV)
		imx8_iomux_setup_multiple_pads(pad_enet0, ARRAY_SIZE(pad_enet0));
	else
		imx8_iomux_setup_multiple_pads(pad_enet1, ARRAY_SIZE(pad_enet1));
}

#define PHY1_RST IMX_GPIO_NR(5, 9)
#define PHY2_RST IMX_GPIO_NR(0, 29)

static void enet_device_phy_reset(void)
{
	/* Reserve GPIOs for reset of PHYs */
	gpio_request(PHY1_RST, "phy1_rst");
	gpio_request(PHY2_RST, "phy2_rst");
	/* Start the reset (active low) */
	gpio_direction_output(PHY1_RST, 0);
	gpio_direction_output(PHY2_RST, 0);
	udelay(50);
	/* End the reset */
	gpio_direction_output(PHY1_RST, 1);
	gpio_direction_output(PHY2_RST, 1);

	/* The board has a long delay for this reset to become stable */
	mdelay(200);
}

int board_eth_init(bd_t *bis)
{
	int ret;
	struct power_domain pd;

	printf("[%s] %d\n", __func__, __LINE__);

	if (CONFIG_FEC_ENET_DEV) {
		if (!power_domain_lookup_name("conn_enet1", &pd))
			power_domain_on(&pd);
	} else {
		if (!power_domain_lookup_name("conn_enet0", &pd))
			power_domain_on(&pd);
	}

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		0x4, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return ret;
}

int board_phy_config(struct phy_device *phydev)
{
	struct udevice *dev;
	struct ofnode_phandle_args phandle_args;
	char * ethprime;
	int phy_addr = 0;
	int ret = 1;
#if 1

	ethprime = env_get("ethprime");
	if (ethprime != NULL)
	{
		if (!strcmp(ethprime, "eth0"))
			ret = uclass_get_device_by_seq(UCLASS_ETH,0,&dev);
		if (!strcmp(ethprime, "eth1"))
			ret = uclass_get_device_by_seq(UCLASS_ETH,1,&dev);
		if (!ret)
		{
			if(!dev_read_phandle_with_args(dev, "phy-handle", NULL, 0, 0, &phandle_args))
				phy_addr = ofnode_read_u32_default(phandle_args.node, "reg", 0);
		}
	}

	/* Only enable the phy from FEC defined by ethprime */

	if (phydev->addr == phy_addr)
	{
#endif
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

		phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

		if (phydev->drv->config)
			phydev->drv->config(phydev);
#if 1
	}
#endif
	return 0;
}



static int setup_fec(int ind)
{
	/* Reset ENET PHY */
	enet_device_phy_reset();

	return 0;
}
#endif

#ifdef CONFIG_MXC_GPIO

//#define DUAL

#ifdef DUAL
	#define SPI_NOR_WP IMX_GPIO_NR(3, 11)
	#define SPI_NOR_HOLD IMX_GPIO_NR(3, 12)
#endif

static iomux_cfg_t board_gpios[] = {
	SC_P_ENET0_REFCLK_125M_25M |  MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_SAI1_RXD |  MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
#ifdef DUAL
	SC_P_QSPI0A_DATA2 |  MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_QSPI0A_DATA3 |  MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
#endif
};

static void board_gpio_init(void)
{
	imx8_iomux_setup_multiple_pads(board_gpios, ARRAY_SIZE(board_gpios));

#ifdef DUAL
	gpio_request(SPI_NOR_WP, "spi_nor_wp");
	gpio_direction_output(SPI_NOR_WP, 1);

	gpio_request(SPI_NOR_HOLD, "spi_nor_hold");
	gpio_direction_output(SPI_NOR_HOLD, 1);
#endif
}
#endif

int checkboard(void)
{
#if defined(CONFIG_TARGET_FSIMX8X)
	puts("Board: efusMX8X\n");
#else
	puts("Board: NOT FOUND\n");
#endif

	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH)
		printf("\nSCI error! Invalid handle\n");

	return 0;
}

#ifdef CONFIG_FSL_HSIO

#define WLAN_RST	IMX_GPIO_NR(0, 19)
#define PCIE_PERST	IMX_GPIO_NR(4,  0)
#define PCIE_CLKREQ	IMX_GPIO_NR(4,  1)
#define PCIE_WAKE	IMX_GPIO_NR(4,  2)

#define PCIE_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT))
#define PCIE_WAKE_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT))
#if 0
	SC_P_PCIE_CTRL0_CLKREQ_B | MUX_MODE_ALT(4) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_WAKE_B | MUX_MODE_ALT(4) | MUX_PAD_CTRL(PCIE_WAKE_PAD_CTRL),
	SC_P_PCIE_CTRL0_PERST_B | MUX_MODE_ALT(4) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
#endif
static iomux_cfg_t board_pcie_pins[] = {
	SC_P_MCLK_IN0 | MUX_MODE_ALT(4) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
};

static void imx8qxp_hsio_initialize(void)
{
	struct power_domain pd;
	int ret;

#if 1
	if (!power_domain_lookup_name("hsio_pcie1", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie1 Power up failed! (error = %d)\n", ret);
	}
        if (!power_domain_lookup_name("hsio_gpio", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_gpio Power up failed! (error = %d)\n", ret);
	}

	LPCG_AllClockOn(HSIO_PCIE_X1_LPCG);
	LPCG_AllClockOn(HSIO_PHY_X1_LPCG);
	LPCG_AllClockOn(HSIO_PHY_X1_CRR1_LPCG);
	LPCG_AllClockOn(HSIO_PCIE_X1_CRR3_LPCG);
	LPCG_AllClockOn(HSIO_MISC_LPCG);
	LPCG_AllClockOn(HSIO_GPIO_LPCG);
#endif

#if 1
	imx8_iomux_setup_multiple_pads(board_pcie_pins, ARRAY_SIZE(board_pcie_pins));
#endif

#if 1
	/* Reserve GPIO for WLAN-Reset / Power-Down */
	gpio_request(WLAN_RST, "wlan_rst");
	/* Start the reset (active low) */
	gpio_direction_output(WLAN_RST, 0);

	udelay(50);

	/* End the reset */
	gpio_direction_output(WLAN_RST, 1);

	mdelay(200);
#endif

#if 0

	// signal to send the pcie clock for device
	gpio_request(PCIE_CLKREQ, "pcie_clkreq");
	gpio_direction_output(PCIE_CLKREQ, 0);

	gpio_request(PCIE_WAKE, "pcie_wake");
	gpio_direction_output(PCIE_WAKE, 1);

	gpio_request(PCIE_PERST, "pcie_perst");
	gpio_direction_output(PCIE_PERST, 0);

	gpio_direction_output(PCIE_PERST, 1);

	/* Reserve GPIO for WLAN-Reset / Power-Down */
	gpio_request(WLAN_RST, "wlan_rst");
	/* Start the reset (active low) */
	gpio_direction_output(WLAN_RST, 0);

	udelay(50);

	/* End the reset */
	gpio_direction_output(WLAN_RST, 1);

	mdelay(100);


	/* The board has a long delay for this reset to become stable */
	mdelay(200);

#endif
}

void pci_init_board(void)
{
	imx8qxp_hsio_initialize();
#if 0
	/* test the 1 lane mode of the PCIe A controller */
	mx8qxp_pcie_init();
#endif
}

#endif

#if defined(CONFIG_USB_CDNS3_GADGET)

static struct cdns3_device cdns3_device_data = {
	.none_core_base = 0x5B110000,
	.xhci_base = 0x5B130000,
	.dev_base = 0x5B140000,
	.phy_base = 0x5B160000,
	.otg_base = 0x5B120000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 1,
};

int usb_gadget_handle_interrupts(void)
{
	cdns3_uboot_handle_interrupt(1);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_DEVICE) {
			struct power_domain pd;
			int ret;

			/* Power on usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2 Power up failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
			}

			ret = cdns3_uboot_init(&cdns3_device_data);
			printf("%d cdns3_uboot_initmode %d\n", index, ret);
		}
	}
	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_DEVICE) {
			struct power_domain pd;
			int ret;

			cdns3_uboot_exit(1);

			/* Power off usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2 Power up failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
			}
		}
	}
	return ret;
}
#else

	int board_usb_init(int index, enum usb_init_type init)
	{
		int ret = 0;

		debug("board_usb_init %d, type %d\n", index, init);

		if (index == 0) {
			if (init == USB_INIT_DEVICE) {
				struct power_domain pd;
				int ret;

				/* Power on usb */
				if (!power_domain_lookup_name("conn_usb0", &pd)) {
					ret = power_domain_on(&pd);
					if (ret)
						printf("conn_usb0 Power up failed! (error = %d)\n", ret);
				}

				if (!power_domain_lookup_name("conn_usb0_phy", &pd)) {
					ret = power_domain_on(&pd);
					if (ret)
						printf("conn_usb0_phy Power up failed! (error = %d)\n", ret);
				}
			}
		}

		return ret;
	}

	int board_usb_cleanup(int index, enum usb_init_type init)
	{
		int ret = 0;

		debug("board_usb_cleanup %d, type %d\n", index, init);

		if (index == 0) {
			if (init == USB_INIT_DEVICE) {
				struct power_domain pd;
				int ret;

				/* Power off usb */
				if (!power_domain_lookup_name("conn_usb0", &pd)) {
					ret = power_domain_off(&pd);
					if (ret)
						printf("conn_usb0 Power up failed! (error = %d)\n", ret);
				}

				if (!power_domain_lookup_name("conn_usb0_phy", &pd)) {
					ret = power_domain_off(&pd);
					if (ret)
						printf("conn_usb0_phy Power up failed! (error = %d)\n", ret);
				}
			}
		}

		return ret;
	}

#endif

int board_init(void)
{
#ifdef CONFIG_MXC_GPIO
	board_gpio_init();
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_NAND_MXS
	imx8qxp_gpmi_nand_initialize();
#endif

	return 0;
}

/* Get the number of the debug port reported by NBoot */
struct serial_device *default_serial_console(void)
{
	return get_serial_device(CONFIG_SYS_UART_PORT);
}

void board_quiesce_devices()
{
	const char *power_on_devices[] = {
		"dma_lpuart2",

		/* HIFI DSP boot */
		"audio_sai0",
		"audio_ocram",
	};

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	puts("SCI reboot request");
	sc_pm_reboot(SC_IPC_CH, SC_PM_RESET_TYPE_COLD);
	while (1)
		putc('.');
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

void board_late_mmc_env_init(void)
{
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "efus");
	env_set("board_rev", "iMX8QXP");
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
       return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#if defined(CONFIG_VIDEO_IMXDPUV1)
static void enable_lvds(struct display_info_t const *dev)
{
	display_controller_setup((PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_soc_setup(dev->bus, (PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_configure(dev->bus);
	//lvds2hdmi_setup(13);
}

struct display_info_t const displays[] = {{
	.bus	= 0, /* LVDS0 */
	.addr	= 0, /* LVDS0 */
	.pixfmt	= IMXDPUV1_PIX_FMT_BGR32,
	.detect	= NULL,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "J070WVTC0211",
		.refresh        = 60,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 29850, // 10^12/freq
		.left_margin    = 44,
		.right_margin   = 4,
		.upper_margin   = 21,
		.lower_margin   = 11,
		.hsync_len      = 2,
		.vsync_len      = 2,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);

#endif /* CONFIG_VIDEO_IMXDPUV1 */
