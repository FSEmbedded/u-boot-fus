// SPDX-License-Identifier: GPL-2.0+
/*
 * f_sdp.c -- USB HID Serial Download Protocol
 *
 * Copyright (C) 2017 Toradex
 * Author: Stefan Agner <stefan.agner@toradex.com>
 *
 * This file implements the Serial Download Protocol (SDP) as specified in
 * the i.MX 6 Reference Manual. The SDP is a USB HID based protocol and
 * allows to download images directly to memory. The implementation
 * works with the imx_loader (imx_usb) USB client software on host side.
 *
 * Not all commands are implemented, e.g. WRITE_REGISTER, DCD_WRITE and
 * SKIP_DCD_HEADER are only stubs.
 *
 * Parts of the implementation are based on f_dfu and f_thor.
 */

#include <errno.h>
#include <common.h>
#include <console.h>
#include <env.h>
#include <log.h>
#include <malloc.h>
#include <linux/printk.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include <asm/io.h>
#include <g_dnl.h>
#include <sdp.h>
#include <spl.h>
#include <image.h>
#include <imximage.h>
#include <imx_container.h>
#include <watchdog.h>

#ifdef CONFIG_FS_BOARD_CFG
#include "../../../board/F+S/common/fs_image_common.h"
#endif


#define HID_REPORT_ID_MASK	0x000000ff

/*
 * HID class requests
 */
#define HID_REQ_GET_REPORT		0x01
#define HID_REQ_GET_IDLE		0x02
#define HID_REQ_GET_PROTOCOL		0x03
#define HID_REQ_SET_REPORT		0x09
#define HID_REQ_SET_IDLE		0x0A
#define HID_REQ_SET_PROTOCOL		0x0B

#define HID_USAGE_PAGE_LEN		76

struct hid_report {
	u8 usage_page[HID_USAGE_PAGE_LEN];
} __packed;

#define SDP_READ_REGISTER	0x0101
#define SDP_WRITE_REGISTER	0x0202
#define SDP_WRITE_FILE		0x0404
#define SDP_ERROR_STATUS	0x0505
#define SDP_DCD_WRITE		0x0a0a
#define SDP_JUMP_ADDRESS	0x0b0b
#define SDP_SKIP_DCD_HEADER	0x0c0c

#define SDP_SECURITY_CLOSED		0x12343412
#define SDP_SECURITY_OPEN		0x56787856

#define SDP_WRITE_FILE_COMPLETE		0x88888888
#define SDP_WRITE_REGISTER_COMPLETE	0x128A8A12
#define SDP_SKIP_DCD_HEADER_COMPLETE	0x900DD009
#define SDP_ERROR_IMXHEADER		0x000a0533

#define SDP_COMMAND_LEN		16

struct sdp_command {
	u16 cmd;
	u32 addr;
	u8 format;
	u32 cnt;
	u32 data;
	u8 rsvd;
} __packed;

enum sdp_state {
	SDP_STATE_IDLE,
	SDP_STATE_RX_DCD_DATA,
	SDP_STATE_RX_FILE_DATA,
	SDP_STATE_TX_SEC_CONF,
	SDP_STATE_TX_SEC_CONF_BUSY,
	SDP_STATE_TX_REGISTER,
	SDP_STATE_TX_REGISTER_BUSY,
	SDP_STATE_TX_STATUS,
	SDP_STATE_TX_STATUS_BUSY,
	SDP_STATE_JUMP,
};

struct f_sdp {
	struct usb_function		usb_function;

	struct usb_descriptor_header	**function;

	u8				altsetting;
	enum sdp_state			state;
	enum sdp_state			next_state;
	u32				dnl_address;
	u32				dnl_bytes;
	u32				dnl_bytes_remaining;
	u32				jmp_address;
	bool				always_send_status;
	u32				error_status;

	/* EP0 request */
	struct usb_request		*req;

	/* EP1 IN */
	struct usb_ep			*in_ep;
	struct usb_request		*in_req;

	bool				configuration_done;
};

