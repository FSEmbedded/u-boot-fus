// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Copyright (C) 2004-2015 Freescale Semiconductor, Inc.
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com)
 */

#include <common.h>
#include <clock_legacy.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/global_data.h>

#ifdef CONFIG_FSL_ESDHC_IMX
DECLARE_GLOBAL_DATA_PTR;
#endif

int get_clocks(void)
{
#ifdef CONFIG_FSL_ESDHC_IMX
#ifdef CONFIG_FSL_USDHC
#if CFG_SYS_FSL_ESDHC_ADDR == USDHC2_BASE_ADDR
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
#if !defined(CONFIG_MX6UL) && !defined(CONFIG_MX6ULL)
#elif CFG_SYS_FSL_ESDHC_ADDR == USDHC3_BASE_ADDR
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
#elif CFG_SYS_FSL_ESDHC_ADDR == USDHC4_BASE_ADDR
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
#endif
#else
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
#endif
#else
#if CFG_SYS_FSL_ESDHC_ADDR == MMC_SDHC2_BASE_ADDR
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
#elif CFG_SYS_FSL_ESDHC_ADDR == MMC_SDHC3_BASE_ADDR
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
#elif CFG_SYS_FSL_ESDHC_ADDR == MMC_SDHC4_BASE_ADDR
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
#else
	gd->arch.sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
#endif
#endif
#endif
	return 0;
}
