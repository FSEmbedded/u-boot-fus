/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * sdp.h - Serial Download Protocol
 *
 * Copyright (C) 2017 Toradex
 * Author: Stefan Agner <stefan.agner@toradex.com>
 */

#ifndef __SDP_H_
#define __SDP_H_

/**
 * struct sdp_stream_ops - Call back functions for SDP in stream mode
 * @new_file:	indicate the beginning of a new file download
 * @rx_data:	handle the next data chunk of a file
 */
struct sdp_stream_ops {
	/* A new file with size should be downloaded to dnl_address */
	void (*new_file)(void *dnl_address, int size);

	/* The next data_len bytes have been received in data_buf */
	void (*rx_data)(u8 *data_buf, int data_len);
};

int sdp_init(struct udevice *udc);

#ifdef CONFIG_SPL_BUILD
#include <spl.h>

int spl_sdp_handle(struct udevice *udc, struct spl_image_info *spl_image,
		   struct spl_boot_device *bootdev);
#else
int sdp_handle(struct udevice *udc);
#endif

#endif /* __SDP_H_ */