static struct f_sdp *sdp_func;
static const struct sdp_stream_ops *stream_ops;

static inline struct f_sdp *func_to_sdp(struct usb_function *f)
{
	return container_of(f, struct f_sdp, usb_function);
}

static struct usb_interface_descriptor sdp_intf_runtime = {
	.bLength =		sizeof(sdp_intf_runtime),
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_HID,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

/* HID configuration */
static struct usb_class_hid_descriptor sdp_hid_desc = {
	.bLength =		sizeof(sdp_hid_desc),
	.bDescriptorType =	USB_DT_CS_DEVICE,

	.bcdCDC =		__constant_cpu_to_le16(0x0110),
	.bCountryCode =		0,
	.bNumDescriptors =	1,

	.bDescriptorType0	= USB_DT_HID_REPORT,
	.wDescriptorLength0	= HID_USAGE_PAGE_LEN,
};

static struct usb_endpoint_descriptor in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT, /*USB_DT_CS_ENDPOINT*/

	.bEndpointAddress =	1 | USB_DIR_IN,
	.bmAttributes =	USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	64,
	.bInterval =		1,
};

static struct usb_endpoint_descriptor in_hs_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT, /*USB_DT_CS_ENDPOINT*/

	.bEndpointAddress =	1 | USB_DIR_IN,
	.bmAttributes =	USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	512,
	.bInterval =		1,
};

static struct usb_descriptor_header *sdp_runtime_descs[] = {
	(struct usb_descriptor_header *)&sdp_intf_runtime,
	(struct usb_descriptor_header *)&sdp_hid_desc,
	(struct usb_descriptor_header *)&in_desc,
	NULL,
};

static struct usb_descriptor_header *sdp_runtime_hs_descs[] = {
	(struct usb_descriptor_header *)&sdp_intf_runtime,
	(struct usb_descriptor_header *)&sdp_hid_desc,
	(struct usb_descriptor_header *)&in_hs_desc,
	NULL,
};

/* This is synchronized with what the SoC implementation reports */
static struct hid_report sdp_hid_report = {
	.usage_page = {
		0x06, 0x00, 0xff, /* Usage Page */
		0x09, 0x01, /* Usage (Pointer?) */
		0xa1, 0x01, /* Collection */

		0x85, 0x01, /* Report ID */
		0x19, 0x01, /* Usage Minimum */
		0x29, 0x01, /* Usage Maximum */
		0x15, 0x00, /* Local Minimum */
		0x26, 0xFF, 0x00, /* Local Maximum? */
		0x75, 0x08, /* Report Size */
		0x95, 0x10, /* Report Count */
		0x91, 0x02, /* Output Data */

		0x85, 0x02, /* Report ID */
		0x19, 0x01, /* Usage Minimum */
		0x29, 0x01, /* Usage Maximum */
		0x15, 0x00, /* Local Minimum */
		0x26, 0xFF, 0x00, /* Local Maximum? */
		0x75, 0x80, /* Report Size 128 */
		0x95, 0x40, /* Report Count */
		0x91, 0x02, /* Output Data */

		/*
		 * When sending a file with the write -f command (OUT report
		 * 2), the answer is IN report 3 (HAB status) immediately
		 * followed by IN report 4 (transmit status). However UUU on
		 * the host side reads all IN reports with 65 bytes length (1
		 * byte report ID plus 64 bytes data). If the first report has
		 * less than 65 bytes, older USB drivers on Windows may
		 * consolidate the two reports in one single message and UUU
		 * fails to see the second report. So either have a rather
		 * long delay after the first report to make sure that it is
		 * fully handled on the host side before the second report
		 * comes in or (as we do it here) make sure that both reports
		 * use the full 65 bytes so that no free space is available in
		 * the first report anymore, which successfully avoids the
		 * message consolidation. This seems to work well on all
		 * platforms.
		 */
		0x85, 0x03, /* Report ID */
		0x19, 0x01, /* Usage Minimum */
		0x29, 0x01, /* Usage Maximum */
		0x15, 0x00, /* Local Minimum */
		0x26, 0xFF, 0x00, /* Local Maximum? */
		0x75, 0x08, /* Report Size 8 */
		//### 0x95, 0x04, /* Report Count */
		0x95, 0x40, /* Report Count */
		0x81, 0x02, /* Input Data */

		0x85, 0x04, /* Report ID */
		0x19, 0x01, /* Usage Minimum */
		0x29, 0x01, /* Usage Maximum */
		0x15, 0x00, /* Local Minimum */
		0x26, 0xFF, 0x00, /* Local Maximum? */
		0x75, 0x08, /* Report Size 8 */
		0x95, 0x40, /* Report Count */
		0x81, 0x02, /* Input Data */
		0xc0
	},
};

