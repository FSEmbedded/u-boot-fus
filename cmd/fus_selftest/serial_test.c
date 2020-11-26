/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <serial.h>
#include <stdio_dev.h>
#include "serial_test.h"
#include "selftest.h"

#define TIMEOUT 1 // ms (1 sec)

#define OUTPUT 0
#define INPUT 1

#define BUFFERSIZE 128

static char tx_buffer[BUFFERSIZE];

static void wait_tx_buf_empty(struct stdio_dev *dev)
{
	int count;
	for( count = 0; count < 500; count++ )
	{
		if( !pending(dev,OUTPUT) )
			break;

		mdelay(1);
	}
	while(pending(dev,INPUT))
		dev->getc(dev);
}

static void clear_rx_buf(struct stdio_dev *dev)
{
	/* Get the character 0x0 that is seen by RX because of the flank 0 -> 1.
	 * The flank is created by value 0 in GPIO mode and the low-active UART signal.
	 */
    mdelay(1);



	while (pending(dev,INPUT))
		dev->getc(dev);
}


int tx_to_rx_test(struct stdio_dev *dev, int *msec)
{
    int mismatch = 0;
    int tx_pos = 0;
    int c = 0;

    *msec = 0;

    while(tx_pos < BUFFERSIZE)
    {
        dev->puts(dev,tx_buffer+tx_pos);
        mdelay(1);

        for(int j=tx_pos; j<BUFFERSIZE-1;j++)
        {
            // check receive buffer for input
            *msec = 0;
            while ((!pending(dev,INPUT)) && (*msec < TIMEOUT))
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
            c = dev->getc(dev);

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
	struct serial_device *sdev = get_serial_device(get_debug_port());
	struct stdio_dev *dev = &sdev->dev;
	int port = get_debug_port();

	mdelay(1); // Delay needed or else 1-2 characters glitch to wrong port
	wait_tx_buf_empty(dev);
	set_loopback(port,on);
	if (!on)
		clear_rx_buf(dev);
}

// main functions

int test_serial(char *szStrBuffer)
{
	struct serial_device *sdev;
	struct stdio_dev *dev;
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
		sdev = get_serial_device(port);
		dev = &sdev->dev;

		dev->start(dev);
		sdev->setbrg(sdev); // setbrg to CONFIG_BAUDRATE

		mismatch = 0;
		printf("  internal loopback...");
		mdelay(10);

		// Set internal
		wait_tx_buf_empty(dev);

		set_loopback(port,1);

        	// Test TX to RX
        	mismatch = tx_to_rx_test(dev, &msec);

		// Set external
		wait_tx_buf_empty(dev);
		set_loopback(port,0);
    		clear_rx_buf(dev);

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
            	mismatch = tx_to_rx_test(dev, &msec);

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
