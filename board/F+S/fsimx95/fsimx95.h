// SPDX-License-Identifier: GPL-2.0+
/**
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

#ifndef __BOARD_FSIMX95_H
#define __BOARD_FSIMX95_H

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
#define FEAT_BT		BIT(9)
#define FEAT_SDIO_A	BIT(10)
#define FEAT_SDIO_B	BIT(11)
#define FEAT_SDIO_C	BIT(12)
#define FEAT_MIPI_DSI	BIT(13)
#define FEAT_MIPI_CSI	BIT(14)
#define FEAT_LVDS	BIT(15)
#define FEAT_USB_HUB	BIT(16)
#define FEAT_TEMP	BIT(17)
#define FEAT_SEC	BIT(18)
#define FEAT_GPIO_EXP	BIT(19)
#define FEAT_EDP	BIT(20)

/* SCMI Protocols */
#define SCMI_BRD_FLAG 0x8000U
/* IO_CTRLs */
#define SCMI_FUS_MISC_IO_T_SENSE_EV       (0U)
#define SCMI_FUS_MISC_IO_BATLOW           (1U)
#define SCMI_FUS_MISC_IO_CHARGER_PRSNT    (2U)
#define SCMI_FUS_MISC_IO_CHARGING         (3U)
#define SCMI_FUS_MISC_IO_SMB_ALERT        (4U)
#define SCMI_FUS_MISC_IO_TEST             (5U)
#define SCMI_FUS_MISC_IO_SLEEP            (6U)
#define SCMI_FUS_MISC_IO_LID              (7U)
#define SCMI_FUS_MISC_IO_SEC_EN_IRQ       (8U)
#define SCMI_FUS_MISC_IO_PCIe_A_WAKE      (9U)
#define SCMI_FUS_MISC_IO_WLAN_WAKE_HOST   (10U)
#define SCMI_FUS_MISC_IO_BT_WAKE_HOST     (11U)
#define SCMI_FUS_MISC_IO_SN65DSI86_INT    (12U)
#define SCMI_FUS_MISC_IO_RSTOUT           (13U)
#define SCMI_FUS_MISC_IO_PCIE_PERST       (14U)
#define SCMI_FUS_MISC_IO_ETH_A_PHY_RST    (15U)
#define SCMI_FUS_MISC_IO_ETH_B_PHY_RST    (16U)

/* MISC_CTRLs */
#define SCMI_FUS_MISC_DDR_INIT        (17U)
#define SCMI_FUS_MISC_RTC             (18U)
#define SCMI_FUS_MISC_TEST            (19U)
#define SCMI_FUS_MISC_TEST_A          (20U)

enum fsimx95_board_types {
	BT_FSSM95S,
	BT_PICOCOREMX95
};

#endif /* __BOARD_FSIMX95_H */
