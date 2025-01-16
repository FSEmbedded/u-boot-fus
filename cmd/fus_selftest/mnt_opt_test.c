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
#include <dm/pinctrl.h>
#include <asm/gpio.h>
#include <malloc.h>
#include "mnt_opt_test.h"
#include "selftest.h"

#include "serial_test.h" // mute_debug_port()

#define OUTPUT 0
#define INPUT 1

static int size;

// helper functions

static int get_gpio_list(struct udevice *dev, struct gpio_desc **in_gpios)
{
	int ret;

	size = 0;

	/* Get GPIO List */
	size = gpio_get_list_count(dev, "in-gpios-mnt");
	printf("SIZE = %d\n",size);
	*in_gpios = calloc(size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "in-gpios-mnt", *in_gpios, size, 0);
	printf("gpio_request_list_by_name = %d\n",ret);
	if (ret < 0)
		return -1;

	return 0;
}

static int init_gpios(struct udevice *dev, struct gpio_desc *gpios, char *label_name)
{
	const char * label;

	for (int i = 0; i < size; i++) {
		dev_read_string_index(dev, label_name, i, &label);
		dm_gpio_free(dev, (gpios+i));
		dm_gpio_request((gpios+i), label);
		dm_gpio_set_dir_flags((gpios+i), GPIOD_IS_IN);
	}
	/* Delay for setup */
	mdelay(10);
	return 0;
}

static void free_gpios(struct udevice *dev, struct gpio_desc *gpios)
{
	for (int i = 0; i < size; i++)
		dm_gpio_free(dev, (gpios+i));
}

/*	DTS:
	pinctrl-names = "mntgpio";
	pinctrl-0 = <&pinctrl_mnt_gpio>;
	in-pins-mnt = "RS485/232";
	in-gpios-mnt = <&gpio1 7 0>;
	in-high-mnt = "RS485";
	in-low-mnt = "RS232";
*/

// main functions

int test_mnt_opt(char *szStrBuffer)
{
	struct udevice *dev;
	struct uclass *uc;
	int gpio_exists, failed, first_pins, i, ret;
	u32 req_val[128];
	struct gpio_desc *in_gpios;
	const char *in_label, *in_label_val;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	mute_debug_port(1);

	gpio_exists = 0;
	failed = 0;
	first_pins = 1;

	if (uclass_get(UCLASS_GPIO, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {

		/* Get GPIO-List */
		if (get_gpio_list(dev, &in_gpios))
			continue;

		/* Initialize and request the GPIOs */
		if (init_gpios(dev, in_gpios,"in-pins-mnt"))
			continue;

		/* Mark that at least one device of the driver has a gpio-test-config */
		if (size > 0) {
			gpio_exists = 1;

			/* Set IOMUX to mntgpio */
			pinctrl_select_state(dev,"mntgpio");
		}

		ret = dev_read_u32_array(dev, "req-val-mnt", req_val, size);

		if(!ret) {
			/* req-val-mnt set in devicetree, compare with gpio inputs */
			bool failed_tmp;

			/* Handle every GPIO */
			for (i = 0; i < size; i++) {

				/* Check for failed */
				failed_tmp = (dm_gpio_get_value(in_gpios+i) != req_val[i]);

				/* Get description of pin and value */
				dev_read_string_index(dev, "in-pins-mnt", i, &in_label);
				if (req_val[i])
					dev_read_string_index(dev, "in-high-mnt", i, &in_label_val);
				else
					dev_read_string_index(dev, "in-low-mnt", i, &in_label_val);

				/* Print results to szStrBuffer */
				if (first_pins)
					first_pins = 0;
				else
					sprintf(szStrBuffer + strlen(szStrBuffer),", ");

				sprintf(szStrBuffer + strlen(szStrBuffer),"%s",in_label);

				if (failed_tmp) {
					failed = 1;
					sprintf(szStrBuffer + strlen(szStrBuffer),"!=");
				}
				else
					sprintf(szStrBuffer + strlen(szStrBuffer),"=");

				sprintf(szStrBuffer + strlen(szStrBuffer),"%s",in_label_val);
			}
		}
		else if(ret == -1) {
			/* req-val-mnt not set in devicetree, just output the mounting option present */

			/* Handle every GPIO */
			for (i = 0; i < size; i++) {

				/* Get description of pin and value */
				dev_read_string_index(dev, "in-pins-mnt", i, &in_label);
				if (dm_gpio_get_value(in_gpios+i))
					dev_read_string_index(dev, "in-high-mnt", i, &in_label_val);
				else
					dev_read_string_index(dev, "in-low-mnt", i, &in_label_val);

				if (size == 1) {
					mute_debug_port(0);
					printf("MOUNT_OPTION..........%s\n",in_label_val);
					gpio_exists = 0;
					mute_debug_port(1);
					continue;
				}

				/* Print results to szStrBuffer */
				if (first_pins)
					first_pins = 0;
				else
					sprintf(szStrBuffer + strlen(szStrBuffer),", ");

				sprintf(szStrBuffer + strlen(szStrBuffer),", %s=%s", in_label, in_label_val);
			}
		}
		else {
			/* Unexpected return value from dev_read_u32_array() */
			failed = 1;
			sprintf(szStrBuffer + strlen(szStrBuffer),"req-val-mnt not okay");
		}

		/* Free GPIOs */
		free_gpios(dev, in_gpios);

	}

	mute_debug_port(0);

	if (gpio_exists) {
		printf("MOUNT_OPTION..........");
		if (failed)
			test_OkOrFail(-1, 1, szStrBuffer);
		else
			test_OkOrFail(0, 1, szStrBuffer);
	}

	return 0;
}
