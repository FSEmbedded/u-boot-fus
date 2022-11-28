// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <errno.h>
#include <fsl_ddr.h>
#include <net.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <hwconfig.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <env_internal.h>
#include <asm/arch-fsl-layerscape/soc.h>
#include <asm/arch-fsl-layerscape/fsl_icid.h>
#include <i2c.h>
#include <asm/arch/soc.h>
#ifdef CONFIG_FSL_LS_PPA
#include <asm/arch/ppa.h>
#endif
#include <fsl_immap.h>
#include <netdev.h>

#include <fdtdec.h>
#include <miiphy.h>
#include "../drivers/net/fsl_enetc.h"

DECLARE_GLOBAL_DATA_PTR;

int config_board_mux(void)
{
	return 0;
}

#ifdef CONFIG_LPUART
u32 get_lpuart_clk(void)
{
	return gd->bus_clk / CONFIG_SYS_FSL_LPUART_CLK_DIV;
}
#endif

int board_init(void)
{
#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

#ifndef CONFIG_SYS_EARLY_PCI_INIT
	pci_init();
#endif
	return 0;
}

int board_eth_init(struct bd_info *bis)
{
	return pci_eth_init(bis);
}

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	config_board_mux();

	return 0;
}
#endif

int board_early_init_f(void)
{
#ifdef CONFIG_SYS_I2C_EARLY_INIT
	i2c_early_init_f();
#endif

	fsl_lsch3_early_init_f();

	return 0;
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
	print_size(gd->bd->bi_dram[0].size + gd->bd->bi_dram[1].size, "");
	print_ddr_info(0);
}

int esdhc_status_fixup(void *blob, const char *compat)
{
	void __iomem *dcfg_ccsr = (void __iomem *)DCFG_BASE;
	char esdhc1_path[] = "/soc/mmc@2140000";
	char esdhc2_path[] = "/soc/mmc@2150000";
	char dspi1_path[] = "/soc/spi@2100000";
	char dspi2_path[] = "/soc/spi@2110000";
	u32 mux_sdhc1, mux_sdhc2;
	u32 io = 0;

	/*
	 * The PMUX IO-expander for mux select is used to control
	 * the muxing of various onboard interfaces.
	 */

	io = in_le32(dcfg_ccsr + DCFG_RCWSR12);
	mux_sdhc1 = (io >> DCFG_RCWSR12_SDHC_SHIFT) & DCFG_RCWSR12_SDHC_MASK;

	/* Disable esdhc1/dspi1 if not selected. */
	if (mux_sdhc1 != 0)
		do_fixup_by_path(blob, esdhc1_path, "status", "disabled",
				 sizeof("disabled"), 1);
	if (mux_sdhc1 != 2)
		do_fixup_by_path(blob, dspi1_path, "status", "disabled",
				 sizeof("disabled"), 1);

	io = in_le32(dcfg_ccsr + DCFG_RCWSR13);
	mux_sdhc2 = (io >> DCFG_RCWSR13_SDHC_SHIFT) & DCFG_RCWSR13_SDHC_MASK;

	/* Disable esdhc2/dspi2 if not selected. */
	if (mux_sdhc2 != 0)
		do_fixup_by_path(blob, esdhc2_path, "status", "disabled",
				 sizeof("disabled"), 1);
	if (mux_sdhc2 != 2)
		do_fixup_by_path(blob, dspi2_path, "status", "disabled",
				 sizeof("disabled"), 1);

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	ft_cpu_setup(blob, bd);

	/* fixup DT for the two GPP DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

#ifdef CONFIG_RESV_RAM
	/* reduce size if reserved memory is within this bank */
	if (gd->arch.resv_ram >= base[0] &&
	    gd->arch.resv_ram < base[0] + size[0])
		size[0] = gd->arch.resv_ram - base[0];
	else if (gd->arch.resv_ram >= base[1] &&
		 gd->arch.resv_ram < base[1] + size[1])
		size[1] = gd->arch.resv_ram - base[1];
#endif

	fdt_fixup_memory_banks(blob, base, size, 2);

	fdt_fixup_icid(blob);

#ifdef CONFIG_FSL_ENETC
	fdt_fixup_enetc_mac(blob);
#endif

	return 0;
}
#endif

int checkboard(void)
{
#ifdef CONFIG_TFABOOT
	enum boot_src src = get_boot_src();
#endif
	puts("BOOT-SRC: ");
#ifdef CONFIG_TFABOOT
	if (src == BOOT_SOURCE_SD_MMC) {
		puts("SD\n");
	} else if (src == BOOT_SOURCE_SD_MMC2) {
		puts("eMMC\n");
	} else {
#endif
#if defined(CONFIG_SD_BOOT)
		puts("SD\n");
#elif defined(CONFIG_EMMC_BOOT)
		puts("eMMC\n");
#else
		puts("unknown\n");
#endif
#ifdef CONFIG_TFABOOT
	}
#endif
	return 0;
}

void *video_hw_init(void)
{
	return NULL;
}
