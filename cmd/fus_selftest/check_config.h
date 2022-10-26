/*
 * check_config.h
 *
 *  Created on: Mar 08, 2021
 *      Author: developer
 */

#ifndef CMD_FUS_SELFTEST_CHECK_CONFIG_H_
#define CMD_FUS_SELFTEST_CHECK_CONFIG_H_
/* Board features; hast to be in sync with fsimx8mm.c */
#ifdef CONFIG_IMX8MM
#define FEAT_ETH_A	(1<<0)
#define FEAT_ETH_B	(1<<1)
#define FEAT_ETH_A_PHY	(1<<2)
#define FEAT_ETH_B_PHY	(1<<3)
#define FEAT_NAND	(1<<4)
#define FEAT_EMMC	(1<<5)
#define FEAT_AUDIO	(1<<6)
#define FEAT_WLAN	(1<<7)
#define FEAT_LVDS	(1<<8)
#define FEAT_MIPI_DSI	(1<<9)
#define FEAT_RTC85063	(1<<10)
#define FEAT_RTC85263	(1<<11)
#define FEAT_EXT_RTC ((1<<10) | (1<<11))
#define FEAT_SEC_CHIP	(1<<12)
#define FEAT_CAN	(1<<13)
#define FEAT_EEPROM	(1<<14)
#endif

#ifdef CONFIG_IMX8MP
#define FEAT_ETH_A 	(1<<0)	/* 0: no LAN0,  1: has LAN0 */
#define FEAT_ETH_B	(1<<1)	/* 0: no LAN1,  1: has LAN1 */
#define FEAT_DISP_A	(1<<2)	/* 0: MIPI-DSI, 1: LVDS lanes 0-3 */
#define FEAT_DISP_B	(1<<3)	/* 0: HDMI,     1: LVDS lanes 0-4 or (4-7 if DISP_A=1) */
#define FEAT_AUDIO 	(1<<4)	/* 0: no Audio, 1: Analog Audio Codec */
#define FEAT_WLAN	(1<<5)	/* 0: no WLAN,  1: has WLAN */
#define FEAT_EXT_RTC	(1<<6)	/* 0: internal RTC, 1: external RTC */
#define FEAT_NAND	(1<<7)	/* 0: no NAND,  1: has NAND */
#define FEAT_EMMC	(1<<8)	/* 0: no EMMC,  1: has EMMC */
#define FEAT_SEC_CHIP	(1<<9)	/* 0: no SE050,  1: has SE050 */
#define FEAT_EEPROM	(1<<10)	/* 0: no EEPROM,  1: has EEPROM */
#endif
int has_feature(int feature);
int get_board_fert(char *fert);
#endif /* CMD_FUS_SELFTEST_CHECK_CONFIG_H_ */
