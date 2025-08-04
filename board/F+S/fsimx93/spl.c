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
#include <command.h>
#include <cpu_func.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <log.h>
#include <spl.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/sections.h>
#if defined(CONFIG_TARGET_FSIMX93)
#include <asm/arch/imx93_pins.h>
#elif defined(CONFIG_TARGET_FSIMX91)
#include <asm/arch/imx91_pins.h>
#endif
#include <asm/arch/mu.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch-mx7ulp/gpio.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/mach-imx/syscounter.h>
#include <asm/sections.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/root.h>
#include <linux/delay.h>
#include <asm/arch/clock.h>
#include <asm/arch/ccm_regs.h>
#include <asm/arch/ddr.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <power/regulator.h>
#include <asm/arch/trdc.h>

#include "../common/fs_dram_common.h"
#include "../common/fs_cntr_common.h"
#include "../common/fs_image_common.h"
#include "../common/fs_bootrom.h"
#include "fsimx93.h"

DECLARE_GLOBAL_DATA_PTR;

static struct dram_timing_info *_dram_timing;

int fs_board_init_dram_data(unsigned long *ptr){
	if(!ptr)
		return -ENODATA;

	_dram_timing = (struct dram_timing_info *)ptr;

	return 0;
}

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
#if CONFIG_IS_ENABLED(BOOTROM_SUPPORT)
	return BOOT_DEVICE_BOOTROM;
#else
	return BOOT_DEVICE_NONE;
#endif
}

void spl_board_init(void)
{
	int ret;

	ret = ele_start_rng();
	if (ret)
		printf("Fail to start RNG: %d\n", ret);

	puts("Normal Boot\n");
}

/**
 * NOTE: MUST BE CALLED AFTER FS INIT
*/
void spl_dram_init(void)
{
	struct dram_timing_info *dtiming = _dram_timing;

	printf("DDR: %uMTS\n", dtiming->fsp_msg[0].drate);
	ddr_init(dtiming);

	/* save ram info in gd */
	dram_init();
}

static int set_gd_board_type(void)
{
	const char *board_id;
	const char *ptr;
	int len;

	board_id = fs_image_get_board_id();
	ptr = strchr(board_id, '-');
	len = (int)(ptr - board_id);

#if defined(CONFIG_TARGET_FSIMX93)
	SET_BOARD_TYPE("PCoreMX93", BT_PICOCOREMX93, board_id, len);
	SET_BOARD_TYPE("OSM93", BT_OSMSFMX93, board_id, len);
	SET_BOARD_TYPE("efusMX93", BT_EFUSMX93, board_id, len);
#elif defined(CONFIG_TARGET_FSIMX91)
	SET_BOARD_TYPE("OSM91", BT_OSMSFMX91, board_id, len);
	SET_BOARD_TYPE("efusMX91", BT_EFUSMX91, board_id, len);
#endif

	return -EINVAL;
}

int board_early_init_f(void)
{
	int rescan = 0;

	set_gd_board_type();

	switch(gd->board_type) {
		case BT_PICOCOREMX93:
			imx_iomux_v3_setup_multiple_pads(lpuart7_pads, ARRAY_SIZE(lpuart7_pads));
			init_uart_clk(LPUART7_CLK_ROOT);
			break;
		case BT_EFUSMX93:
		case BT_OSMSFMX93:
			imx_iomux_v3_setup_multiple_pads(lpuart1_pads, ARRAY_SIZE(lpuart1_pads));
			init_uart_clk(LPUART1_CLK_ROOT);
			break;
		default:
			return -EINVAL;
			break;
	}

	fdtdec_resetup(&rescan);

	if(rescan) {
		dm_uninit();
		dm_init_and_scan(!CONFIG_IS_ENABLED(OF_PLATDATA));
	}

	return 0;
}

#if CONFIG_IS_ENABLED(MULTI_DTB_FIT)
int board_fit_config_name_match(const char *name)
{
#if defined(CONFIG_TARGET_FSIMX93)
	CHECK_BOARD_TYPE_AND_NAME("picocoremx93", BT_PICOCOREMX93, name);
	CHECK_BOARD_TYPE_AND_NAME("fs-osm-sf-mx93-adp-osm-bb", BT_OSMSFMX93, name);
	CHECK_BOARD_TYPE_AND_NAME("efusmx93", BT_EFUSMX93, name);
#elif defined(CONFIG_TARGET_FSIMX91)
	CHECK_BOARD_TYPE_AND_NAME("fs-osm-sf-mx91-adp-osm-bb", BT_OSMSFMX91, name);
	CHECK_BOARD_TYPE_AND_NAME("efusmx91", BT_EFUSMX91, name);
#endif

	return -EINVAL;
}
#endif

