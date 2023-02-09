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
#include "relay_test.h"
#include "selftest.h"
#include <malloc.h>

#include "serial_test.h" // mute_debug_port()

#define OUTPUT 0
#define INPUT 1

static int size;

// helper functions

static int get_gpio_lists(struct udevice *dev, struct gpio_desc **in_gpios_off, struct gpio_desc **in_gpios_on, struct gpio_desc **out_gpios)
{
	int ret;

	size = 0;

	/* Get GPIO List */
	size = gpio_get_list_count(dev, "in-gpios-relay-off");
	*in_gpios_off = calloc(size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "in-gpios-relay-off", *in_gpios_off, size, 0);
	if (ret < 0)
		return -1;
	size = gpio_get_list_count(dev, "in-gpios-relay-on");
	*in_gpios_on = calloc(size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "in-gpios-relay-on", *in_gpios_on, size, 0);
	if (ret < 0)
		return -1;
	size = gpio_get_list_count(dev, "out-gpios-relay");
	*out_gpios = calloc(size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "out-gpios-relay", *out_gpios, size, 0);
	if (ret < 0)
		return -1;

	return 0;
}

static int init_gpios(struct udevice *dev, struct gpio_desc *gpios, char *label_name, int input)
{
	const char * label;

	for (int i = 0; i < size; i++) {
		dev_read_string_index(dev, label_name, i, &label);
		dm_gpio_free(dev, (gpios+i));
		dm_gpio_request((gpios+i), label);

		if (input)
			dm_gpio_set_dir_flags((gpios+i), GPIOD_IS_IN);
		else {
			dm_gpio_set_dir_flags((gpios+i), GPIOD_IS_OUT);
			dm_gpio_set_value((gpios+i),0);
		}
	}
	/* Delay for setup */
	mdelay(10);
	return 0;
}

static int init_switch_gpio(struct udevice *dev, struct gpio_desc *gpio, char *label_name)
{
	const char * label;

	dev_read_string_index(dev, label_name, 0, &label);
	dm_gpio_free(dev, gpio);
	dm_gpio_request(gpio, label);
	dm_gpio_set_dir_flags(gpio, GPIOD_IS_OUT);
	dm_gpio_set_value(gpio,0);

	/* Delay for setup */
	mdelay(10);

	return 0;
}

static void free_gpios(struct udevice *dev, struct gpio_desc *gpios)
{
	for (int i = 0; i < size; i++)
		dm_gpio_free(dev, (gpios+i));
}

static void set_test_bit(struct gpio_desc *gpios, int bit, int active_high)
{
	int val;
	for (int i=0; i<size; i++) {
		val = active_high ? (i == bit) : (i != bit);
		dm_gpio_set_value((gpios+i),val);
	}
	/* Delay needed for longer loopbacks */
	mdelay(10);
}

static u32 cmp_test_bit(struct gpio_desc *gpios, int bit, int active_high)
{
	int cmp_val;
	u32 failmask = 0;
	for (int i=0; i<size; i++) {
		cmp_val = active_high ? (i == bit) : (i != bit);
		if (cmp_val != dm_gpio_get_value(gpios+i))
			failmask |= (1 << i);
	}
	/* Delay needed for longer loopbacks */
	mdelay(10);

	return failmask;
}

// main functions

