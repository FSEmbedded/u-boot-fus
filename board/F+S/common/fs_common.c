/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * Common code used on Layerscape Boards
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-generic/gpio.h>

int fs_set_gpio(const char *gpio_name, int output){
	struct gpio_desc gpio;
	int ret;

	ret = dm_gpio_lookup_name(gpio_name, &gpio);
	if(ret<0)
	{
		printf("GPIO: '%s' not found\n", gpio_name);
        return ret;
	}
    
    ret = dm_gpio_request(&gpio, __func__);
    if(ret<0)
    {
        printf("GPIO: requesting \"%s\" failed: %d\n", gpio_name, ret);
        return ret;
    }

    ret = dm_gpio_set_dir_flags(&gpio, GPIOD_IS_OUT);
    if(ret < 0)
    {
        printf("GPIO: '%s' set as output failed: %d\n", gpio_name, ret);
        return ret;
    }

    ret = dm_gpio_set_value(&gpio, output);
    if(ret<0)
    {
        printf("GPIO: '%s' set output=\"%d\" failed: %d\n",gpio_name, output, ret);
        return ret;
    }

    ret = dm_gpio_free(gpio.dev, &gpio);
    if(ret<0)
    {
        printf("GPIO: free \"%s\" failed: %d\n", gpio_name, ret);
        return ret;
    }

    return ret;
}