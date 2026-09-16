// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright 2026 F&S Elektronik Systeme GmbH
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

#include <hang.h>
#include <init.h>
#include <spl.h>
#include <asm/global_data.h>
#include <asm/sections.h>
#include <asm/arch/mu.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/mach-imx/qb.h>
#include <dm.h>
#include <asm/gpio.h>
#include <dm/root.h>
#ifdef CONFIG_SCMI_FIRMWARE
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <scmi_nxp_protocols.h>
#include "scmi_ddr_init.h"
#endif
#include "../common/fs_cntr_common.h"
#include "../common/fs_image_common.h"
#include "fsimx95.h"

DECLARE_GLOBAL_DATA_PTR;

__maybe_unused static struct udevice *scmi_dev;

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

	msg_in.ctrlId = SCMI_BRD_FLAG | SCMI_FUS_MISC_DDR_INIT;
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

	if (ret || msg_out.status) {
		printf("DDR_INIT(SCMI) ret = %d\n", ret);
		printf("DDR_INIT(SCMI) status = %d\n", msg_out.status);
	}

	if (!ret)
		ret = msg_out.status;

	return ret;
}

void spl_dram_init(void)
{
	struct dram_timing_info *dtiming = _dram_timing;

	printf("DDR: %uMTS\n", dtiming->fsp_table[0]);
	scmi_ddr_init();
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

	SET_BOARD_TYPE("SM95S", BT_FSSM95S, board_id, len);
	SET_BOARD_TYPE("PC95S", BT_PICOCOREMX95, board_id, len);

	return -EINVAL;
}

int board_early_init_f(void)
{
	int rescan = 0;

	set_gd_board_type();

	switch(gd->board_type) {
		case BT_FSSM95S:
			init_uart_clk(0);
			break;
		case BT_PICOCOREMX95:
			init_uart_clk(6);
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
	CHECK_BOARD_TYPE_AND_NAME("fssm95s", BT_FSSM95S, name);
	CHECK_BOARD_TYPE_AND_NAME("picocoremx95", BT_PICOCOREMX95, name);

	return -EINVAL;
}
#endif

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

	/* Setup default Devicetree */
	spl_early_init();

	/* Need enable SCMI drivers and ELE driver before arch init */
	ret = imx9_probe_mu();
	if (ret)
		hang(); /* if MU not probed, nothing can output, just hang here */

	arch_cpu_init();

	/* Load Board ID to know the Board in early state*/
	fs_cntr_load_board_id();

	/* Setup Multiple Devicetree */
	board_early_init_f();

	preloader_console_init();

	debug("SOC: 0x%x\n", gd->arch.soc_rev);
	debug("LC: 0x%x\n", gd->arch.lifecycle);

	get_reset_reason(true, false);

	disable_smmuv3();

	/*load F&S NBOOT-Images*/
	fs_cntr_init(true);

	/* DDR initialization */
	spl_dram_init();

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