int test_relay(char *szStrBuffer)
{
	struct udevice *dev;
	struct uclass *uc;
	int gpio_exists, failed, first_pins, i;
	struct gpio_desc *in_gpios_off, *in_gpios_on, *out_gpios;
	struct gpio_desc *switch_gpio = calloc(1, sizeof(struct gpio_desc));
	const char *in_label, *out_label;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	mute_debug_port(1);

	gpio_exists = 0;
	failed = 0;
	first_pins = 1;

	sprintf(szStrBuffer,"Pins: ");

	if (uclass_get(UCLASS_GPIO, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {

		u32 failmask_off = 0;
		u32 failmask_on = 0;

		/* Get Switch GPIO */
		if(gpio_request_by_name(dev, "switch-gpio-relay", 0, switch_gpio, 0))
			continue;

		/* Get all GPIO-Lists */
		if (get_gpio_lists(dev, &in_gpios_off, &in_gpios_on, &out_gpios))
			continue;

		/* Initialize and request the GPIOs */
		if (init_switch_gpio(dev, switch_gpio,"switch-pin-relay"))
			continue;
		if (init_gpios(dev, in_gpios_off,"in-pins-relay-off", INPUT) || init_gpios(dev, in_gpios_on,"in-pins-relay-on", INPUT) || init_gpios(dev, out_gpios, "out-pins-relay", OUTPUT))
			continue;

		/* Mark that at least one device of the driver has a gpio-test-config */
		if (size > 0) {
			gpio_exists = 1;

			/* Set IOMUX to relaygpio */
			pinctrl_select_state(dev,"relaygpio");

			/* Deactivate relay switch */
			dm_gpio_set_value(switch_gpio,0);
			mdelay(10);

			/* Test every GPIO-Bit with relay off */
			/* Active High: 001, 010, 100 */
			for (i = 0; i < size; i++) {
				set_test_bit(out_gpios, i, 1);
				failmask_off |= cmp_test_bit(in_gpios_off, i, 1);
			}
			/* Active Low: 110, 101, 011 */
			for (i = 0; i < size; i++) {
				set_test_bit(out_gpios, i, 0);
				failmask_off |= cmp_test_bit(in_gpios_off, i, 0);
			}

			/* Activate relay switch */
			dm_gpio_set_value(switch_gpio,1);
			mdelay(10);

			/* Test every GPIO-Bit with relay on */
			/* Active High: 001, 010, 100 */
			for (i = 0; i < size; i++) {
				set_test_bit(out_gpios, i, 1);
				failmask_on |= cmp_test_bit(in_gpios_on, i, 1);
			}
			/* Active Low: 110, 101, 011 */
			for (i = 0; i < size; i++) {
				set_test_bit(out_gpios, i, 0);
				failmask_on |= cmp_test_bit(in_gpios_on, i, 0);
			}

			if (failmask_off || failmask_on)
					failed = 1;

			for (i = 0; failmask_off; i++) {
				if (failmask_off & (1 << i)) {
					dev_read_string_index(dev, "in-pins-relay-off", i, &in_label);
					dev_read_string_index(dev, "out-pins-relay", i, &out_label);
					if (first_pins) {
						first_pins = 0;
						sprintf(szStrBuffer + strlen(szStrBuffer),"%s->%s", out_label, in_label);
					}
					else
						sprintf(szStrBuffer + strlen(szStrBuffer),", %s->%s", out_label, in_label);
					failmask_off &= ~(1 << i);
				}
			}

			for (i = 0; failmask_on; i++) {
				if (failmask_on & (1 << i)) {
					dev_read_string_index(dev, "in-pins-relay-on", i, &in_label);
					dev_read_string_index(dev, "out-pins-relay", i, &out_label);
					if (first_pins) {
						first_pins = 0;
						sprintf(szStrBuffer + strlen(szStrBuffer),"%s->%s", out_label, in_label);
					}
					else
						sprintf(szStrBuffer + strlen(szStrBuffer),", %s->%s", out_label, in_label);
					failmask_on &= ~(1 << i);
				}
			}
		}

		/* Free GPIOs */
		free_gpios(dev, in_gpios_off);
		free_gpios(dev, in_gpios_on);
		free_gpios(dev, out_gpios);
		dm_gpio_free(dev, switch_gpio);

	}

	mute_debug_port(0);

	if (gpio_exists) {
		printf("RELAY.................");
		if (failed)
			test_OkOrFail(-1, 1, szStrBuffer);
		else
			test_OkOrFail(0, 1, NULL);
	}

	return 0;
}
