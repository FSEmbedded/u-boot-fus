/*
 * check_config.c
 *
 *  Created on: Mar 08, 2021
 *      Author: developer
 */
#include <common.h>
#include "check_config.h"
#include "selftest.h"
#include "../../board/F+S/common/fs_board_common.h"/* fs_board_*() */
/* =============== SDRAM Test ============================================== */

#ifdef CONFIG_IMX8MM
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

#define BT_TBS2				0

/* Features set in fs_nboot_args.chFeature2 (available since NBoot VN27) */
#define FEAT2_TBS2_ETH		(1<<0)		/* 0: no LAN, 1; has LAN */
#define FEAT2_TBS2_EMMC		(1<<1)		/* 0: no eMMC, 1: has eMMC */
#define FEAT2_TBS2_WLAN		(1<<2)		/* 0: no WLAN, 1: has WLAN */
#endif

int audio_present(){
	int ret = 0;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args ();

	switch (fs_board_get_type())
	{
#ifdef CONFIG_IMX8MM
	case BT_PICOCOREMX8MM:
		if(pargs->chFeatures2 & FEAT2_8MM_AUDIO)
			ret = 1;
		break;
	case BT_PICOCOREMX8MX:
		if(pargs->chFeatures2 & FEAT2_8MX_AUDIO)
			ret = 1;
		break;
#endif
	}
	return ret;
}

int can_present(){
	int ret = 0;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args ();

	switch (fs_board_get_type())
	{
#ifdef CONFIG_IMX8MM
	case BT_PICOCOREMX8MM:
		break;
	case BT_PICOCOREMX8MX:
		if(pargs->chFeatures2 & FEAT2_8MX_CAN)
			ret = 1;
		break;
#endif
	}
	return ret;
}

int sec_present(){
	int ret = 0;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args ();

	switch (fs_board_get_type())
	{
#ifdef CONFIG_IMX8MM
	case BT_PICOCOREMX8MM:
		break;
	case BT_PICOCOREMX8MX:
		if(pargs->chFeatures2 & FEAT2_8MX_SEC_CHIP)
			ret = 1;
		break;
#endif
	}
	return ret;
}

int wlan_present(){
	int ret = 0;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args ();

	switch (fs_board_get_type())
	{
#ifdef CONFIG_IMX8MM
#if defined(CONFIG_TARGET_TBS2)
	case BT_TBS2:
		if(pargs->chFeatures2 & FEAT2_TBS2_WLAN)
			ret = 1;
		break;
#else
	case BT_PICOCOREMX8MM:
		if(pargs->chFeatures2 & FEAT2_8MM_WLAN)
			ret = 1;
		break;
	case BT_PICOCOREMX8MX:
		break;
#endif
#endif
	}
	return ret;
}

