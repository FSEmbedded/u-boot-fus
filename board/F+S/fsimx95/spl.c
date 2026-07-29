// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025-2026 NXP
 */

#include <hang.h>
#include <init.h>
#include <spl.h>
#include <dm.h>
#include <asm/global_data.h>
#include <asm/sections.h>
#include <asm/arch/clock.h>
#include <asm/arch/mu.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/bbsm.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/mach-imx/qb.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#ifdef CONFIG_SCMI_FIRMWARE
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <scmi_nxp_protocols.h>
//#include <dt-bindings/clock/fsl,imx95-clock.h>
//#include <dt-bindings/power/fsl,imx95-power.h>
#include "scmi_ddr_init.h"
#endif
#include "../common/fs_cntr_common.h"

DECLARE_GLOBAL_DATA_PTR;

static struct udevice *scmi_dev __maybe_unused;

static struct dram_timing_info *_dram_timing;

int fs_board_init_dram_data(unsigned long *ptr){
	if(!ptr)
		return -ENODATA;

	_dram_timing = (struct dram_timing_info *)ptr;

	return 0;
}

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
	case SD1_BOOT:
	case MMC1_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC2;
	case USB_BOOT:
	case USB2_BOOT:
		return BOOT_DEVICE_BOARD;
	case QSPI_BOOT:
		return BOOT_DEVICE_SPI;
	default:
		return BOOT_DEVICE_NONE;
	}
}

void spl_board_init(void)
{
	int ret;
	u32 bd;

	puts("Normal Boot\n");

	ret = ele_start_rng();
	if (ret)
		printf("Fail to start RNG: %d\n", ret);

#ifdef CONFIG_SPL_IMX_BBSM
	ret = bbsm_tamper_detect_enable();
	if (ret)
		printf("Failed to enable BBSM Tamper Detection: %d\n", ret);
#endif

	bd = spl_boot_device();
	if (bd == BOOT_DEVICE_BOARD) { /* USB */
		ret = power_on_hsio();
		if (ret)
			printf("power on hsio is failed\n");
	}

	if (IS_ENABLED(CONFIG_SPL_IMX_QB))
		spl_qb_save();
}

static int scmi_ddr_init(void)
{
	int ret, numVal;
	msg_rmisc32_t msg_in;
	scmi_msg_status_t msg_out;
	struct udevice *dev;
	struct scmi_msg msg = SCMI_MSG_IN( \
		SCMI_PROTOCOL_ID_IMX_MISC, \
		SCMI_MISC_CONTROL_EXT_SET, \
		msg_in, msg_out);

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	numVal = sizeof(struct ddr_init_params) / sizeof(uint32_t);

	msg_in.ctrlId = 0x8007;
	msg_in.addr = 0x0;
	msg_in.len = numVal;
	msg_in.numVal = numVal;
	msg_in.params.magic = DDR_PARAM_MAGIC;
	msg_in.params.version = DDR_PARAM_VERSION;
	msg_in.params.fw_addr = CFG_SPL_DRAM_FW_EXE_ADDR;
	msg_in.params.fw_size = 0;
	msg_in.params.timings_addr = CFG_SPL_DRAM_TIMING_ADDR;
	msg_in.params.timings_size = 0;
	msg_in.params.options = 0;
	msg_in.params.reserved = 0;

	ret = devm_scmi_process_msg(dev, &msg);

	printf("DDR_INIT(SCMI) ret = %d\n", ret);
	printf("DDR_INIT(SCMI) status = %d\n", msg_out.status);

	if (!ret)
		ret = msg_out.status;

	return ret;
}

static void flexspi_nor_reset(void)
{
	int ret;
	struct gpio_desc desc;

	/* 15x15 EVK use M.2 QSPI card, not support booting */
	if (IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK))
		return;

	ret = dm_gpio_lookup_name("GPIO5_11", &desc);
	if (ret) {
		printf("%s lookup GPIO5_11 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&desc, "XSPI_RST_B");
	if (ret) {
		printf("%s request XSPI_RST_B failed ret = %d\n", __func__, ret);
		return;
	}

	/* assert the XSPI_RST_B */
	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE | GPIOD_ACTIVE_LOW);
	udelay(200); /* 50 ns at least, so use 200ns */
	dm_gpio_set_value(&desc, 0); /* deassert the XSPI_RST_B */
}

void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

#ifdef CONFIG_SPL_RECOVER_DATA_SECTION
	if (IS_ENABLED(CONFIG_SPL_BUILD))
		spl_save_restore_data();
#endif

	timer_init();

	/* Need dm_init() to run before any SCMI calls can be made. */
	spl_early_init();

	/* Need enable SCMI drivers and ELE driver before enabling console */
	ret = imx9_probe_mu();
	if (ret)
		hang(); /* if MU not probed, nothing can output, just hang here */

	arch_cpu_init();

	preloader_console_init();

	printf("SOC: 0x%x\n", gd->arch.soc_rev);
	printf("LC: 0x%x\n", gd->arch.lifecycle);

	get_reset_reason(true, false);

	disable_smmuv3();

	flexspi_nor_reset();

	/*load F&S NBOOT-Images*/
	fs_cntr_init(true);

	/* DDR initialization */
	ret = scmi_ddr_init();

	board_init_r(NULL, 0);
}

#ifndef CONFIG_USB_PORT_AUTO
int board_usb_gadget_port_auto(void)
{
    enum boot_device bt_dev = get_boot_device();
	int usb_boot_index = 0;

	if (bt_dev == USB2_BOOT)
		usb_boot_index = 1;

	return usb_boot_index;
}
#endif

#ifdef CONFIG_ANDROID_SUPPORT
int board_get_emmc_id(void) {
	return 0;
}
#endif
