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
#include <dm/device-internal.h>
#include <asm/gpio.h>
#include <serial.h>
#ifndef CONFIG_DM_SERIAL
#include "serial_test.h"
#else
#include "serial_test_dm.h"
#endif
#include "selftest.h"
#include "gpio_test.h"
#define TIMEOUT 1 // ms (1 sec)

#define OUTPUT 0
#define INPUT 1

#define BUFFERSIZE 32

#define CTS_READ_EN 0
#define CTS_WRITE_EN 1

static char tx_buffer[BUFFERSIZE];

static struct gpio_desc rs485_cts[1];

static void wait_tx_buf_empty(struct udevice *dev, struct dm_serial_ops *ops)
{
	int count;
	for( count = 0; count < 500; count++ )
	{
		if( !ops->pending(dev,OUTPUT) )
			break;

		mdelay(1);
	}
	while(ops->pending(dev,INPUT))
		ops->getc(dev);
}

static void clear_rx_buf(struct udevice *dev, struct dm_serial_ops *ops)
{
	/* Get the character 0x0 that is seen by RX because of the flank 0 -> 1.
	 * The flank is created by value 0 in GPIO mode and the low-active UART signal.
	 */
    mdelay(1);

	while (ops->pending(dev,INPUT))
		ops->getc(dev);
}

void serialtest_puts(struct udevice *dev, struct dm_serial_ops *ops, char *s)
{
	int i = 0;
	if (rs485_cts->dev != NULL)
		dm_gpio_set_value(rs485_cts,CTS_WRITE_EN);
	while (*s) {
		mdelay(1);
		ops->putc(dev,*s++);
	}
	if (rs485_cts->dev != NULL) {
		while (ops->pending(dev,OUTPUT) && (i++ < 100))
			mdelay(1);
		mdelay(1);
		dm_gpio_set_value(rs485_cts,CTS_READ_EN);
	}
}

void init_rs485_cts(struct udevice *dev)
{
	if (!gpio_request_by_name(dev, "rs485-cts-pin", 0, rs485_cts, 0))
	{
		char label[32];
		sprintf(label,"SERIAL%d_RS485_CTS",dev->seq);
		dm_gpio_free(dev, rs485_cts);
		dm_gpio_request(rs485_cts, label);
		dm_gpio_set_dir_flags(rs485_cts, GPIOD_IS_OUT);
		dm_gpio_set_value(rs485_cts,CTS_READ_EN);
	}
}


int tx_to_rx_test(struct udevice *dev, struct dm_serial_ops *ops, int *msec)
{
    int mismatch = 0;
    int tx_pos = 0;
    char c = 0;

    *msec = 0;

	while(tx_pos < BUFFERSIZE)
	{
	    serialtest_puts(dev,ops,tx_buffer+tx_pos);
		mdelay(100);

	    for(int j=tx_pos; j<BUFFERSIZE-1;j++)
	    {
	        // check receive buffer for input
	        *msec = 0;
	        while ((!ops->pending(dev,INPUT)) && (*msec < TIMEOUT))
	        {
	            mdelay(1);
	            *msec += 1;
	        }
	        if (*msec >= TIMEOUT)
	        {
	            tx_pos = (tx_pos != j) ? j : BUFFERSIZE;
	            break;
	        }

	        // get character
	        c = ops->getc(dev);

	        if (c != tx_buffer[j])
	            mismatch = 1;

	    }
	    // got the whole buffer
	    if (*msec < TIMEOUT)
	        tx_pos = BUFFERSIZE;
	}

    return mismatch;
}


void mute_debug_port(int on)
{
	struct dm_serial_ops *ops;
	struct udevice *dev = get_debug_dev();

	ops = serial_get_ops(dev);

	mdelay(1); // Delay needed or else 1-2 characters glitch to wrong port
	wait_tx_buf_empty(dev, ops);
	set_loopback(dev,on);
	if (!on)
		clear_rx_buf(dev, ops);
}

// main functions

int test_serial(char *szStrBuffer)
{
	struct uclass *uc;
	struct udevice *dev;
	struct dm_serial_ops *ops;
	const void *fdt = gd->fdt_blob;
	int node;
	int ret = 0, msec = 0;
	int mismatch = 0;
	const char *in_label, *out_label;
	int gpio_exists, first_pins, i;
	u32 failmask;
	ret = init_uart();

	// Seed the current time to achieve a better pseudo randomizer
	//srand(timer_get_us());

	for (int i=0; i<BUFFERSIZE-1; i++) {
		//tx_buffer[i] = (char) ((rand() % 223) + 33);
		tx_buffer[i] = (char) (i + 33);
	}

	tx_buffer[BUFFERSIZE-1] = 0;

	if (uclass_get(UCLASS_SERIAL, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {

		failmask = 0;
		gpio_exists = 0;
		first_pins  = 1;

		ret = 0;
		/* Clear reason-string */
		szStrBuffer[0] = '\0';

		dev->driver->ofdata_to_platdata(dev);

		device_probe(dev);

		node = dev_of_offset(dev);

		printf("SERIAL %d: (%s)\n", dev->seq, fdt_get_property(fdt, node, "port-name", NULL)->data);
		// Wait for debug output to finish
		if (fdt_get_property(fdt, node, "debug-port", NULL))
			mdelay(1);

		ops = serial_get_ops(dev);
		ops->setbrg(dev,115200);

		mismatch = 0;
		printf("  internal loopback...");
		mdelay(10);

		// Reset potential RS485 CTS pin
		rs485_cts->dev = NULL;

		// Set internal
		wait_tx_buf_empty(dev, ops);

		set_loopback(dev,1);

		// Test TX to RX
		mismatch = tx_to_rx_test(dev, ops, &msec);

		// Set external
		wait_tx_buf_empty(dev, ops);

		set_loopback(dev,0);
		clear_rx_buf(dev, ops);

		// Print result
		if (msec >= TIMEOUT) {
			sprintf(szStrBuffer, "Failed with timeout");
			ret = -1;
		}
		else if (mismatch) {
			sprintf(szStrBuffer, "Failed with TX/RX mismatch");
			ret = -1;
		}
		test_OkOrFail(ret, 1, szStrBuffer);

		printf("  external loopback...");

		// If RS485 initialize CTS pin
		init_rs485_cts(dev);

		ret = 0;
		/* Clear reason-string */
		szStrBuffer[0] = '\0';

		if (test_gpio_dev(dev, NULL, &failmask))
			gpio_exists = 1;

		if (!fdt_get_property(fdt, node, "debug-port", NULL)) {
			// Test TX to RX
			mismatch = tx_to_rx_test(dev, ops, &msec);

			// Print result
			if (msec >= TIMEOUT) {
				sprintf(szStrBuffer + strlen(szStrBuffer), "TX/RX timeout");
				ret = -1;
			}
			else if (mismatch) {
				sprintf(szStrBuffer + strlen(szStrBuffer), "TX/RX mismatch");
				ret = -1;
			}
		}
		else {
			sprintf(szStrBuffer + strlen(szStrBuffer), "Debug port");
			if (gpio_exists) {
				sprintf(szStrBuffer + strlen(szStrBuffer), ", ");
				if (!failmask)
					sprintf(szStrBuffer + strlen(szStrBuffer), "TX/RX skipped");
			}
			else
				ret = 1;
		}

		if (failmask) {
			if (ret == -1)
				sprintf(szStrBuffer + strlen(szStrBuffer), ", ");
			ret = -1;
		}

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

		test_OkOrFail(ret, 1, szStrBuffer);
	}
	return 0;
}