static const char sdp_name[] = "Serial Downloader Protocol";

/*
 * static strings, in UTF-8
 */
static struct usb_string strings_sdp_generic[] = {
	[0].s = sdp_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_sdp_generic = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_sdp_generic,
};

static struct usb_gadget_strings *sdp_generic_strings[] = {
	&stringtab_sdp_generic,
	NULL,
};

static inline void *sdp_ptr(u32 val)
{
	return (void *)(uintptr_t)val;
}

static void sdp_rx_command_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_sdp *sdp = req->context;
	int status = req->status;
	u8 *data = req->buf;
	u8 report = data[0];

	if (status != 0) {
		pr_err("Status: %d\n", status);
		return;
	}

	if (report != 1) {
		pr_err("Unexpected report %d\n", report);
		return;
	}

	struct sdp_command *cmd = req->buf + 1;

	debug("%s: command: %04x, addr: %08x, cnt: %u\n",
	      __func__, be16_to_cpu(cmd->cmd),
	      be32_to_cpu(cmd->addr), be32_to_cpu(cmd->cnt));

	switch (be16_to_cpu(cmd->cmd)) {
	case SDP_READ_REGISTER:
		sdp->always_send_status = false;
		sdp->error_status = 0x0;

		sdp->state = SDP_STATE_TX_SEC_CONF;
		sdp->dnl_address = be32_to_cpu(cmd->addr);
		sdp->dnl_bytes_remaining = be32_to_cpu(cmd->cnt);
		sdp->next_state = SDP_STATE_TX_REGISTER;
		printf("Reading %d registers at 0x%08x... ",
		       sdp->dnl_bytes_remaining, sdp->dnl_address);
		break;
	case SDP_WRITE_FILE:
		sdp->always_send_status = true;
		sdp->error_status = SDP_WRITE_FILE_COMPLETE;

		sdp->state = SDP_STATE_RX_FILE_DATA;
		sdp->dnl_address = cmd->addr ? be32_to_cpu(cmd->addr) : CONFIG_SDP_LOADADDR;
		sdp->dnl_bytes_remaining = be32_to_cpu(cmd->cnt);
		sdp->dnl_bytes = sdp->dnl_bytes_remaining;
		sdp->next_state = SDP_STATE_IDLE;

		printf("Downloading file of size %d to 0x%08x... ",
		       sdp->dnl_bytes_remaining, sdp->dnl_address);

		if (stream_ops && stream_ops->new_file) {
			stream_ops->new_file((void *)(ulong)(sdp->dnl_address),
					     sdp->dnl_bytes_remaining);
		}
		break;
	case SDP_ERROR_STATUS:
		sdp->always_send_status = true;
		sdp->error_status = 0;

		sdp->state = SDP_STATE_TX_SEC_CONF;
		sdp->next_state = SDP_STATE_IDLE;
		break;
	case SDP_DCD_WRITE:
		sdp->always_send_status = true;
		sdp->error_status = SDP_WRITE_REGISTER_COMPLETE;

		sdp->state = SDP_STATE_RX_DCD_DATA;
		sdp->dnl_bytes_remaining = be32_to_cpu(cmd->cnt);
		sdp->next_state = SDP_STATE_IDLE;
		break;
	case SDP_JUMP_ADDRESS:
		sdp->always_send_status = false;
		sdp->error_status = 0;

		sdp->jmp_address = cmd->addr ? be32_to_cpu(cmd->addr) : CONFIG_SDP_LOADADDR;
		sdp->state = SDP_STATE_TX_SEC_CONF;
		sdp->next_state = SDP_STATE_JUMP;
		break;
	case SDP_SKIP_DCD_HEADER:
		sdp->always_send_status = true;
		sdp->error_status = SDP_SKIP_DCD_HEADER_COMPLETE;

		/* Ignore command, DCD not supported anyway */
		sdp->state = SDP_STATE_TX_SEC_CONF;
		sdp->next_state = SDP_STATE_IDLE;
		break;
	default:
		pr_err("Unknown command: %04x\n", be16_to_cpu(cmd->cmd));
	}
}