#if CONFIG_IS_ENABLED(DM_PMIC_PCA9450)
int power_init_board(void)
{
	struct udevice *dev;
	int ret;
	unsigned int val = 0, buck_val;

	ret = pmic_get("pmic@25", &dev);
	if (ret != 0) {
		puts("ERROR: Get PMIC PCA9451A failed!\n");
		return ret;
	}

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(dev, PCA9450_BUCK123_DVS, 0x29);

	/* enable DVS control through PMIC_STBY_REQ */
	pmic_reg_write(dev, PCA9450_BUCK1CTRL, 0x59);

	ret = pmic_reg_read(dev, PCA9450_PWR_CTRL);
	if (ret < 0)
		return ret;
	else
		val = ret;

	if (is_voltage_mode(VOLT_LOW_DRIVE)) {
		buck_val = 0x0c; /* 0.8v for Low drive mode */
		printf("PMIC: Low Drive Voltage Mode\n");
	} else if (is_voltage_mode(VOLT_NOMINAL_DRIVE)) {
		buck_val = 0x10; /* 0.85v for Nominal drive mode */
		printf("PMIC: Nominal Voltage Mode\n");
	} else {
		buck_val = 0x14; /* 0.9v for Over drive mode */
		printf("PMIC: Over Drive Voltage Mode\n");
	}

	if (val & PCA9450_REG_PWRCTRL_TOFF_DEB) {
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS0, buck_val);
		pmic_reg_write(dev, PCA9450_BUCK3OUT_DVS0, buck_val);
	} else {
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS0, buck_val + 0x4);
		pmic_reg_write(dev, PCA9450_BUCK3OUT_DVS0, buck_val + 0x4);
	}

	/* set standby voltage to 0.65v */
	if (val & PCA9450_REG_PWRCTRL_TOFF_DEB)
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS1, 0x0);
	else
		pmic_reg_write(dev, PCA9450_BUCK1OUT_DVS1, 0x4);

	/*
	 * The Following Configuration is for FS-OSM-SF-MX93 and
	 * PicoCoreMX93
	 * 
	 * TODO: READ Board variant and do specific configuration
	 */

	/* Set VDDQ to 1.1V from buck6 */
	pmic_reg_write(dev, PCA9450_BUCK6OUT, 0x14);

	/* enable buck6 during PMIC_ON_REQ = H */
	pmic_reg_write(dev, PCA9450_BUCK6CTRL, 0x09);

	/* disable buck2 */
	/* TODO: read board variant in case the buck is used for lpddr4x */
	pmic_reg_write(dev, PCA9450_BUCK2CTRL, 0x08);

	/* I2C_LT_EN*/
	pmic_reg_write(dev, 0xa, 0x3);
	return 0;
}
#endif

void board_init_f(ulong dummy)
{
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
	board_early_init_f();

	regulators_enable_boot_on(false);
	
	preloader_console_init();

	print_bootstage();

	print_devinfo();

	ret = imx9_probe_mu();
	if (ret) {
		printf("Fail to init ELE API\n");
	} else {
		printf("SOC: 0x%x\n", gd->arch.soc_rev);
		printf("LC: 0x%x\n", gd->arch.lifecycle);
	}

	clock_init_late();

	power_init_board();

	if (!is_voltage_mode(VOLT_LOW_DRIVE))
		set_arm_core_max_clk();

	/* Init power of mix */
	soc_power_init();
	
	/*load F&S NBOOT-Images*/
	fs_cntr_init(true);

	/* Setup TRDC for DDR access */
	trdc_init();
	
	/* DDR initialization */
	spl_dram_init();

#if defined(CONFIG_TARGET_FSIMX93)
	/* Put M33 into CPUWAIT for following kick */
	ret = m33_prepare();
	if (!ret)
		printf("M33 prepare ok\n");
#endif

	board_init_r(NULL, 0);
}
