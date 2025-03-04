// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2021 NXP
 */

#include <common.h>
#include <display_options.h>
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
#include <linux/delay.h>
#include <linux/compat.h>

#include "../drivers/net/fsl_enetc.h"
#include "../common/fs_eth_common.h"
#include "../common/fs_ls1028a_common.h"
#include "../common/fs_common.h"
#include "../common/fs_fdt_common.h"

/* GPIO-NAMES */
#define GPIO_PCIe_CLK_EN "gpio@22_2"
#define GPIO_PCIe_SIM_SW "gpio@22_3"
#define GPIO_PCIe1_PWR_EN "gpio@22_11"
#define GPIO_PCIe1_RST_B "gpio@22_12"
#define GPIO_PCIe2_PWR_EN "gpio@22_14"
#define GPIO_PCIe2_RST_B "gpio@22_15"
#define GPIO_RGMII_RESET_NAME "gpio@22_19"
#define GPIO_QSGMII_RESET_NAME "gpio@22_21"
#define GPIO_USB_VBUS_EN "MPC@0232000018"
#define GPIO_SFP_INT_LINK_LED "gpio@22_23"

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

static void clear_pci_perst(void){
	fs_set_gpio(GPIO_PCIe1_RST_B, 1);
	fs_set_gpio(GPIO_PCIe2_RST_B, 1);
}

static void set_pci_perst(void){
	fs_set_gpio(GPIO_PCIe1_RST_B, 0);
	fs_set_gpio(GPIO_PCIe2_RST_B, 0);
}

static void prepare_pci(void){
	set_pci_perst();

	/*	Set mPCIe (right) -> right SIM
	 *	and M.2 (left) -> left SIM
	 */
	fs_set_gpio(GPIO_PCIe_SIM_SW, 0);

	/* Enable PCIe Power */
	fs_set_gpio(GPIO_PCIe1_PWR_EN, 1);
	fs_set_gpio(GPIO_PCIe2_PWR_EN, 1);
	
	/* Enable clock */
	fs_set_gpio(GPIO_PCIe_CLK_EN, 0);
}

/* Set GPIO Reset-Pins for Eth.-PHYs */
static void prepare_eth(void){
	/* Disable SFP_LINK_LED per default */
	fs_set_gpio(GPIO_SFP_INT_LINK_LED, 1);

	/* Reset QSGMII Phy */
	fs_set_gpio(GPIO_QSGMII_RESET_NAME, 1);

	/* Realtek Phy needs some extra help to read the correct PHYAD[2:0] values.
	 *
	 * When the board is powered up, phy starts and after 12.5ms it gets a
	 * reset signal from a pull-up to 1v8. This leads to a misbehaviour of
	 * the phy after clearing the reset, where it can't read its configured 
	 * MDIO address.
	 * 
	 * Long story short, the power sequence of the PHY must not receive a
	 * reset signal within 100ms.
	 */
	fs_set_gpio(GPIO_RGMII_RESET_NAME, 0);
	udelay(150000); // min 100ms for Power Sequence needed
	fs_set_gpio(GPIO_RGMII_RESET_NAME, 1);
	udelay(20000);  // min 10ms for reset needed
	fs_set_gpio(GPIO_RGMII_RESET_NAME, 0);

	fs_set_gpio(GPIO_QSGMII_RESET_NAME, 0);
}

static void prepare_usb(void){
	fs_set_gpio(GPIO_USB_VBUS_EN, 1);
}

static void prepare_rtc(void){
	struct udevice *dev = NULL;
	uint8_t buf[1] = { 0x2 };

	i2c_get_chip_for_busnum(0, 0x51, 1, &dev);

	if(!dev)
		return;

	dm_i2c_write(dev, 0x25, buf, 1);
}

int board_init(void)
{
#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

	prepare_pci();
	prepare_eth();
	prepare_usb();
	prepare_rtc();

#ifndef CONFIG_SYS_EARLY_PCI_INIT
	clear_pci_perst();
	
	/* some devices needs some time after reset clear */
	udelay(50000);
	pci_init();
#endif

	return 0;
}

int fsl_board_late_init(void){
	enum board_type btype;
	btype = fs_get_board();

	switch(btype) {
		case GAL1:
			env_set("dtb", "fsgal1.dtb");
			break;
		case GAL2:
			env_set("dtb", "fsgal2.dtb");
			break;
		default:
			break;
	}

	fs_set_macaddrs();
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
#if defined(CONFIG_SYS_I2C_EARLY_INIT) && defined(CONFIG_SPL_BUILD)
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

#ifdef CONFIG_OF_BOARD_FIXUP
int board_fix_fdt(void *rw_fdt_blob)
{
	fs_fdt_board_setup(rw_fdt_blob);
	fs_ubootfdt_board_setup(rw_fdt_blob);
	return 0;
}
#endif

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	uint32_t features;
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

	fs_fdt_board_setup(blob);
	fs_linuxfdt_board_setup(blob);

	features = fs_get_board_features();
	if( (features & FEAT_GAL_ETH_INTERN_BASEX) ==
			FEAT_GAL_ETH_INTERN_BASEX){
		fs_fdt_enable_node_by_label(blob, "sfp_int", 1);
	}
	return 0;
}
#endif

int checkboard(void)
{
	char modelname[MODELSTRLEN] = {0};

	/*Print Board-Info*/
	fs_get_modelname(modelname, MODELSTRLEN);
	printf("Board: %s\n", modelname);

	/* Print Boot-SRC */
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
