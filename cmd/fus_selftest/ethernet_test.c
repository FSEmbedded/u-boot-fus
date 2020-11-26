/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <net.h>
#include <phy.h>
#include <miiphy.h>
#include "ethernet_test.h"
#include <asm/gpio.h>
#include "serial_test.h" // mute_debug_port()
#include "selftest.h"
// main functions


static void fus_reset_phy(void){
	struct udevice *dev;
	struct uclass *uc;
	struct gpio_desc *desc;
	int gpio_num;
	u32 reset_duration, post_delay;

	if (uclass_get(UCLASS_ETH, &uc))
		return;

	desc = calloc(1, sizeof(struct gpio_desc));

	uclass_foreach_dev(dev, uc) {
		gpio_request_by_name(dev, "phy-reset-gpios",0,desc,0);
		gpio_num = gpio_get_number(desc);

		dev_read_u32(dev, "phy-reset-duration", &reset_duration);
		dev_read_u32(dev, "phy-reset-post-delay", &post_delay);

		/* Reserve GPIOs for reset of PHYs */
		gpio_request(gpio_num, "phy_rst");
		/* Start the reset (active low) */
		gpio_direction_output(gpio_num, 0);
		udelay(reset_duration);
		/* End the reset */
		gpio_direction_output(gpio_num, 1);
		/* The board has a long delay for this reset to become stable */
		mdelay(post_delay);
	}

}

int test_ethernet(char *szStrBuffer)
{
	struct udevice *dev;
	int port;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	port = 0;

	while (uclass_get_device(UCLASS_ETH,port,&dev) == 0) {
		struct phy_device *phy = mdio_phydev_for_ethname(dev->name);
		char * ethaddr = "";
		int ret = 0, target_speed = 0;

		if (port == 0)
			ethaddr = env_get("ethaddr");
		else
			ethaddr = env_get("eth1addr");

		printf("ETHERNET%d: ", port);

		printf("MAC: %s\n", ethaddr);

		port++;

		printf("  internal loopback...");

		phy = mdio_phydev_for_ethname(dev->name);

		fus_reset_phy();

		mute_debug_port(1);

		ret = phy_config(phy);
		ret |= phy_startup(phy);

		mute_debug_port(0);

		if (ret)
		{
	        sprintf(szStrBuffer, "Link up timeout!");
	        test_OkOrFail(-1, 1, szStrBuffer);
			continue;
		}

		test_OkOrFail(0, 1, szStrBuffer);

		printf("  external loopback...");

		if (phy->drv->features & PHY_1000BT_FEATURES)
			target_speed = 1000;
		else if (phy->drv->features & PHY_100BT_FEATURES)
			target_speed = 100;
		else if (phy->drv->features & PHY_10BT_FEATURES)
			target_speed = 10;
		else
		{
	        sprintf(szStrBuffer, "Unable to get Driver Speed");
	        test_OkOrFail(-1, 1, szStrBuffer);
			continue;
		}

		sprintf(szStrBuffer, "Connected with %dMbps", phy->speed);
		if (phy->speed != target_speed)
		{
	        test_OkOrFail(-1, 1, szStrBuffer);
			continue;
		}
		test_OkOrFail(0, 1, szStrBuffer);
	}

	return 0;
}
