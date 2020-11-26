/*
 * usb_test.c
 *
 *  Created on: Apr 29, 2020
 *      Author: developer
 */
#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <part.h>
#include <usb.h>
#include "selftest.h"
#include "serial_test.h" // mute_debug_port()

static uint8_t devices = 0;
static uint8_t hubs = 0;

static void USBfind(struct usb_device *udev){

	int uid;
	struct udevice *child;
	struct usb_device *child_udev;

	for (device_find_first_child(udev->dev, &child);
		     child;
		     device_find_next_child(&child)) {

			if (!device_active(child))
				continue;

			uid = device_get_uclass_id(child);
			if (uid == UCLASS_USB_HUB)
				hubs++;
			else
				devices++;
			child_udev = dev_get_parent_priv(child);
			if ((device_get_uclass_id(child) != UCLASS_USB_EMUL) &&
			    (device_get_uclass_id(child) != UCLASS_BLK)) {
				USBfind(child_udev);
			}
	}
}


int test_USBHost( char * szStrBuffer )
{
	int result = 0;
	struct udevice *bus;
	struct usb_device *udev;
	struct udevice *dev;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	mute_debug_port(1);
	usb_init(0);
	mute_debug_port(0);

	for (uclass_find_first_device(UCLASS_USB, &bus);
		bus;
		uclass_find_next_device(&bus)) {

		if (!device_active(bus))
			continue;

		printf("USB Host..............");

		devices = 0;
		hubs = 0;

		device_find_first_child(bus, &dev);

		if (dev && device_active(dev)) {
			udev = dev_get_parent_priv(dev);
			USBfind(udev);
		}
		/* Test failes, if no devices or less then one hub is found */
		if(!devices || hubs < 1)
			result = -1;
		else
			result = 0;


		if(result)
			sprintf(szStrBuffer,"No device found");
		else
		{
			sprintf(szStrBuffer,"Hubs: %d / Devices: %d", hubs, devices);
		}

		test_OkOrFail(result,1,szStrBuffer);
	}
	usb_stop();

	return result;
}

