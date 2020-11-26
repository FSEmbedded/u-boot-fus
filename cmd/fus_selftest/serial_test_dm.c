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
#include <serial.h>
#include "serial_test_dm.h"

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

void mute_debug_port(int on)
{
	struct udevice *dev;
	struct dm_serial_ops *ops;
	int port = get_debug_port();

	uclass_get_device_by_seq(UCLASS_SERIAL,port,&dev);
	ops = serial_get_ops(dev);

	mdelay(1); // Delay needed or else 1-2 characters glitch to wrong port
	wait_tx_buf_empty(dev, ops);
	set_loopback(port,on);
}

// main functions

int test_serial()
{
	struct udevice *dev;
	struct dm_serial_ops *ops;
	int ret, port, msec;
	int mismatch = 0;
	char c = 0;

	ret = init_uart();

	// Seed the current time to achieve a better pseudo randomizer
	srand(timer_get_us());

	for(int i=0; i<BUFFERSIZE; i++)
		tx_buffer[i] = (char) ((rand() % 223) + 33);

	for(port=0; get_serial_ports(port)!=NULL; port++)
	{
		printf("SERIAL %d: (%s)", port, get_serial_ports(port));
		ret = uclass_get_device_by_seq(UCLASS_SERIAL,port,&dev);
		if (ret)
		{
			printf(" FAILED (Failed to find SERIAL%d)", port);
			continue;
		}
		ops = serial_get_ops(dev);
		ops->setbrg(dev,115200);

		mismatch = 0;
		printf("\n");
		printf("  internal loopback...");


		// Set internal
		wait_tx_buf_empty(dev, ops);
		set_loopback(port,1);

		for(int j=0; j<BUFFERSIZE;j++)
		{

			// send character
			ret = ops->putc(dev,tx_buffer[j]);

			// check receive buffer for input
			msec = 0;
			while ((!ops->pending(dev,INPUT)) && (msec < TIMEOUT))
			{
				mdelay(10);
				msec += 10;
			}
			if (msec >= TIMEOUT)
				break;

			// get character
			c = ops->getc(dev);

			if (c != tx_buffer[j])
			{
				mismatch = 1;
			}
		}

		// Set external
		wait_tx_buf_empty(dev, ops);
		set_loopback(port,0);

		// print result
		if (msec >= TIMEOUT)
			printf("FAILED\t(Failed with timeout)");
		else if(mismatch)
			printf("FAILED\t(Failed with TX/RX mismatch)");
		else
			printf("OK");


		mismatch = 0;
		printf("\n");
		printf("  external loopback...");

		if (port != get_debug_port())
		{
			for(int j=0; j<BUFFERSIZE;j++) {

				// send character
				ret = ops->putc(dev,tx_buffer[j]);

				// check receive buffer for input
				msec = 0;
				while ((!ops->pending(dev,INPUT)) && (msec < TIMEOUT)) {
					mdelay(10);
					msec += 10;
				}
				if (msec >= TIMEOUT) {
					break;
				}

				// get character
				c = ops->getc(dev);

				if (c != tx_buffer[j])
				{
					mismatch = 1;
					// Don't break out of for(), handle timeout first.
				}
			}
			// print result
			if (msec >= TIMEOUT)
				printf("FAILED\t(Failed with timeout)");
			else if(mismatch)
				printf("FAILED\t(Failed with TX/RX mismatch)");
			else
				printf("OK");
		}
		else
			printf("skipped\t(Debug port)");
		printf("\n");
	}
	return 0;
}