static void sdp_rx_data_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_sdp *sdp = req->context;
	int status = req->status;
	u8 *data = req->buf;
	u8 report = *data++;
	int datalen = req->length - 1;

	if (status != 0) {
		pr_err("Status: %d\n", status);
		return;
	}

	if (report != 2) {
		pr_err("Unexpected report %d\n", report);
		return;
	}

	if (sdp->dnl_bytes_remaining < datalen) {
		/*
		 * Some USB stacks require to send a complete buffer as
		 * specified in the HID descriptor. This leads to longer
		 * transfers than the file length, no problem for us.
		 */
		datalen = sdp->dnl_bytes_remaining;
	}
	sdp->dnl_bytes_remaining -= datalen;

	if (sdp->state == SDP_STATE_RX_FILE_DATA) {
		if (stream_ops && stream_ops->rx_data)
			stream_ops->rx_data(data, datalen);
		else
			memcpy(sdp_ptr(sdp->dnl_address), data, datalen);
		sdp->dnl_address += datalen;
	}

	if (sdp->dnl_bytes_remaining)
		return;

#ifndef CONFIG_SPL_BUILD
	env_set_hex("filesize", sdp->dnl_bytes);
#endif
	printf("done\n");

	switch (sdp->state) {
	case SDP_STATE_RX_FILE_DATA:
		sdp->state = SDP_STATE_TX_SEC_CONF;
		break;
	case SDP_STATE_RX_DCD_DATA:
		sdp->state = SDP_STATE_TX_SEC_CONF;
		break;
	default:
		pr_err("Invalid state: %d\n", sdp->state);
	}
}

static void sdp_tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_sdp *sdp = req->context;
	int status = req->status;

	if (status != 0) {
		pr_err("Status: %d\n", status);
		return;
	}

	switch (sdp->state) {
	case SDP_STATE_TX_SEC_CONF_BUSY:
		/* Not all commands require status report */
		if (sdp->always_send_status || sdp->error_status)
			sdp->state = SDP_STATE_TX_STATUS;
		else
			sdp->state = sdp->next_state;

		break;
	case SDP_STATE_TX_STATUS_BUSY:
		sdp->state = sdp->next_state;
		break;
	case SDP_STATE_TX_REGISTER_BUSY:
		if (sdp->dnl_bytes_remaining)
			sdp->state = SDP_STATE_TX_REGISTER;
		else
			sdp->state = SDP_STATE_IDLE;
		break;
	default:
		pr_err("Wrong State: %d\n", sdp->state);
		sdp->state = SDP_STATE_IDLE;
		break;
	}
	debug("%s complete --> %d, %d/%d\n", ep->name,
	      status, req->actual, req->length);
}

