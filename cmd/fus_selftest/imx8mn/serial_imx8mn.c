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
#ifndef CONFIG_DM_SERIAL
#include "../serial_test.h" // struct stdio_dev
#else
#include "../serial_test_dm.h" // dm serial
#endif
#include <dm.h>
#include <dm/platform_data/serial_mxc.h>

#define UART_LOOP		(1 <<  12)

//---Address-Operations---//

#define U32(X)      ((uint32_t) (X))
#define READ4(A) *((volatile uint32_t*)(A))
#define DATA4(A, V) *((volatile uint32_t*)(A)) = U32(V)
#define SET_BIT4(A, V) *((volatile uint32_t*)(A)) |= U32(V)
#define CLR_BIT4(A, V) *((volatile uint32_t*)(A)) &= ~(U32(V))

int init_uart()
{
	/* Set RDC for UART4 to r/w acces for both cpus */
	DATA4(RDC_BASE_ADDR + 0x518, 0xff);

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

void set_loopback(void *dev, int on)
{
	struct udevice *pdev = (struct udevice *) dev;
	struct mxc_serial_platdata *plat;
	struct mxc_uart * base;

	plat = pdev->platdata;
	base = plat->reg;

	/* Set internal loopback mode */
	if( on )
	{	writel(base->ts | UART_LOOP, &base->ts);
		pinctrl_select_state(pdev,"mute");

	}
	else
	{
		writel(base->ts & ~UART_LOOP, &base->ts);
		pinctrl_select_state(pdev,"default");

	}
}


