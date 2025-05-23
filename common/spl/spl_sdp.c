// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016 Toradex
 * Author: Stefan Agner <stefan.agner@toradex.com>
 */

#include <common.h>
#include <log.h>
#include <spl.h>
#include <usb.h>
#include <g_dnl.h>
#include <sdp.h>
#include <linux/printk.h>

static int spl_sdp_get_controller(struct udevice **udc)
{
	int controller_index = CONFIG_SPL_SDP_USB_DEV;
	int index;

	index = board_usb_gadget_port_auto();
	if (index >= 0)
		controller_index = index;

	return udc_device_get_by_index(controller_index, udc);
}

int spl_sdp_stream_continue(const struct sdp_stream_ops *ops, bool single)
{
	struct udevice *udc;
	int ret;

	ret = spl_sdp_get_controller(&udc);
	if (ret)
		return ret;

	/* Should not return, unless in single mode when it returns after one
	   SDP command */
	sdp_handle(udc, ops, single);

	return 0;
}

void spl_sdp_stream_done(void)
{
	struct udevice *udc;
	int ret;

	ret = spl_sdp_get_controller(&udc);
	if (ret)
		return;

	g_dnl_unregister();
	udc_device_put(udc);
}

/**
 * Load an image with Serial Download Protocol (SDP)
 *
 * @ops:	call-back functions for stream mode
 *
 * Typically, the file is downloaded and stored at the given address. In
 * stream mode, when ops is not NULL, data is not automatically stored, but
 * instead a call-back function is called for each data chunk that can handle
 * the data on the fly. For example it can only load the FDT part of a FIT
 * image, parse it, and then only loads those images that are meaningful and
 * ignores everything else.
 */
int spl_sdp_stream_image(const struct sdp_stream_ops *ops, bool single)
{
	struct udevice *udc;
	int ret;
	static int initdone;

	if (!initdone) {
		/* Only init the USB controller once while in SPL */
		ret = spl_sdp_get_controller(&udc);
		if (ret)
			return ret;

		g_dnl_clear_detach();
		ret = g_dnl_register("usb_dnl_sdp");
		if (ret) {
			pr_err("SDP dnl register failed: %d\n", ret);
			goto err_detach;
		}

		ret = sdp_init(udc);
		if (ret) {
			pr_err("SDP init failed: %d\n", ret);
			goto err_unregister;
		}

		initdone = 1;
	}

	ret = spl_sdp_stream_continue(ops, single);
	if (single)
		return ret;
	debug("SDP ended\n");

err_unregister:
	g_dnl_unregister();
err_detach:
	udc_device_put(udc);

	return ret;
}

/**
 * Load an image with Serial Download Protocol (SDP)
 *
 * @spl_image:	info about the loaded image (ignored)
 * @bootdev:	info about the device to load from (ignored)
 *
 * Download an image with Serial Download Protocol (SDP).
 */
static int spl_sdp_load_image(struct spl_image_info *spl_image,
				struct spl_boot_device *bootdev)
{
	return spl_sdp_stream_image(NULL, false);
}

SPL_LOAD_IMAGE_METHOD("USB SDP", 0, BOOT_DEVICE_BOARD, spl_sdp_load_image);
