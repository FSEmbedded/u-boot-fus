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
#include <linux/usb/otg.h>
#include "selftest.h"
#include "serial_test.h" // mute_debug_port()

static uint8_t devices = 0;
static uint8_t hubs = 0;
static uint8_t storages = 0;

struct usb_uclass_priv {
	int companion_device_count;
};

static void USBfind(struct usb_device *udev){

	int uid;
	struct udevice *child;
	struct usb_device *child_udev;
	struct usb_interface *iface;

	for (device_find_first_child(udev->dev, &child);
		     child;
		     device_find_next_child(&child)) {

			if (!device_active(child))
				continue;

			uid = device_get_uclass_id(child);
			iface = &child_udev->config.if_desc[0];
			if (uid == UCLASS_USB_HUB)
				hubs++;
			else if (child_udev->descriptor.bDeviceClass == 0 ||
				iface->desc.bInterfaceClass == USB_CLASS_MASS_STORAGE ||
				iface->desc.bInterfaceSubClass >= US_SC_MIN ||
				iface->desc.bInterfaceSubClass <= US_SC_MAX) {
				/* A storage is also a device, so increment both */
				storages++;
				devices++;
			}
			else
				devices++;
			child_udev = dev_get_parent_priv(child);
			if ((device_get_uclass_id(child) != UCLASS_USB_EMUL) &&
			    (device_get_uclass_id(child) != UCLASS_BLK)) {
				USBfind(child_udev);
			}
	}
}

static void usb_scan_bus(struct udevice *bus, bool recurse)
{
	struct usb_bus_priv *priv;
	struct udevice *dev;
	int ret;

	priv = dev_get_uclass_priv(bus);

	assert(recurse);	/* TODO: Support non-recusive */

	printf("scanning bus %d for devices... ", dev_seq(bus));
	debug("\n");
	ret = usb_scan_device(bus, 0, USB_SPEED_FULL, &dev);
	if (ret)
		printf("failed, error %d\n", ret);
	else if (priv->next_addr == 0)
		printf("No USB Device found\n");
	else
		printf("%d USB Device(s) found\n", priv->next_addr);
}

static void remove_inactive_children(struct uclass *uc, struct udevice *bus)
{
	uclass_foreach_dev(bus, uc) {
		struct udevice *dev, *next;
		if (!device_active(bus))
			continue;
		device_foreach_child_safe(dev, next, bus) {
			if (!device_active(dev))
				device_unbind(dev);
		}
	}
}

/* Here we need a separate function, that only looks for USB Hosts.
 * The reason for that is, that usb_init initializes every USB device
 * and therefore disconnects the fastboot USB device from the UUU tool.
 */
static int usb_init_host(void)
{
	int controllers_initialized = 0;
	struct usb_uclass_priv *uc_priv;
	struct usb_bus_priv *priv;
	struct uclass *uc;
	struct udevice *bus;
	int count = 0;
	int ret;

	ret = uclass_get(UCLASS_USB, &uc);
	if (ret)
		return ret;

	uc_priv = uclass_get_priv(uc);

	uclass_foreach_dev(bus, uc) {
		if (usb_get_dr_mode(dev_ofnode(bus)) != USB_DR_MODE_HOST)
			continue;
		/* init low_level USB */
		printf("USB%d:   ", count);
		count++;
		ret = device_probe(bus);
		if (ret == -ENODEV) {	/* No such device. */
			puts("Port not available.\n");
			controllers_initialized++;
			continue;
		}

		if (ret) {		/* Other error. */
			printf("probe failed, error %d\n", ret);
			continue;
		}
		controllers_initialized++;
	}

	/*
	 * lowlevel init done, now scan the bus for devices i.e. search HUBs
	 * and configure them, first scan primary controllers.
	 */
	uclass_foreach_dev(bus, uc) {
		if (usb_get_dr_mode(dev_ofnode(bus)) != USB_DR_MODE_HOST)
			continue;
		if (!device_active(bus))
			continue;

		priv = dev_get_uclass_priv(bus);
		if (!priv->companion)
			usb_scan_bus(bus, true);
	}

	/*
	 * Now that the primary controllers have been scanned and have handed
	 * over any devices they do not understand to their companions, scan
	 * the companions if necessary.
	 */
	if (uc_priv->companion_device_count) {
		uclass_foreach_dev(bus, uc) {
			if (usb_get_dr_mode(dev_ofnode(bus)) != USB_DR_MODE_HOST)
				continue;
			if (!device_active(bus))
				continue;

			priv = dev_get_uclass_priv(bus);
			if (priv->companion)
				usb_scan_bus(bus, true);
		}
	}

	/* Remove any devices that were not found on this scan */
	remove_inactive_children(uc, bus);

	ret = uclass_get(UCLASS_USB_HUB, &uc);
	if (ret)
		return ret;
	remove_inactive_children(uc, bus);

	/* if we were not able to find at least one working bus, bail out */
	if (!count){
			printf("No controllers found\n");
	}
	else if (controllers_initialized == 0) {
			printf("USB error: all controllers failed lowlevel init\n");
	}

	return 0;
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
	usb_init_host();
	mute_debug_port(0);

	for (uclass_find_first_device(UCLASS_USB, &bus); bus; uclass_find_next_device(&bus)) {

		if (usb_get_dr_mode(dev_ofnode(bus)) != USB_DR_MODE_HOST)
			continue;

		if (!device_active(bus))
			continue;

		printf("USB Host..............");

		devices = 0;
		hubs = 0;
		storages = 0;

		device_find_first_child(bus, &dev);

		if (dev && device_active(dev)) {
			udev = dev_get_parent_priv(dev);
			USBfind(udev);
		}
		/* Test failes, if no devices or less then one hub is found */
		if(!devices || (hubs < 1 && storages < 1))
			result = -1;
		else
			result = 0;


		if(result)
			sprintf(szStrBuffer,"No device found");
		else
		{
			sprintf(szStrBuffer,"Devices: %d / Hubs: %d / Storages: %d", storages, hubs, devices);
		}

		test_OkOrFail(result,1,szStrBuffer);
	}
	//usb_stop();

	return result;
}

