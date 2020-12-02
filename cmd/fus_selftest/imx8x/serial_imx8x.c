/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/mach-imx/sci/sci.h> // SCU Functions
#include <asm/arch/lpcg.h> // Clocks
#include <asm/arch/iomux.h> // IOMUX
#include <asm/arch/imx8-pins.h> // Pins
#include <asm/arch/imx-regs.h> // LPUART_BASE_ADDR
#include <asm/io.h>			/* __raw_readl(), __raw_writel() */
#include <common.h> // DECLARE_GLOBAL_DATA_PTR
#include <dm/pinctrl.h>
#ifndef CONFIG_DM_SERIAL
#include "../serial_test.h" // struct stdio_dev
#else
#include "../serial_test_dm.h" // dm serial
#endif
#include <dm.h>

//---LPUART Register---//

struct lpuart {
	u32 verid;
	u32 param;
	u32 global;
	u32 pincfg;
	u32 baud;
	u32 stat;
	u32 ctrl;
	u32 data;
	u32 match;
	u32 modir;
	u32 fifo;
	u32 water;
};

//---LPUART Control-Register---//

#define LPUART_CTRL_LOOPS	(1 <<  7)

enum lpuart_devtype {
	DEV_VF610 = 1,
	DEV_LS1021A,
	DEV_MX7ULP,
	DEV_IMX8
};

struct lpuart_serial_platdata {
	void *reg;
	enum lpuart_devtype devtype;
	ulong flags;
};

// helper functions

int init_uart()
{
	struct uclass *uc;
	struct udevice *dev;

	// Clocks aren't handled by the device tree, so we need to do it here.

	sc_ipc_t ipcHndl = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	sc_pm_clock_rate_t rate = 80000000;

	/* Power up UART */
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_0, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_1, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_2, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_3, SC_PM_PW_MODE_ON);

	/* Set UART clock rate */
	sc_pm_set_clock_rate(ipcHndl, SC_R_UART_0, 2, &rate);
	sc_pm_set_clock_rate(ipcHndl, SC_R_UART_1, 2, &rate);
	sc_pm_set_clock_rate(ipcHndl, SC_R_UART_2, 2, &rate);
	sc_pm_set_clock_rate(ipcHndl, SC_R_UART_3, 2, &rate);

	/* Enable UART clock root */
	sc_pm_clock_enable(ipcHndl, SC_R_UART_0, 2, true, false);
	sc_pm_clock_enable(ipcHndl, SC_R_UART_1, 2, true, false);
	sc_pm_clock_enable(ipcHndl, SC_R_UART_2, 2, true, false);
	sc_pm_clock_enable(ipcHndl, SC_R_UART_3, 2, true, false);

	/* Open UART clock gates */
	LPCG_AllClockOn(LPUART_0_LPCG);
	LPCG_AllClockOn(LPUART_1_LPCG);
	LPCG_AllClockOn(LPUART_2_LPCG);
	LPCG_AllClockOn(LPUART_3_LPCG);

	if (uclass_get(UCLASS_SERIAL, &uc))
		return -1;

	uclass_foreach_dev(dev, uc)
		pinctrl_select_state(dev,"default");

	return 0;
}

struct udevice * get_debug_dev()
{
	struct uclass *uc;
	struct udevice *dev;
	const void *fdt = gd->fdt_blob;
	int node;

	if (uclass_get(UCLASS_SERIAL, &uc))
		return NULL;

	uclass_foreach_dev(dev, uc) {
		node = dev_of_offset(dev);
		if (fdt_get_property(fdt, node, "debug-port", NULL))
			return dev;
	}

	return NULL;
}

void set_loopback(void *dev, int on)
{
	struct udevice *pdev = (struct udevice *) dev;
	struct lpuart_serial_platdata *plat;
	struct lpuart * base;

	plat = pdev->platdata;
	base = (struct lpuart *) plat->reg;

	if( on )
	{
		writel(base->ctrl | LPUART_CTRL_LOOPS, &base->ctrl);
		pinctrl_select_state(pdev,"mute");

	}
	else
	{
		writel(base->ctrl & ~LPUART_CTRL_LOOPS, &base->ctrl);
		pinctrl_select_state(pdev,"default");
	}
}
