/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../ethernet_test.h"
#include "ethernet_imx8x.h"

// helper functions

void reset_phy()
{
	/* Reserve GPIOs for reset of PHYs */
	gpio_request(PHY1_RST, "phy1_rst");
	gpio_request(PHY2_RST, "phy2_rst");
	/* Start the reset (active low) */
	gpio_direction_output(PHY1_RST, 0);
	gpio_direction_output(PHY2_RST, 0);
	udelay(50);
	/* End the reset */
	gpio_direction_output(PHY1_RST, 1);
	gpio_direction_output(PHY2_RST, 1);

	/* The board has a long delay for this reset to become stable */
	mdelay(200);
}
