/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <asm/arch/imx8mm_pins.h> // Pins
#include <asm/arch/imx-regs-imx8mm.h> // LPUART_BASE_ADDR
#include <asm/arch/clock.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/io.h>			/* __raw_readl(), __raw_writel() */
#include <serial.h>
#include "../../../board/F+S/common/fs_board_common.h"	/* fs_board_*() */
#include "../serial_test.h" // struct stdio_dev
#include <dm.h>
#include <dm/platform_data/serial_mxc.h>

#define DEBUG_PORT 0

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define GPIO_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1 | PAD_CTL_PUE | PAD_CTL_PE)

#define UART_UTS_OFFSET 		0xB4
#define UART_UCR2_OFFSET 		0x84

#define UART_LOOP		(1 <<  12)
#define UART_RXEN		(1 <<  1)
#define UART_TXEN		(1 <<  2)

#define UART_UTS(base)		base + UART_UTS_OFFSET
#define UART_UCR2(base)		base + UART_UCR2_OFFSET

//---Address-Operations---//

#define U32(X)      ((uint32_t) (X))
#define READ4(A) *((volatile uint32_t*)(A))
#define DATA4(A, V) *((volatile uint32_t*)(A)) = U32(V)
#define SET_BIT4(A, V) *((volatile uint32_t*)(A)) |= U32(V)
#define CLR_BIT4(A, V) *((volatile uint32_t*)(A)) &= ~(U32(V))


static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_UART1_RXD_UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART1_TXD_UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART2_RXD_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART2_TXD_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART3_RXD_UART3_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART3_TXD_UART3_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART4_RXD_UART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART4_TXD_UART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart_gio_pads[] = {
	IMX8MM_PAD_UART1_RXD_GPIO5_IO22 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART1_TXD_GPIO5_IO23 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART2_RXD_GPIO5_IO24 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART2_TXD_GPIO5_IO25 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART3_RXD_GPIO5_IO26 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART3_TXD_GPIO5_IO27 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART4_RXD_GPIO5_IO28 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	IMX8MM_PAD_UART4_TXD_GPIO5_IO29 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

// Must be in right order !!!
static char * serial_ports[] = {
		"uart1",
		"uart2",
		"uart3",
		"uart4",
		NULL
};

int init_uart()
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	/* Set RDC for UART4 to r/w acces for both cpus */
	DATA4(RDC_BASE_ADDR + 0x518, 0xff);

	return 0;
}
char * get_serial_ports(int port)
{
	return  serial_ports[port];

}
int get_debug_port()
{
#if 0
	int port = 6;
	struct serial_device *sdev;
	struct fs_nboot_args *pargs = fs_board_get_nboot_args();

	do {
		sdev = get_serial_device(--port);
		if (sdev && sdev->dev.priv == (void *)(ulong)pargs->dwDbgSerPortPA)
			return port;
	} while (port);
	return -1;
#endif
return 0;
}

#define UTS_TXEMPTY	(1<<6)	/* TxFIFO empty */
#define UTS_RXEMPTY	(1<<5)	/* RxFIFO empty */

struct mxc_uart {
	u32 rxd;
	u32 spare0[15];

	u32 txd;
	u32 spare1[15];

	u32 cr1;
	u32 cr2;
	u32 cr3;
	u32 cr4;

	u32 fcr;
	u32 sr1;
	u32 sr2;
	u32 esc;

	u32 tim;
	u32 bir;
	u32 bmr;
	u32 brc;

	u32 onems;
	u32 ts;
};

int pending(void *dev, int input)
{
	struct udevice *pdev = (struct udevice *) dev;
	struct mxc_serial_platdata *plat = pdev->platdata;
	struct mxc_uart *const base = plat->reg;

	if (input) {
		/* If receive fifo is empty, return false */
		if (readl(&base->ts) & UTS_RXEMPTY)
			return 0;
		else
			return 1;
	}
	else {
		/* If transmit fifo is empty, return false */
		if (readl(&base->ts) & UTS_TXEMPTY)
			return 0;
		else
			return 1;
	}
}

void set_loopback(int port, int on)
{
	uint64_t uart_base = 0;

	if (port == 0)
		uart_base = UART1_BASE_ADDR;
	else if (port == 1)
		uart_base = UART2_BASE_ADDR;
	else if  (port == 2)
		uart_base = UART3_BASE_ADDR;
	else if  (port == 3)
		uart_base = UART4_BASE_ADDR;

	/* Disable transmitter */
	//CLR_BIT4(UART_UCR2(uart_base),UART_TXEN);

	/* Disable receiver */
	//CLR_BIT4(UART_UCR2(uart_base),UART_RXEN);

	/* Set internal loopback mode */
	if( on )
	{
		SET_BIT4(UART_UTS(uart_base),UART_LOOP);
		if  (port*2 < ARRAY_SIZE(uart_gio_pads))
			imx_iomux_v3_setup_multiple_pads(uart_gio_pads + (port*2), 2);

	}
	else
	{
		CLR_BIT4(UART_UTS(uart_base),UART_LOOP);
		if  (port*2 < ARRAY_SIZE(uart_pads))
			imx_iomux_v3_setup_multiple_pads(uart_pads + (port*2), 2);

	}
	/* Enable transmitter */
	//SET_BIT4(UART_UCR2(uart_base),UART_TXEN);
	/* Enable receiver */
	//SET_BIT4(UART_UCR2(uart_base),UART_RXEN);
}