static int sdp_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_gadget *gadget = f->config->cdev->gadget;
	struct usb_request *req = f->config->cdev->req;
	struct f_sdp *sdp = f->config->cdev->req->context;
	u16 len = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	int value = 0;
	u8 req_type = ctrl->bRequestType & USB_TYPE_MASK;

	debug("w_value: 0x%04x len: 0x%04x\n", w_value, len);
	debug("req_type: 0x%02x ctrl->bRequest: 0x%02x sdp->state: %d\n",
	      req_type, ctrl->bRequest, sdp->state);

	if (req_type == USB_TYPE_STANDARD) {
		if (ctrl->bRequest == USB_REQ_GET_DESCRIPTOR) {
			/* Send HID report descriptor */
			value = min(len, (u16) sizeof(sdp_hid_report));
			memcpy(req->buf, &sdp_hid_report, value);
			sdp->configuration_done = true;
		}
	}

	if (req_type == USB_TYPE_CLASS) {
		int report = w_value & HID_REPORT_ID_MASK;

		/* HID (SDP) request */
		switch (ctrl->bRequest) {
		case HID_REQ_SET_REPORT:
			switch (report) {
			case 1:
				value = SDP_COMMAND_LEN + 1;
				req->complete = sdp_rx_command_complete;
				break;
			case 2:
				value = len;
				req->complete = sdp_rx_data_complete;
				break;
			}
		}
	}

	if (value >= 0) {
		req->length = value;
		req->zero = value < len;
		value = usb_ep_queue(gadget->ep0, req, 0);
		if (value < 0) {
			debug("ep_queue --> %d\n", value);
			req->status = 0;
		}
	}

	return value;
}

static int sdp_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_gadget *gadget = c->cdev->gadget;
	struct usb_composite_dev *cdev = c->cdev;
	struct f_sdp *sdp = func_to_sdp(f);
	int rv = 0, id;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	sdp_intf_runtime.bInterfaceNumber = id;

	struct usb_ep *ep;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(gadget, &in_desc);
	if (!ep) {
		rv = -ENODEV;
		goto error;
	}

	if (gadget_is_dualspeed(gadget)) {
		/* Assume endpoint addresses are the same for both speeds */
		in_hs_desc.bEndpointAddress = in_desc.bEndpointAddress;
	}

	sdp->in_ep = ep; /* Store IN EP for enabling @ setup */

	cdev->req->context = sdp;

error:
	return rv;
}

static void sdp_unbind(struct usb_configuration *c, struct usb_function *f)
{
	free(sdp_func);
	sdp_func = NULL;
}

static struct usb_request *alloc_ep_req(struct usb_ep *ep, unsigned length)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return req;

	req->length = length;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, length);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		req = NULL;
	}

	return req;
}


static struct usb_request *sdp_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = alloc_ep_req(ep, 65);
	debug("%s: ep:%p req:%p\n", __func__, ep, req);

	if (!req)
		return NULL;

	memset(req->buf, 0, req->length);
	req->complete = sdp_tx_complete;

	return req;
}
static int sdp_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_sdp *sdp = func_to_sdp(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	int result;

	debug("%s: intf: %d alt: %d\n", __func__, intf, alt);

	if (gadget_is_dualspeed(gadget) && gadget->speed == USB_SPEED_HIGH)
		result = usb_ep_enable(sdp->in_ep, &in_hs_desc);
	else
	result = usb_ep_enable(sdp->in_ep, &in_desc);
	if (result)
		return result;
	sdp->in_req = sdp_start_ep(sdp->in_ep);
	sdp->in_req->context = sdp;

	sdp->in_ep->driver_data = cdev; /* claim */

	sdp->altsetting = alt;
	sdp->state = SDP_STATE_IDLE;

	return 0;
}

static int sdp_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_sdp *sdp = func_to_sdp(f);

	return sdp->altsetting;
}

static void sdp_disable(struct usb_function *f)
{
	struct f_sdp *sdp = func_to_sdp(f);

	usb_ep_disable(sdp->in_ep);

	if (sdp->in_req) {
		free(sdp->in_req->buf);
		usb_ep_free_request(sdp->in_ep, sdp->in_req);
		sdp->in_req = NULL;
	}
}

