/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * common code for ls1028a
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FS_CONFIG__
#define __FS_CONFIG__

#include <linux/types.h>
#define MODELSTRLEN 64

/* Defined features for LS1028a boards. */
#define FEAT_ETH_ENETC0 BIT(0)	/* 0: no ENETC0; 1: use ENETC0 */
#define FEAT_ETH_ENETC1 BIT(1)	/* 0: no ENETC1; 1: use ENETC1 */
#define FEAT_ETH_SWP1	BIT(2)	/* 0: no SWPx; 1: use SWPx */
#define FEAT_ETH_SWP2	BIT(3)
#define FEAT_ETH_SWP3	BIT(4)
#define FEAT_ETH_SWP4	BIT(5)
#define FEAT_USB1		BIT(6)	/* 0: no USB1; 1: use USB2 */
#define FEAT_USB2		BIT(7)
#define FEAT_LPUART1	BIT(8)	/* 0: no LPUARTx; 1: use LPUARTx */
#define FEAT_LPUART2	BIT(9)
#define FEAT_LPUART3	BIT(10)
#define FEAT_LPUART4	BIT(11)
#define FEAT_LPUART5	BIT(12)
#define FEAT_LPUART6	BIT(13)
#define FEAT_I2C1		BIT(14)	/* 0: no I2Cx; 1: use I2Cx */
#define FEAT_I2C2		BIT(15)
#define FEAT_I2C3		BIT(16)
#define FEAT_I2C4		BIT(17)
#define FEAT_I2C5		BIT(18)
#define FEAT_I2C6		BIT(19)
#define FEAT_ESDHC0		BIT(20)	/* 0: no ESDHCx; 1: use ESDHCx */
#define FEAT_ESDHC1		BIT(21)
#define FEAT_I2C_TEMP	BIT(22)	/* 0: no on Board Temp Sensor; 1: on Board Temp Sensor */
#define FEAT_LPUART1_RS485	BIT(23)
#define FEAT_LPUART3_RS485	BIT(24)

/* Define feature alias */
// Alias for GAL Boards
#define FEAT_GAL_ETH_SFP		   (FEAT_ETH_ENETC0 | FEAT_I2C2)
#define FEAT_GAL_ETH_INTERN		FEAT_ETH_ENETC1
#define FEAT_GAL_ETH_INTERN_BASET  FEAT_ETH_ENETC1
#define FEAT_GAL_ETH_INTERN_BASEX  (FEAT_ETH_ENETC1 | FEAT_I2C5)
#define FEAT_GAL_ETH_RJ45_1		FEAT_ETH_SWP1
#define FEAT_GAL_ETH_RJ45_2		FEAT_ETH_SWP2
#define FEAT_GAL_ETH_RJ45_3		FEAT_ETH_SWP3
#define FEAT_GAL_ETH_RJ45_4		FEAT_ETH_SWP4
#define FEAT_GAL_RS485A			(FEAT_LPUART1 | FEAT_LPUART1_RS485)
#define FEAT_GAL_RS485B			(FEAT_LPUART3 | FEAT_LPUART3_RS485)
#define FEAT_GAL_RS232			 FEAT_LPUART3
#define FEAT_GAL_MMC			   FEAT_ESDHC1
#define FEAT_GAL_SD				FEAT_ESDHC0

enum board_type {
	GAL1,
	GAL2,
};

enum board_rev {
	REV10,
	REV11,
	REV12,
	REV13,
};

enum board_config {
	FERT1,
	FERT2,
	FERT3,
	FERT4,
	FERT5,
	FERT6,
	FERT7,
	FERT8,
	FERT9,
	FERT10,
	FERT11,
	FERT12,
	FERT13,
	FERT14,
	FERT15,
	FERT16,
};

/**
 * @brief Define Board specific features
 * 
 * @return uint32_t 
 * 
 * @details
 * Define Board specific features based on board-type
 * board-revision and configuration.
 */
uint32_t fs_get_board_features(void);

enum board_config fs_get_board_config(void);
enum board_rev fs_get_board_rev(void);
enum board_type fs_get_board(void);

/**
 * @brief setup Linux and U-Boot devicetree for specific F&S-Layerscape Boards
 * 
 * @param blob Pointer to FDT
 * @return int 
 * 
 * @details
 * Here the defined board features specify changes in Linux and U-Boot DTB.
 * Based on the features, the nodes are disabled or enabled and the properties within
 * the nodes are modified.
 */
void fs_fdt_board_setup(void *blob);
void fs_linuxfdt_board_setup(void *blob);
void fs_ubootfdt_board_setup(void *blob);

/**
 * @brief read OUID Reg to determine model name
 * 
 * @param buf Pointer to char array
 * @param buf_size size of char array
 */
void fs_get_modelname(char* buf, int buf_size);
#endif // __FS_CONFIG__
