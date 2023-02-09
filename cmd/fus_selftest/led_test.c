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
#include "led_test.h"
#include "selftest.h"

#include "serial_test.h" // mute_debug_port()

#define OUTPUT 0
#define INPUT 1

static int size;

// helper functions

static int get_gpio_lists(struct udevice *dev, struct gpio_desc **in_gpios, struct gpio_desc **out_gpios)
{
	int ret;

	size = 0;

	/* Get GPIO List */
	size = gpio_get_list_count(dev, "in-gpios-led");
	*in_gpios = calloc(size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "in-gpios-led", *in_gpios, size, 0);
	if (ret < 0)
		return -1;
	size = gpio_get_list_count(dev, "out-gpios-led");
	*out_gpios = calloc(size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "out-gpios-led", *out_gpios, size, 0);
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

int test_led(char *szStrBuffer)
{
	struct udevice *dev;
	struct uclass *uc;
	int gpio_exists, failed, first_pins, i;
	struct gpio_desc *in_gpios, *out_gpios;
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

		bool inverted = false;
		u32 failmask = 0;

		/* Get all GPIO-Lists */
		if (get_gpio_lists(dev, &in_gpios, &out_gpios))
			continue;

		if (init_gpios(dev, in_gpios,"in-pins-led", INPUT) || init_gpios(dev, out_gpios, "out-pins-led", OUTPUT))
			continue;

		/* Mark that at least one device of the driver has a gpio-test-config */
		if (size > 0) {
			gpio_exists = 1;

			/* Set IOMUX to ledgpio */
			pinctrl_select_state(dev,"ledgpio");
		}

		if (ofnode_get_property(dev->node, "is-inverted", NULL))
			inverted = true;

		/* Test every GPIO-Bit */
		/* Active High: 001, 010, 100 */
		for (i = 0; i < size; i++) {
			set_test_bit(out_gpios, i, 1);
			failmask |= cmp_test_bit(in_gpios, i, !inverted);
		}
		/* Active Low: 110, 101, 011 */
		for (i = 0; i < size; i++) {
			set_test_bit(out_gpios, i, 0);
			failmask |= cmp_test_bit(in_gpios, i, inverted);
		}

		if (failmask)
				failed = 1;

		for (i = 0; failmask; i++) {
			if (failmask & (1 << i)) {
				dev_read_string_index(dev, "in-pins-led", i, &in_label);
				dev_read_string_index(dev, "out-pins-led", i, &out_label);
				if (first_pins) {
					first_pins = 0;
					sprintf(szStrBuffer + strlen(szStrBuffer),"%s->%s", out_label, in_label);
				}
				else
					sprintf(szStrBuffer + strlen(szStrBuffer),", %s->%s", out_label, in_label);
				failmask &= ~(1 << i);
			}
		}

		/* Free GPIOs */
		free_gpios(dev, in_gpios);
		free_gpios(dev, out_gpios);

	}

	mute_debug_port(0);

	if (gpio_exists) {
		printf("LED...................");
		if (failed)
			test_OkOrFail(-1, 1, szStrBuffer);
		else
			test_OkOrFail(0, 1, NULL);
	}

	return 0;
}