static int sdp_bind_config(struct usb_configuration *c)
{
	int status;

	if (!sdp_func) {
		sdp_func = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*sdp_func));
		if (!sdp_func)
			return -ENOMEM;
	}

	memset(sdp_func, 0, sizeof(*sdp_func));

	sdp_func->usb_function.name = "sdp";
	sdp_func->usb_function.hs_descriptors = sdp_runtime_hs_descs;
	sdp_func->usb_function.descriptors = sdp_runtime_descs;
	sdp_func->usb_function.bind = sdp_bind;
	sdp_func->usb_function.unbind = sdp_unbind;
	sdp_func->usb_function.set_alt = sdp_set_alt;
	sdp_func->usb_function.get_alt = sdp_get_alt;
	sdp_func->usb_function.disable = sdp_disable;
	sdp_func->usb_function.strings = sdp_generic_strings;
	sdp_func->usb_function.setup = sdp_setup;

	status = usb_add_function(c, &sdp_func->usb_function);

	return status;
}

int sdp_init(struct udevice *udc)
{
	printf("SDP: initialize...\n");
	while (!sdp_func->configuration_done) {
		if (ctrlc()) {
			puts("\rCTRL+C - Operation aborted.\n");
			return 1;
		}

		schedule();
		dm_usb_gadget_handle_interrupts(udc);
	}

	return 0;
}

static u32 sdp_jump_imxheader(void *address)
{
	flash_header_v2_t *headerv2 = address;
	ulong (*entry)(void);

	if (headerv2->header.tag != IVT_HEADER_TAG) {
		printf("Header Tag is not an IMX image\n");
		return SDP_ERROR_IMXHEADER;
	}

	printf("Jumping to 0x%08x\n", headerv2->entry);
	entry = sdp_ptr(headerv2->entry);
	entry();

	/* The image probably never returns hence we won't reach that point */
	return 0;
}

#ifdef CONFIG_SPL_BUILD
static ulong sdp_load_read(struct spl_load_info *load, ulong sector,
			   ulong count, void *buf)
{
	memcpy(buf, (void *)sector, count);

	return count;
}

static ulong search_fit_header(ulong p, int size)
{
	int i;

	for (i = 0; i < size; i += 4) {
		if (genimg_get_format((const void *)(p+i)) == IMAGE_FORMAT_FIT)
			return p + i;
	}

        return 0;
}

static int get_extra_offset(void *header, struct spl_image_info *spl_image)
{
	int extra_offset = 0;

#ifdef CONFIG_FS_BOARD_CFG
	/* Allow U-Boot image to be prepended with F&S header */
	extra_offset = spl_check_fs_header(header);
	if (extra_offset < 0)
		panic("Failed to jump to U-Boot\n");

	/* In case of signed U-Boot, load U-Boot image completely */
	if ((extra_offset > 0) && fs_image_is_signed(header)) {
		u32 size;
		void *addr;

		addr = fs_image_get_ivt_info((void *)header, &size);
		if (addr && size)
			memcpy(addr, header, size);
		if (secure_spl_load_simple_fit(spl_image, addr, size) < 0)
			panic("Failed to jump to U-Boot\n");
		jump_to_image_no_args(spl_image);
	}
#endif

	return extra_offset;
}
#endif

