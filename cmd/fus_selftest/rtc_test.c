/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#include <common.h>
#include <command.h>
#include <dm/device.h>
#include <dm.h>
#include <i2c.h>
#include <rtc_def.h>
#include <rtc.h>
#include "rtc_test.h"
#include "selftest.h"


static struct udevice *dev;
static struct udevice *devp = NULL;
static struct rtc_time rtc_send;
static struct rtc_time rtc_rec;

static int ret = 0;
static ulong start_time;
enum RET { SKIP = 1, INIT_FAILED, DEVICE_NOT_FOUND, ERROR_WRITE, ERROR_READ};


int test_rtc_start(void){
	ret = uclass_get_device(UCLASS_RTC,0,&dev);

	if (ret == 0) {

		struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
		struct udevice *bus = dev_get_parent(dev);
		struct rtc_ops *ops = rtc_get_ops(dev);

		ret = dm_i2c_probe(bus,chip->chip_addr,chip->flags,&devp);
		if (ret){
			ret = -DEVICE_NOT_FOUND;
			return ret;
		}

		rtc_send.tm_hour = 12;
		rtc_send.tm_min = 20;
		rtc_send.tm_sec = 30;
		ret = ops->set(dev,&rtc_send);
		start_time = get_timer(0);
		if (ret){
			ret = -ERROR_WRITE;
			return ret;
		}
	}
	else if (ret == -ENODEV) {
		ret = -SKIP;
	}
	else {
		ret = -INIT_FAILED;
	}

	return ret;
}

int test_rtc_end(char *szStrBuffer){

	int rtc_dSec;
	int timer_dSec;
	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	if (ret == -SKIP){ /* No RTC in device tree */
        return 1;
	}
	else {
		printf("RTC..................."); /* Only print if RTC is in device tree */

		if (ret == -DEVICE_NOT_FOUND)
			  sprintf(szStrBuffer, "Device not found");
		else if (ret == -ERROR_WRITE)
			sprintf(szStrBuffer, "Error write");
		else if (ret == -INIT_FAILED)
			sprintf(szStrBuffer, "Probe failed");

		if (ret){
			test_OkOrFail(ret, 1, szStrBuffer);
			return ret;
		}
	}

	struct rtc_ops *ops = rtc_get_ops(dev);
	
	ret = ops->get(dev,&rtc_rec);

	/* Measure time with internal timer and check 
	 * if RTC value is within +-1 second.
	 */
	timer_dSec = get_timer(start_time)/1000;
	rtc_dSec = rtc_rec.tm_sec-rtc_send.tm_sec;

	if (rtc_send.tm_hour !=  rtc_rec.tm_hour ||
		rtc_send.tm_min  !=  rtc_rec.tm_min  ||
		rtc_dSec  >  timer_dSec+1			 ||
		rtc_dSec  <  timer_dSec-1){
		ret = -ERROR_READ;
		sprintf(szStrBuffer, "Wrong time read");
	}
	test_OkOrFail(ret, 1, szStrBuffer);

	return ret;

}
