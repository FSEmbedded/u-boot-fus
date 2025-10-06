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

#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx8ulp-pins.h>
#include <fsl_sec.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <asm/arch/ddr.h>
#include <asm/arch/rdc.h>
#include <asm/arch/upower.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/sections.h>
#include <asm/mach-imx/boot_mode.h>
#include <power/regulator.h>
#include <hang.h>

#include "fsimx8ulp.h"
#include "../common/fs_bootrom.h"
#include "../common/fs_cntr_common.h"
#include "../common/fs_image_common.h"

DECLARE_GLOBAL_DATA_PTR;

static struct dram_timing_info2 *_dram_timing;

int fs_board_init_dram_data(unsigned long *ptr){
	if(!ptr)
		return -ENODATA;

	_dram_timing = (struct dram_timing_info2 *)ptr;

	return 0;
}

void spl_dram_init(void)
{
	struct dram_timing_info2 *dtiming = _dram_timing;

	/* Reboot in dual boot setting no need to init ddr again */
	bool ddr_enable = pcc_clock_is_enable(5, LPDDR4_PCC5_SLOT);

	if (!ddr_enable) {
		init_clk_ddr();
		printf("DDR: %uMTS\n", dtiming->fsp_table[2]);
		ddr_init(dtiming);
	} else {
		/* reinit pfd/pfddiv and lpavnic except pll4*/
		cgc2_pll4_init(false);
	}
	
	dram_init();
}

u32 spl_boot_device(void)
{
#if CONFIG_IS_ENABLED(BOOTROM_SUPPORT)
	return BOOT_DEVICE_BOOTROM;
#else
	return BOOT_DEVICE_NONE;
#endif
}

#define PMIC_I2C_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_SRE_SLOW | PAD_CTL_ODE)
#define PMIC_MODE_PAD_CTRL	(PAD_CTL_PUS_UP)

static iomux_cfg_t const pmic_ptb_pads[] = {
	IMX8ULP_PAD_PTB7__PMIC0_MODE2 | MUX_PAD_CTRL(PMIC_MODE_PAD_CTRL),
	IMX8ULP_PAD_PTB8__PMIC0_MODE1 | MUX_PAD_CTRL(PMIC_MODE_PAD_CTRL),
	IMX8ULP_PAD_PTB9__PMIC0_MODE0 | MUX_PAD_CTRL(PMIC_MODE_PAD_CTRL),
	IMX8ULP_PAD_PTB11__PMIC0_SCL | MUX_PAD_CTRL(PMIC_I2C_PAD_CTRL),
	IMX8ULP_PAD_PTB10__PMIC0_SDA | MUX_PAD_CTRL(PMIC_I2C_PAD_CTRL),
};

void setup_iomux_pmic(void)
{
	switch(gd->board_type) {
		case BT_PICOCOREMX8ULP:
		case BT_OSMSFMX8ULP:
		case BT_ARMSTONEMX8ULP:
		case BT_SOLDERCOREMX8ULP:
			imx8ulp_iomux_setup_multiple_pads(pmic_ptb_pads, ARRAY_SIZE(pmic_ptb_pads));
			return;
		default:
			return;
	}
}

int power_init_board(void)
{
	u32 tmp;

	/* Set buck2 ramp-up speed 1us */
	upower_pmic_i2c_write(0x14, 0x39);
	/* Set buck3 ramp-up speed 1us */
	upower_pmic_i2c_write(0x21, 0x39);
	/* Set buck3out min limit 0.625v */
	upower_pmic_i2c_write(0x2d, 0x2);

	if (IS_ENABLED(CONFIG_IMX8ULP_ND_MODE)) {
		/* Set buck3 to 1.0v ND */
		upower_pmic_i2c_write(0x22, 0x20);
	} else {
		/* Set buck3 to 1.1v OD */
		upower_pmic_i2c_write(0x22, 0x28);
	}

	/* Reset immediately after the PMIC_RST_B pin goes low */
	upower_pmic_i2c_read(0x0b, &tmp);
	tmp &= ~0x1c;
	upower_pmic_i2c_write(0x0b, tmp);

	return 0;
}

void display_ele_fw_version(void)
{
	u32 fw_version, sha1, res = 0;
	int ret;

	ret = ele_get_fw_version(&fw_version, &sha1, &res);
	if (ret) {
		printf("ele get firmware version failed %d, 0x%x\n", ret, res);
	} else {
		printf("ELE firmware version %u.%u.%u-%x",
		       (fw_version & (0x00ff0000)) >> 16,
		       (fw_version & (0x0000fff0)) >> 4,
		       (fw_version & (0x0000000f)), sha1);
		((fw_version & (0x80000000)) >> 31) == 1 ? puts("-dirty\n") : puts("\n");
	}
}

/**
 * According to RM, a defined Voltage Range
 * will reduce power consumption
 */
static void sim_pad_setup(void)
{
	switch(gd->board_type){
		case BT_PICOCOREMX8ULP:
			set_apd_gpiox_op_range(PTE, RANGE_3V3V);
			set_apd_gpiox_op_range(PTF, RANGE_3V3V);
			break;
		case BT_ARMSTONEMX8ULP:
		case BT_OSMSFMX8ULP:
			set_apd_gpiox_op_range(PTE, RANGE_1P8V);
			set_apd_gpiox_op_range(PTF, RANGE_AUTO);
			break;
		case BT_SOLDERCOREMX8ULP:
			set_apd_gpiox_op_range(PTE, RANGE_AUTO);
			set_apd_gpiox_op_range(PTF, RANGE_AUTO);
			break;
		default:
			break;
	}
}

