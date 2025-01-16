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
#include "common/ksz9893r.h" // ksz9893r_switch_port()
#include "../../board/F+S/common/fs_board_common.h"/* fs_board_*() */

#define BT_PICOCOREMX8MM 	0
#define BT_PICOCOREMX8MX	1

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

/*
commands:
	- DHCP
	- TFTP
	- PING
*/

int test_ethernet(char *szStrBuffer/*, char cmd*/)
{
	struct udevice *dev;
	const void *fdt = gd->fdt_blob;
	unsigned int board_type = fs_board_get_type();
	int node;
	int port;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	port = 0;

	while (uclass_get_device(UCLASS_ETH,port,&dev) == 0) {
		char * ethaddr = "";
		int ret = 0, target_speed = 0;

		node = dev_of_offset(dev);

		node = fdt_subnode_offset(fdt, node, "fixed-link");

		if (!strcmp("fixed-link",fdt_get_name(fdt, node, NULL))) {
			int i2c_bus = 4;
			char ip[22];
			char ip2[22];
			bool port_failed = 0;
			bool port2_failed = 0;

			if (port == 0)
				ethaddr = env_get("ethaddr");
			else
				ethaddr = env_get("eth1addr");

			printf("ETHERNET%d: ", port);

			printf("MAC: %s\n", ethaddr);

			port++;

			printf("  internal loopback...");

			sprintf(szStrBuffer, "Fixed link");
			test_OkOrFail(1, 1, szStrBuffer);

			mute_debug_port(1);

			env_set("autoload", "no");
			env_set("bootpretryperiod", "10000");

			if (board_type == BT_PICOCOREMX8MX) {

				ksz9893r_power_port(i2c_bus,0x1);
				net_loop(DHCP);
				if(net_ip.s_addr)
					ip_to_string(net_ip, ip);
				else
					port_failed = 1;

				ksz9893r_power_port(i2c_bus,0x2);
				net_loop(DHCP);
				if(net_ip.s_addr)
					ip_to_string(net_ip, ip2);
				else
					port2_failed = 1;
			}
			else {
				net_loop(DHCP);
				if(net_ip.s_addr)
					ip_to_string(net_ip, ip);
				else
					port_failed = 1;
			}

//			net_loop(PING/TFTP);

			mute_debug_port(0);

			if (board_type == BT_PICOCOREMX8MX) {

				/* ETH_A */
				printf("  external loopback...");
				if (!port_failed) {
					sprintf(szStrBuffer, "ETH_A IP: %s", ip);
					test_OkOrFail(0, 1, szStrBuffer);
				}
				else {
					sprintf(szStrBuffer, "ETH_A failed with timeout");
					test_OkOrFail(-1, 1, szStrBuffer);
				}

				/* ETH_B */
				printf("  external loopback...");
				if (!port2_failed) {
					sprintf(szStrBuffer, "ETH_B IP: %s", ip2);
					test_OkOrFail(0, 1, szStrBuffer);
				}
				else {
					sprintf(szStrBuffer, "ETH_B failed with timeout");
					test_OkOrFail(-1, 1, szStrBuffer);
				}
			}
			else {

				printf("  external loopback...");
				if (!port_failed) {
					sprintf(szStrBuffer, "IP: %s", ip);
					test_OkOrFail(0, 1, szStrBuffer);
				}
				else {
					sprintf(szStrBuffer, "Failed with timeout");
					test_OkOrFail(-1, 1, szStrBuffer);
				}
			}
		}
		else {
			struct phy_device *phy = mdio_phydev_for_ethname(dev->name);


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

			mute_debug_port(0);

			if (ret)
			{
				/* PHY doesn't communicate or is not present */
			    sprintf(szStrBuffer, "PHY timeout!");
			    test_OkOrFail(-1, 1, szStrBuffer);
				continue;
			}

			if (phy->drv->uid == 0xffffffff)
			{
				/* No PHY found, generic driver is loaded */
			    sprintf(szStrBuffer, "Can't match UID and PHY driver!");
			    test_OkOrFail(-1, 1, szStrBuffer);
				continue;
			}

			test_OkOrFail(0, 1, szStrBuffer);

			printf("  external loopback...");

			mute_debug_port(1);

			ret = phy_startup(phy);

			mute_debug_port(0);

			if (ret)
			{
			    sprintf(szStrBuffer, "Link up timeout!");
			    test_OkOrFail(-1, 1, szStrBuffer);
				continue;
			}

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
	}

	return 0;
}