static void sdp_handle_in_ep(void)
{
	u8 *data = sdp_func->in_req->buf;
	u32 status;
	int datalen;

	switch (sdp_func->state) {
	case SDP_STATE_TX_SEC_CONF:
		debug("Report 3: HAB security\n");
		data[0] = 3;

		status = SDP_SECURITY_OPEN;
		memcpy(&data[1], &status, 4);
		//### sdp_func->in_req->length = 5;
		sdp_func->in_req->length = 65;
		usb_ep_queue(sdp_func->in_ep, sdp_func->in_req, 0);
		sdp_func->state = SDP_STATE_TX_SEC_CONF_BUSY;
		break;

	case SDP_STATE_TX_STATUS:
		debug("Report 4: Status\n");
		data[0] = 4;

		memcpy(&data[1], &sdp_func->error_status, 4);
		sdp_func->in_req->length = 65;
		usb_ep_queue(sdp_func->in_ep, sdp_func->in_req, 0);
		sdp_func->state = SDP_STATE_TX_STATUS_BUSY;
		break;
	case SDP_STATE_TX_REGISTER:
		debug("Report 4: Register Values\n");
		data[0] = 4;

		datalen = sdp_func->dnl_bytes_remaining;

		if (datalen > 64)
			datalen = 64;

		memcpy(&data[1], sdp_ptr(sdp_func->dnl_address), datalen);
		sdp_func->in_req->length = 65;

		sdp_func->dnl_bytes_remaining -= datalen;
		sdp_func->dnl_address += datalen;

		usb_ep_queue(sdp_func->in_ep, sdp_func->in_req, 0);
		sdp_func->state = SDP_STATE_TX_REGISTER_BUSY;
		break;
	case SDP_STATE_JUMP:
		printf("Jumping to header at 0x%08x\n", sdp_func->jmp_address);
		status = sdp_jump_imxheader(sdp_ptr(sdp_func->jmp_address));

		/* If imx header fails, try some U-Boot specific headers */
		if (status) {
#ifdef CONFIG_SPL_BUILD
			struct legacy_img_hdr *header =
				sdp_ptr(sdp_func->jmp_address);
			struct spl_image_info spl_image = {};
			int extra_offset = get_extra_offset(header, &spl_image);

			header = (void *)header + extra_offset;

			if (IS_ENABLED(CONFIG_SPL_LOAD_FIT))
				sdp_func->jmp_address = (u32)search_fit_header(
					(ulong)header, sdp_func->dnl_bytes);

			if (sdp_func->jmp_address == 0)
				panic("Error in search header, failed to jump\n");

			printf("Found header at 0x%08x\n", sdp_func->jmp_address);

			header = sdp_ptr(sdp_func->jmp_address);
			if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
			    image_get_magic(header) == FDT_MAGIC) {
				struct spl_load_info load;

				debug("Found FIT\n");
				load.priv = header;
				spl_set_bl_len(&load, 1);
				load.read = sdp_load_read;
				load.extra_offset = 0;
				spl_load_simple_fit(&spl_image, &load,
						    sdp_func->jmp_address,
						    (void *)header);
			} else {
				/* In SPL, allow jumps to U-Boot images */
				spl_parse_image_header(&spl_image, NULL, header);
			}
			jump_to_image_no_args(&spl_image);
#else
			/* In U-Boot, allow jumps to scripts */
			cmd_source_script(sdp_func->jmp_address, NULL, NULL);
#endif
		}

		sdp_func->next_state = SDP_STATE_IDLE;
		sdp_func->error_status = status;

		/* Only send Report 4 if there was an error */
		if (status)
			sdp_func->state = SDP_STATE_TX_STATUS;
		else
			sdp_func->state = SDP_STATE_IDLE;
		break;
	default:
		break;
	};
}

void sdp_handle(struct udevice *udc,
		const struct sdp_stream_ops *ops, bool single)
{
	enum sdp_state last_state = SDP_STATE_IDLE;

	stream_ops = ops;

	printf("SDP: handle requests...\n");
	while (1) {
		if (ctrlc()) {
			puts("\rCTRL+C - Operation aborted.\n");
			return;
		}

		schedule();
		dm_usb_gadget_handle_interrupts(udc);

		sdp_handle_in_ep();
		if (single) {
			if ((last_state != SDP_STATE_IDLE)
			    && (sdp_func->state == SDP_STATE_IDLE))
				break;
			last_state = sdp_func->state;
		}
	}
}

int sdp_add(struct usb_configuration *c)
{
	int id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	strings_sdp_generic[0].id = id;
	sdp_intf_runtime.iInterface = id;

	debug("%s: cdev: %p gadget: %p gadget->ep0: %p\n", __func__,
	      c->cdev, c->cdev->gadget, c->cdev->gadget->ep0);

	return sdp_bind_config(c);
}

DECLARE_GADGET_BIND_CALLBACK(usb_dnl_sdp, sdp_add);
