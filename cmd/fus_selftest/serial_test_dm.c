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
#include "selftest.h"

#define TIMEOUT 1 // ms (1 sec)

#define OUTPUT 0
#define INPUT 1

#define BUFFERSIZE 128

static char tx_buffer[BUFFERSIZE];

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
	while (*s)
		ops->putc(dev,*s++);
}


int tx_to_rx_test(struct udevice *dev, struct dm_serial_ops *ops, int *msec)
{
    int mismatch = 0;
    int tx_pos = 0;
    int c = 0;

    *msec = 0;

    while(tx_pos < BUFFERSIZE)
    {
        serialtest_puts(dev,ops,tx_buffer+tx_pos);
        mdelay(1);

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
            {
                mismatch = 1;
            }
        }
        // got the whole buffer
        if (*msec < TIMEOUT)
            tx_pos = BUFFERSIZE;
    }
    return mismatch;
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
	if (!on)
		clear_rx_buf(dev, ops);
}

// main functions

int test_serial(char *szStrBuffer)
{
	struct udevice *dev;
	struct dm_serial_ops *ops;
	int ret = 0, port = 0, msec = 0;
	int mismatch = 0;

	ret = init_uart();

	// Seed the current time to achieve a better pseudo randomizer
	srand(timer_get_us());

    for(int i=0; i<BUFFERSIZE-1; i++)
		tx_buffer[i] = (char) ((rand() % 223) + 33);

    tx_buffer[BUFFERSIZE-1] = 0;

	for(port=0; get_serial_ports(port)!=NULL; port++)
	{
		ret = 0;
		/* Clear reason-string */
		szStrBuffer[0] = '\0';

		printf("SERIAL %d: (%s)\n", port, get_serial_ports(port));
        // Wait for debug output to finish
        if(port == get_debug_port())
            mdelay(1);
		ret = uclass_get_device_by_seq(UCLASS_SERIAL,port,&dev);
		if (ret)
		{
			printf(" FAILED (Failed to find SERIAL%d)", port);
			continue;
		}

		ops = serial_get_ops(dev);
		ops->setbrg(dev,115200);

		mismatch = 0;
		printf("  internal loopback...");
		mdelay(10);

		// Set internal
		wait_tx_buf_empty(dev, ops);

		set_loopback(port,1);

        	// Test TX to RX
        	mismatch = tx_to_rx_test(dev, ops, &msec);

		// Set external
		wait_tx_buf_empty(dev, ops);
		set_loopback(port,0);
    		clear_rx_buf(dev, ops);


        	// Print result
		if (msec >= TIMEOUT) {
			sprintf(szStrBuffer, "Failed with timeout");
			ret = -1;
		}
        else if(mismatch){
			sprintf(szStrBuffer, "Failed with TX/RX mismatch");
			ret = -1;
        }
		test_OkOrFail(ret, 1, szStrBuffer);

		printf("  external loopback...");

		if (port != get_debug_port())
		{
           	 // Test TX to RX
            	mismatch = tx_to_rx_test(dev, ops, &msec);

            	// Print result
				if (msec >= TIMEOUT) {
    			sprintf(szStrBuffer, "Failed with timeout");
    			ret = -1;
				}
            else if(mismatch){
    			sprintf(szStrBuffer, "Failed with TX/RX mismatch");
    			ret = -1;
				}
			}
		else{
			sprintf(szStrBuffer, "Debug port");
			ret = 1;
		}
		test_OkOrFail(ret, 1, szStrBuffer);
	}
	return 0;
}
