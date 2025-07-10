// SPDX-License-Identifier: GPL-2.0+
/**
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

#ifndef __BOARD_FSIMX93_H
#define __BOARD_FSIMX93_H

#define SET_BOARD_TYPE(ID, TYPE, BOARD_ID, LEN)	\
	if (!strncmp(BOARD_ID, ID, LEN))	\
	{					\
		gd->board_type = TYPE;		\
		return 0;			\
	}

#define CHECK_BOARD_TYPE_AND_NAME(BOARD_STRING, BOARD_TYPE, NAME) \
	if (gd->board_type == BOARD_TYPE)		\
	{						\
		if (!strcmp(NAME, BOARD_STRING))	\
			return 0;			\
	}

/* TODO: IN PREPARATION */
#define FEAT_EMMC 	BIT(0)
#define FEAT_EXT_RTC 	BIT(1)
#define FEAT_EEPROM	BIT(2)
#define FEAT_ETH_A	BIT(3)
#define FEAT_ETH_B	BIT(4)
#define FEAT_ETH_PHY_A	BIT(5)
#define FEAT_ETH_PHY_B	BIT(6)
#define FEAT_AUDIO	BIT(7)
#define FEAT_WLAN	BIT(8)
#define FEAT_SDIO_A	BIT(9)
#define FEAT_SDIO_B	BIT(10)
#define FEAT_SDIO_C	BIT(11)
#define FEAT_MIPI_DSI	BIT(12)
#define FEAT_MIPI_CSI	BIT(13)
#define FEAT_LVDS	BIT(14)
#define FEAT_RGB	BIT(15)


#define UART_PAD_CTRL (PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define WDOG_PAD_CTRL (PAD_CTL_DSE(6) | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

#if defined(CONFIG_TARGET_FSIMX93)
/* PCoreMX93 rev100 */
static __maybe_unused iomux_v3_cfg_t const lpuart2_pads[] = {
    MX93_PAD_UART2_RXD__LPUART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX93_PAD_UART2_TXD__LPUART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

/* OSMSFMX93 rev100 */
/* EFUSMX93 rev100 */
static __maybe_unused iomux_v3_cfg_t const lpuart1_pads[] = {
    MX93_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX93_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL)};

/* PCoreMX93 rev110 */
static __maybe_unused iomux_v3_cfg_t const lpuart7_pads[] = {
    MX93_PAD_GPIO_IO09__LPUART7_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX93_PAD_GPIO_IO08__LPUART7_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

/**
 *  TODO: WDOG PAD
 */
#elif defined(CONFIG_TARGET_FSIMX91)
/* PCoreMX91 rev100 */
static __maybe_unused iomux_v3_cfg_t const lpuart2_pads[] = {
	MX91_PAD_UART2_RXD__LPUART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX91_PAD_UART2_TXD__LPUART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
    };
    
    /* OSMSFMX91 rev100 */
    /* EFUSMX91 rev100 */
    static __maybe_unused iomux_v3_cfg_t const lpuart1_pads[] = {
	MX91_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX91_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL)};
    
    /* PCoreMX91 rev110 */
    static __maybe_unused iomux_v3_cfg_t const lpuart7_pads[] = {
	MX91_PAD_GPIO_IO09__LPUART7_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX91_PAD_GPIO_IO08__LPUART7_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
    };
    
    /**
     *  TODO: WDOG PAD
     */
#endif

enum fsimx93_board_types {
	BT_PICOCOREMX93,
	BT_OSMSFMX93,
	BT_EFUSMX93,
	BT_PICOCOREMX91,
	BT_OSMSFMX91,
	BT_EFUSMX91
};

#endif /* __BOARD_FSIMX93_H */
