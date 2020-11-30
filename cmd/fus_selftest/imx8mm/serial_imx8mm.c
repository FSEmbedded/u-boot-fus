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
#include <dm/pinctrl.h>
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
	struct udevice *dev;
	struct mxc_serial_platdata *plat;
	struct mxc_uart * base;

	uclass_get_device_by_seq(UCLASS_SERIAL,port,&dev);
	plat = dev->platdata;
	base = plat->reg;
	/* Disable transmitter */
	//CLR_BIT4(UART_UCR2(uart_base),UART_TXEN);

	/* Disable receiver */
	//CLR_BIT4(UART_UCR2(uart_base),UART_RXEN);

	/* Set internal loopback mode */
	if( on )
	{	writel(base->ts | UART_LOOP, &base->ts);
		pinctrl_select_state(dev,"mute");

	}
	else
	{
		writel(base->ts & ~UART_LOOP, &base->ts);
		pinctrl_select_state(dev,"default");

	}
	/* Enable transmitter */
	//SET_BIT4(UART_UCR2(uart_base),UART_TXEN);
	/* Enable receiver */
	//SET_BIT4(UART_UCR2(uart_base),UART_RXEN);
}


