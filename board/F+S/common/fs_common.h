/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * Common code used on Layerscape Boards
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FS_COMMON__
#define __FS_COMMON__

#define SFP_OUIDR3_ADDR 0x01e80280ull

/*
 * fs_get_gpio() - Look for a gpio_pin with given name and set output
 *
 * @gpio_name: Name to look up
 * @output: set gpio pin to 0 or 1
 * @return: 0 if OK, -ve on error
 */
int fs_set_gpio(const char *gpio_name, int output);

#endif // __FS_COMMON__
