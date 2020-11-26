/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../serial_test.h"
#include "serial_imx8x.h"

// helper functions

int init_uart()
{
	// Clocks aren't handled by the device tree, so we need to do it here.

	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	sc_pm_clock_rate_t rate = 80000000;

	/* Power up UART */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_0, SC_PM_PW_MODE_ON);
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_1, SC_PM_PW_MODE_ON);
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_2, SC_PM_PW_MODE_ON);
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_3, SC_PM_PW_MODE_ON);

	/* Set UART clock rate */
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_0, 2, &rate);
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_1, 2, &rate);
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_2, 2, &rate);
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_3, 2, &rate);

	/* Enable UART clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_0, 2, true, false);
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_1, 2, true, false);
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_2, 2, true, false);
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_3, 2, true, false);

	/* Open UART clock gates */
	LPCG_AllClockOn(LPUART_0_LPCG);
	LPCG_AllClockOn(LPUART_1_LPCG);
	LPCG_AllClockOn(LPUART_2_LPCG);
	LPCG_AllClockOn(LPUART_3_LPCG);

	for (int port=0; serial_ports[port]!=NULL; port++)
	{
		sc_pad_set_mux(ipcHndl, uart_rx_pads[port].pad, uart_rx_pads[port].mux, SC_PAD_CONFIG_OUT_IN, SC_PAD_ISO_OFF);
		sc_pad_set_mux(ipcHndl, uart_tx_pads[port].pad, uart_tx_pads[port].mux, SC_PAD_CONFIG_OUT_IN, SC_PAD_ISO_OFF);
	}

	return 0;
}
char * get_serial_ports(int port)
{
	return  serial_ports[port];

}
int get_debug_port()
{

	return DEBUG_PORT;
}

int pending(void *dev, int input)
{
	struct stdio_dev *pdev = (struct stdio_dev *) dev;
	u32 stat;

	if (input) {
		return pdev->tstc(pdev);
	} else {
		stat = READ4(pdev->priv + LPUART_STAT_OFFSET);
		return stat & LPUART_STAT_TDRE ? 0 : 1;
	}
}

void set_loopback(int port, int on)
{
	sc_ipc_t ipcHndl = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* Set internal loopback mode */
	if( on )
	{
		SET_BIT4(LPUART_CTRL(port),LPUART_CTRL_LOOPS);
		sc_pad_set_mux(ipcHndl, uart_tx_gpio_pads[port].pad, uart_tx_gpio_pads[port].mux, SC_PAD_CONFIG_OUT_IN, SC_PAD_ISO_OFF);
	}
	else
	{
		CLR_BIT4(LPUART_CTRL(port),LPUART_CTRL_LOOPS);
		sc_pad_set_mux(ipcHndl, uart_tx_pads[port].pad, uart_tx_pads[port].mux, SC_PAD_CONFIG_OUT_IN, SC_PAD_ISO_OFF);
	}
}