#if CONFIG_IS_ENABLED(MULTI_DTB_FIT)
int board_fit_config_name_match(const char *name)
{
	CHECK_BOARD_TYPE_AND_NAME("picocoremx8ulp", BT_PICOCOREMX8ULP, name);
	CHECK_BOARD_TYPE_AND_NAME("fs-osm-sf-mx8ulp-adp-osm-bb",
					BT_OSMSFMX8ULP, name);

    return -EINVAL;
}
#endif

static int set_gd_board_type(void)
{
	const char *board_id;
	const char *ptr;
	int len;

	board_id = fs_image_get_board_id();
	ptr = strchr(board_id, '-');
	len = (int)(ptr - board_id);

	SET_BOARD_TYPE("PCoreMX8ULP", BT_PICOCOREMX8ULP, board_id, len);
	SET_BOARD_TYPE("OSM8ULP", BT_OSMSFMX8ULP, board_id, len);
	SET_BOARD_TYPE("aStone8ULP", BT_ARMSTONEMX8ULP, board_id, len);
	SET_BOARD_TYPE("SCoreMX8ULP", BT_SOLDERCOREMX8ULP, board_id, len);

	return -EINVAL;
}

static void lpuart_postinit(ulong lpuart_base)
{
	u32 index = 0, i;

	const u32 lpuart_array[] = {
		LPUART4_RBASE,
		LPUART5_RBASE,
		LPUART6_RBASE,
		LPUART7_RBASE,
	};

	const u32 lpuart_pcc_slots[] = {
		LPUART4_PCC3_SLOT,
		LPUART5_PCC3_SLOT,
		LPUART6_PCC4_SLOT,
		LPUART7_PCC4_SLOT,
	};

	const u32 lpuart_pcc[] = {
		3, 3, 4, 4,
	};

	if(lpuart_base == LPUART_BASE)
		return;
	
	/* Disable default LPUART CLK */
	for (i = 0; i < 4; i++) {
		if (lpuart_array[i] == LPUART_BASE) {
			index = i;
			break;
		}
	}

	if (index > 3)
		return;

	pcc_clock_enable(lpuart_pcc[index], lpuart_pcc_slots[index], false);
	
	/* Enable new lpuart clk */
	init_clk_lpuart(lpuart_base);
}

int board_early_init_f(void)
{
	int rescan = 0;

	set_gd_board_type();

	/**
	 * arch_cpu_init() initializes the lpuart clk for LPUART_BASE,
	 * some boards need a reinit.
	 */
	switch(gd->board_type){
	case BT_PICOCOREMX8ULP:
	case BT_OSMSFMX8ULP:
		break;
	case BT_ARMSTONEMX8ULP:
		lpuart_postinit(LPUART7_RBASE);
		break;
	default:
		break;
	}

	/* reinit dm */
	fdtdec_resetup(&rescan);

	if(!rescan)
		return 0;
	
	dm_uninit();
	return dm_init_and_scan(!CONFIG_IS_ENABLED(OF_PLATDATA));
}

void board_init_f(ulong dummy)
{
	u32 res;
	struct udevice *dev;
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	timer_init();

	arch_cpu_init();

	/* Setup default Devicetree */
	spl_early_init();


	/* Load Board ID to know the Board in early state*/
	fs_cntr_load_board_id();

	/* Setup Multiple Devicetree */
	ret = board_early_init_f();

	preloader_console_init();

	print_bootstage();

	print_devinfo();

	regulators_enable_boot_on(false);

	if(ret)
		printf("DM_INIT FAILED!: %d\n", ret);

	ret = imx8ulp_dm_post_init();
	if (ret)
		printf("imx8ulp_dm_post_init failed\n");

	display_ele_fw_version();

	/* Set iomuxc0 for pmic when m33 is not booted */
	if (!m33_image_booted())
		setup_iomux_pmic();

	/* Load the lposc fuse to work around ROM issue. The fuse depends on ELE to read. */
	if (is_soc_rev(CHIP_REV_1_0))
		load_lposc_fuse();

	upower_init();

	power_init_board();

	clock_init_late();

	/* This must place after upower init, so access to MDA and MRC are valid */
	/* Init XRDC MDA  */
	xrdc_init_mda();

	/* Init XRDC MRC for VIDEO, DSP domains */
	xrdc_init_mrc();

	xrdc_init_pdac_msc();


	/* Load F&S NBOOT-Images */
	fs_cntr_init(true);

	/* DDR initialization */
	spl_dram_init();

	/* Call it after PS16 power up */
	set_lpav_qos();

	sim_pad_setup();

	/* Enable A35 access to the CAAM */
	ret = ele_release_caam(0x7, &res);
	if (!ret) {
		if (((res >> 8) & 0xff) == ELE_NON_SECURE_STATE_FAILURE_IND)
			printf("Warning: CAAM is in non-secure state, 0x%x\n", res);

		/* Only two UCLASS_MISC devicese are present on the platform. There
		 * are MU and CAAM. Here we initialize CAAM once it's released by
		 * ELE firmware..
		 */
		if (IS_ENABLED(CONFIG_FSL_CAAM)) {
			ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(caam_jr), &dev);
			if (ret)
				printf("Failed to initialize caam_jr: %d\n", ret);
		}
	}

	/*
	 * RNG start only available on the A1 soc revision.
	 * Check some JTAG register for the SoC revision.
	 */
	if (!is_soc_rev(CHIP_REV_1_0)) {
		ret = ele_start_rng();
		if (ret)
			printf("Fail to start RNG: %d\n", ret);
	}

	board_init_r(NULL, 0);
}
