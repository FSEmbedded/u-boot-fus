/*
 * check_config.h
 *
 *  Created on: Mar 08, 2021
 *      Author: developer
 */

#ifndef CMD_FUS_SELFTEST_CHECK_CONFIG_H_
#define CMD_FUS_SELFTEST_CHECK_CONFIG_H_

#ifdef CONFIG_IMX8MM
/* Board features; hast to be in sync with fsimx8mm.c */
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
#endif

int has_feature(int feature);
#endif /* CMD_FUS_SELFTEST_CHECK_CONFIG_H_ */
