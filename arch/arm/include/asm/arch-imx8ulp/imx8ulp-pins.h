/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021 NXP
 */

#ifndef __ASM_ARCH_IMX8ULP_PINS_H__
#define __ASM_ARCH_IMX8ULP_PINS_H__

#include <asm/arch/iomux.h>

enum {
	IMX8ULP_PAD_PTA3__TPM0_CH2                         = IOMUX_PAD(0x000c, 0x000c, IOMUX_CONFIG_MPORTS | 0x6, 0x0948, 0x1, 0),
	IMX8ULP_PAD_PTA8__LPI2C0_SCL                       = IOMUX_PAD(0x0020, 0x0020, IOMUX_CONFIG_MPORTS | 0x5, 0x097c, 0x2, 0),
	IMX8ULP_PAD_PTA9__LPI2C0_SDA                       = IOMUX_PAD(0x0024, 0x0024, IOMUX_CONFIG_MPORTS | 0x5, 0x0980, 0x2, 0),

	IMX8ULP_PAD_PTB7__PMIC0_MODE2                       = IOMUX_PAD(0x009C, 0x009C, IOMUX_CONFIG_MPORTS | 0xA, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB8__PMIC0_MODE1                       = IOMUX_PAD(0x00A0, 0x00A0, IOMUX_CONFIG_MPORTS | 0xA, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB9__PMIC0_MODE0                       = IOMUX_PAD(0x00A4, 0x00A4, IOMUX_CONFIG_MPORTS | 0xA, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB10__PMIC0_SDA                        = IOMUX_PAD(0x00A8, 0x00A8, IOMUX_CONFIG_MPORTS | 0xA, 0x0804, 0x2, 0),
	IMX8ULP_PAD_PTB11__PMIC0_SCL                        = IOMUX_PAD(0x00AC, 0x00AC, IOMUX_CONFIG_MPORTS | 0xA, 0x0800, 0x2, 0),

	IMX8ULP_PAD_PTB7__GPIO_PTB7                         = IOMUX_PAD(0x009C, 0x009C, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB8__GPIO_PTB8                         = IOMUX_PAD(0x00A0, 0x00A0, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB9__GPIO_PTB9                         = IOMUX_PAD(0x00A4, 0x00A4, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB10__GPIO_PTB10                       = IOMUX_PAD(0x00A8, 0x00A8, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTB11__GPIO_PTB11                       = IOMUX_PAD(0x00AC, 0x00AC, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),

	IMX8ULP_PAD_PTA3__GPIO_PTA3                         = IOMUX_PAD(0x000C, 0x000C, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA4__GPIO_PTA4                         = IOMUX_PAD(0x0010, 0x0010, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA8__GPIO_PTA8                         = IOMUX_PAD(0x0020, 0x0020, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA12__GPIO_PTA12                       = IOMUX_PAD(0x0030, 0x0030, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA13__GPIO_PTA13                       = IOMUX_PAD(0x0034, 0x0034, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),

	IMX8ULP_PAD_PTA3__PMIC0_MODE2                       = IOMUX_PAD(0x000C, 0x000C, IOMUX_CONFIG_MPORTS | 0xA, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA4__PMIC0_MODE1                       = IOMUX_PAD(0x0010, 0x0010, IOMUX_CONFIG_MPORTS | 0xA, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA8__PMIC0_MODE0                       = IOMUX_PAD(0x0020, 0x0020, IOMUX_CONFIG_MPORTS | 0xA, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTA12__PMIC0_SDA                        = IOMUX_PAD(0x0030, 0x0030, IOMUX_CONFIG_MPORTS | 0xA, 0x0804, 0x1, 0),
	IMX8ULP_PAD_PTA13__PMIC0_SCL                        = IOMUX_PAD(0x0034, 0x0034, IOMUX_CONFIG_MPORTS | 0xA, 0x0800, 0x1, 0),

	IMX8ULP_PAD_PTC0__FLEXSPI0_A_DQS					= IOMUX_PAD(0x0100, 0x0100, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC1__FLEXSPI0_A_DATA7					= IOMUX_PAD(0x0104, 0x0104, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC2__FLEXSPI0_A_DATA6					= IOMUX_PAD(0x0108, 0x0108, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC3__FLEXSPI0_A_DATA5					= IOMUX_PAD(0x010c, 0x010c, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC4__FLEXSPI0_A_DATA4					= IOMUX_PAD(0x0110, 0x0110, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC5__FLEXSPI0_A_SS0_b					= IOMUX_PAD(0x0114, 0x0114, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC6__FLEXSPI0_A_SCLK					= IOMUX_PAD(0x0118, 0x0118, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC7__FLEXSPI0_A_DATA3					= IOMUX_PAD(0x011c, 0x011c, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC8__FLEXSPI0_A_DATA2					= IOMUX_PAD(0x0120, 0x0120, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC9__FLEXSPI0_A_DATA1					= IOMUX_PAD(0x0124, 0x0124, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC10__FLEXSPI0_A_DATA0					= IOMUX_PAD(0x0128, 0x0128, IOMUX_CONFIG_MPORTS | 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTC10__PTC10						= IOMUX_PAD(0x0128, 0x0128, IOMUX_CONFIG_MPORTS | 0x1, 0x0000, 0x0, 0),


	IMX8ULP_PAD_PTD0__SDHC0_RESET_b                     = IOMUX_PAD(0x0000, 0x0000, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD1__SDHC0_CMD                         = IOMUX_PAD(0x0004, 0x0004, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD2__SDHC0_CLK                         = IOMUX_PAD(0x0008, 0x0008, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD3__SDHC0_D7                          = IOMUX_PAD(0x000C, 0x000C, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD4__SDHC0_D6                          = IOMUX_PAD(0x0010, 0x0010, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD5__SDHC0_D5                          = IOMUX_PAD(0x0014, 0x0014, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD6__SDHC0_D4                          = IOMUX_PAD(0x0018, 0x0018, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD7__SDHC0_D3                          = IOMUX_PAD(0x001C, 0x001C, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD8__SDHC0_D2                          = IOMUX_PAD(0x0020, 0x0020, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD9__SDHC0_D1                          = IOMUX_PAD(0x0024, 0x0024, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD10__SDHC0_D0                         = IOMUX_PAD(0x0028, 0x0028, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD11__SDHC0_DQS                        = IOMUX_PAD(0x002C, 0x002C, 0x8, 0x0000, 0x0, 0),

	IMX8ULP_PAD_PTD11__FLEXSPI2_B_SS0_B                 = IOMUX_PAD(0x002C, 0x002C, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD11__FLEXSPI2_A_SS1_B                 = IOMUX_PAD(0x002C, 0x002C, 0xa, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD12__FLEXSPI2_A_SS0_B                 = IOMUX_PAD(0x0030, 0x0030, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD12__FLEXSPI2_B_SS1_B                 = IOMUX_PAD(0x0030, 0x0030, 0xa, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD13__FLEXSPI2_A_SCLK                  = IOMUX_PAD(0x0034, 0x0034, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD14__FLEXSPI2_A_DATA3                 = IOMUX_PAD(0x0038, 0x0038, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD15__FLEXSPI2_A_DATA2                 = IOMUX_PAD(0x003c, 0x003c, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD16__FLEXSPI2_A_DATA1                 = IOMUX_PAD(0x0040, 0x0040, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD17__FLEXSPI2_A_DATA0                 = IOMUX_PAD(0x0044, 0x0044, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD18__FLEXSPI2_A_DQS                   = IOMUX_PAD(0x0048, 0x0048, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD19__FLEXSPI2_A_DATA7                 = IOMUX_PAD(0x004c, 0x004c, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD20__FLEXSPI2_A_DATA6                 = IOMUX_PAD(0x0050, 0x0050, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD21__FLEXSPI2_A_DATA5                 = IOMUX_PAD(0x0054, 0x0054, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD22__FLEXSPI2_A_DATA4                 = IOMUX_PAD(0x0058, 0x0058, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD23__FLEXSPI2_A_SS0_B                 = IOMUX_PAD(0x005c, 0x005c, 0x9, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTD23__FLEXSPI2_A_SCLK                  = IOMUX_PAD(0x005c, 0x005c, 0xa, 0x0000, 0x0, 0),

	IMX8ULP_PAD_PTE19__ENET0_REFCLK                     = IOMUX_PAD(0x00CC, 0x00CC, 0xA, 0x0AF4, 0x1, 0),
	IMX8ULP_PAD_PTF10__ENET0_1588_CLKIN                 = IOMUX_PAD(0x0128, 0x0128, 0x9, 0x0AD0, 0x2, 0),

	IMX8ULP_PAD_PTF11__SDHC1_RESET_b                    = IOMUX_PAD(0x012C, 0x012C, 0x8, 0x0000, 0x0, 0),
	IMX8ULP_PAD_PTF3__SDHC1_CMD                         = IOMUX_PAD(0x010C, 0x010C, 0x8, 0x0A60, 0x2, 0),
	IMX8ULP_PAD_PTF2__SDHC1_CLK                         = IOMUX_PAD(0x0108, 0x0108, 0x8, 0x0A5C, 0x2, 0),
	IMX8ULP_PAD_PTF4__SDHC1_D3                          = IOMUX_PAD(0x0110, 0x0110, 0x8, 0x0A70, 0x2, 0),
	IMX8ULP_PAD_PTF5__SDHC1_D2                          = IOMUX_PAD(0x0114, 0x0114, 0x8, 0x0A6C, 0x2, 0),
	IMX8ULP_PAD_PTF0__SDHC1_D1                          = IOMUX_PAD(0x0100, 0x0100, 0x8, 0x0A68, 0x2, 0),
	IMX8ULP_PAD_PTF1__SDHC1_D0                          = IOMUX_PAD(0x0104, 0x0104, 0x8, 0x0A64, 0x2, 0),
	IMX8ULP_PAD_PTF7__PTF7                              = IOMUX_PAD(0x011C, 0x011C, 0x1, 0x0000, 0x0, 0),
};
#endif  /* __ASM_ARCH_IMX8ULP_PINS_H__ */
