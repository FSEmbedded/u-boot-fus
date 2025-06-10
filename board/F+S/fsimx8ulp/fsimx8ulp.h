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

#ifndef __BOARD_FSIMX8ULP_H
#define __BOARD_FSIMX8ULP_H

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

enum fsimx8ulp_board_types {
	BT_PICOCOREMX8ULP,
	BT_OSMSFMX8ULP,
	BT_SOLDERCOREMX8ULP,
	BT_ARMSTONEMX8ULP,
};

#define FEAT_EMMC	BIT(0)
#define FEAT_EXT_RTC	BIT(1)
#define FEAT_EEPROM	BIT(2)
#define FEAT_ETH	BIT(3)
#define FEAT_ETH_PHY	BIT(4)
#define FEAT_AUDIO_APD	BIT(5)
#define FEAT_AUDIO_RTD	BIT(6)
#define FEAT_WLAN	BIT(7)
#define FEAT_SDIO_A	BIT(8)
#define FEAT_SDIO_B	BIT(9)
#define FEAT_MIPI_DSI	BIT(10)
#define FEAT_MIPI_CSI	BIT(11)
#define FEAT_RGB	BIT(12)

#endif /* __BOARD_FSIMX8ULP_H */