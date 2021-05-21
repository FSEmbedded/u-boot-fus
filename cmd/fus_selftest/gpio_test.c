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
#include "gpio_test.h"
#include "selftest.h"
#include "check_config.h"

#include "serial_test.h" // mute_debug_port()

#define OUTPUT 0
#define INPUT 1

static int size;

enum{
	TEST_SD,
	TEST_SPI,
	TEST_I2C,
	TEST_CAM,
	TEST_AUD,
	TEST_CAN,
	TEST_NONE,
};

struct test_driver {
	int uclass;
	char * name;
};

// helper functions

static int get_gpio_lists(struct udevice *dev, struct gpio_desc **in_gpios, struct gpio_desc **out_gpios)
{
	int ret, in_size, out_size;

	size = 0;

	/* Get GPIO List */
	in_size = gpio_get_list_count(dev, "in-gpios");
	*in_gpios = calloc(in_size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "in-gpios", *in_gpios, in_size, 0);
	if (ret < 0)
		return -1;
	out_size = gpio_get_list_count(dev, "out-gpios");
	*out_gpios = calloc(out_size, sizeof(struct gpio_desc));
	ret = gpio_request_list_by_name(dev, "out-gpios", *out_gpios, out_size, 0);
	if (ret < 0)
		return -1;

	if ((in_size != out_size) || (in_size < 0))
		return -1;

	size = (in_size < out_size) ? in_size : out_size;

	return 0;
}

static int init_gpios(struct udevice *dev, struct gpio_desc *gpios, int input)
{
	const char * label;

	for (int i = 0; i < size; i++) {
		dev_read_string_index(dev, input ? "in-pins" : "out-pins", i, &label);
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

int test_gpio_dev(struct udevice *dev, u32 *failmask)
{
	int i;
	struct gpio_desc *in_gpios, *out_gpios;

	/* Init failmask */
	*failmask = 0;

	/* Get both GPIO-Lists (in-pins,in-gpios & out-pins,out-gpios) */
	if (get_gpio_lists(dev, &in_gpios, &out_gpios))
		return size;

	/* Initialize and request the GPIOs */
	if (init_gpios(dev, in_gpios, INPUT) || init_gpios(dev, out_gpios, OUTPUT))
		return size;

	/* Mark that at least one device of the driver has a gpio-test-config */
	if (size > 0) {
		/* Set IOMUX to gpiotest */
		pinctrl_select_state(dev,"gpiotest");

		/* Test every GPIO-Bit    */
		/* Active High: 001, 010, 100 */
		for (i = 0; i < size; i++) {
			set_test_bit(out_gpios, i, 1);
			*failmask |= cmp_test_bit(in_gpios, i, 1);
		}
		/* Active Low: 110, 101, 011 */
		for (i = 0; i < size; i++) {
			set_test_bit(out_gpios, i, 0);
			*failmask |= cmp_test_bit(in_gpios, i, 0);
		}

		/* Free GPIOs */
		free_gpios(dev, in_gpios);
		free_gpios(dev, out_gpios);
	}

	return size;
}

// main functions

int test_gpio(int uclass, char *szStrBuffer)
{
	struct udevice *dev;
	int port, gpio_exists, failed, first_pins, i;
	const char *in_label, *out_label;
	char *device_name;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	mute_debug_port(1);

	port = 0;
	gpio_exists = 0;
	failed = 0;
	first_pins = 1;

	if (uclass == UCLASS_SPI)
		device_name = "SPI.";
	else if (uclass == UCLASS_I2C)
		device_name = "I2C.";
	else if (uclass == UCLASS_MMC)
		device_name = "SD..";
	else if (uclass == UCLASS_GPIO)
		device_name = "GPIO";
	else
		device_name = "UNKNOWN";

	sprintf(szStrBuffer,"Pins: ");

	while (uclass_get_device(uclass,port,&dev) == 0) {

		u32 failmask = 0;
		ofnode sgtl_node;

		/* CAN / mcp251x */
		if (dev_read_bool(dev, "can-spi-mcp251x")) {
			if (has_feature(FEAT_CAN)) {
				port++;
				continue;
			}
		}

		/* WLAN / mcp251x */
		if (dev_read_bool(dev, "is-wlan")) {
			if (has_feature(FEAT_WLAN)) {
				port++;
				continue;
			}
		}

		/* AUDIO / sgtl5000 */
		sgtl_node = dev_read_subnode(dev, "sgtl5000");
		if (sgtl_node.of_offset >= 0) {
			if (has_feature(FEAT_SGTL5000)) {
				port++;
				continue;
			}
		}

		/* If a size is returned, gpios exists */
		if(test_gpio_dev(dev, &failmask))
			gpio_exists = 1;

		if (failmask)
				failed = 1;

		for (i = 0; failmask; i++) {
			if (failmask & (1 << i)) {
				dev_read_string_index(dev, "in-pins", i, &in_label);
				dev_read_string_index(dev, "out-pins", i, &out_label);
				if (first_pins) {
					first_pins = 0;
					sprintf(szStrBuffer + strlen(szStrBuffer),"%s->%s", out_label, in_label);
				}
				else
					sprintf(szStrBuffer + strlen(szStrBuffer),", %s->%s", out_label, in_label);
				failmask &= ~(1 << i);
			}
		}

		port++;
	}

	mute_debug_port(0);

	if (gpio_exists) {
		printf("%s..................", device_name);
		if (failed)
			test_OkOrFail(-1, 1, szStrBuffer);
		else
			test_OkOrFail(0, 1, NULL);
	}

	return 0;
}
