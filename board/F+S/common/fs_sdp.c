// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright 2025 F&S Elektronik Systeme GmbH
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <errno.h>
#include <malloc.h>
#include <dm/device.h>
#include <sdp.h>
#include <hang.h>
#include <spl.h>

#include "fs_sdp.h"
#include "fs_image_common.h"
#include "fs_cntr_common.h"

int get_bootrom_bootdev(u32 *bdev)
{
	int ret;
	ret = rom_api_query_boot_infor(QUERY_BT_DEV, bdev);
	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at QUERY_BT_DEV\n");
		return -ENODEV;
	}

	return 0;
}

int get_bootrom_bootstage(u32 *bstage)
{
	int ret = 0;
	ret |= rom_api_query_boot_infor(QUERY_BT_STAGE, bstage);

	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at QUERY_BT_STAGE\n");
		return -ENODEV;
	}
	return 0;
}

int get_bootrom_bootimg_offset(u32* offset)
{
	int ret;
	ret = rom_api_query_boot_infor(QUERY_IVT_OFF, offset);
	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at QUERY_IVT_OFF\n");
		return -ENODEV;
	}

	return 0;
}

int get_bootrom_offset(u32* offset)
{
	int ret;
	ret = rom_api_query_boot_infor(QUERY_IMG_OFF, offset);
	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at QUERY_IMG_OFF\n");
		return -ENODEV;
	}

	return 0;
}

int get_sdp_pagesize(u32 *pagesize)
{
	*pagesize = PAGESIZE_USB;

	return 0;
}

/**
 * is_boot_from_stream_device
 *
 * return:
 *  0 if seekable device,
 *  1 if streamable device,
 *  -ERRNO if failure
 */
int is_boot_from_stream_device()
{
#if 0
	int ret;
	u32 interface;
	u32 boot;

	ret = get_bootrom_bootdev(&boot);
	if (ret < 0)
		return ret;

	interface = boot >> 16;

	if (interface >= BT_DEV_TYPE_USB)
		return 1;

	/* EMMC FASTBOOT MODE */
	if (interface == BT_DEV_TYPE_MMC && (boot & 1))
		return 1;

	return 0;
#else
	return 1;
#endif
}

void print_bootstage()
{
	u32 bstage;
	int ret = 0;
	ret = get_bootrom_bootstage(&bstage);
	if(ret)
		return;

	printf("Boot Stage: ");

	switch (bstage) {
	case BT_STAGE_PRIMARY:
		printf("Primary boot\n");
		break;
	case BT_STAGE_SECONDARY:
		printf("Secondary boot\n");
		break;
	case BT_STAGE_RECOVERY:
		printf("Recovery boot\n");
		break;
	case BT_STAGE_USB:
		printf("USB boot\n");
		break;
	default:
		printf("Unknow (0x%x)\n", bstage);
		break;
	}
}

void print_devinfo()
{
	u32 bootdev;
	u32 pagesize;
	u32 bootIface;
	u32 instance;
	u32 devState;
	int ret = 0;

	ret = get_bootrom_bootdev(&bootdev);
	if(ret)
		return;

	ret = get_sdp_pagesize(&pagesize);
	if(ret)
		return;

	bootIface = bootdev >> 16;
	bootIface &= 0xf;
	instance = bootdev >> 8;
	instance &= 0xf;
	devState = bootdev & 0xf;

	printf("BOOTDEV: ");

	switch(bootIface){
		case BT_DEV_TYPE_SD:
			printf("SD:%d\t", instance);
			break;
		case BT_DEV_TYPE_MMC:
			printf("EMMC:%d\t", instance);
			break;
		case BT_DEV_TYPE_NAND:
			printf("NAND:%d\t", instance);
			break;
		case BT_DEV_TYPE_FLEXSPINOR:
			printf("FLEXSPI-NOR:%d\t", instance);
			break;
		case BT_DEV_TYPE_SPI_NOR:
			printf("SPI_NOR:%d\t", instance);
			break;
		case BT_DEV_TYPE_FLEXSPINAND:
			printf("FLEXSPI-NAND:%d\t", instance);
			break;
		case BT_DEV_TYPE_USB:
			printf("USB:%d\t", instance);
			break;
		default:
			printf("UNKOWN\t");
			break;
	}

	printf("PAGESIZE: 0x%x\n", pagesize);
}

#ifdef CONFIG_SPL_BUILD
struct buffer_t {
	u8 buffer[PAGESIZE_USB];
	int r_size; /* remaining buffer size */
	int ptr_idx;
};

static struct buffer_t g_buffer;

#ifdef DEBUG
void debug_dump_mem(char *ptr, int size)
{
	int c;
	for(c = 0; c < size; c++){
		if (!(c % 16))
			printf("\n%08x", c);
		if(!(c % 4))
			puts(" ");

		printf("%02x", ptr[c]);
	}
	puts("\n\n");
}
#endif

static inline void seek_buffer(struct buffer_t *buffer, unsigned int seek)
{
	buffer->ptr_idx += seek;
	if(buffer->ptr_idx > PAGESIZE_USB )
		buffer->ptr_idx = PAGESIZE_USB;

	buffer->r_size = PAGESIZE_USB - buffer->ptr_idx;
}

static inline void reverse_buffer(struct buffer_t *buffer, unsigned int reverse)
{
	buffer->ptr_idx -= reverse;
	if(buffer->ptr_idx < 0)
		buffer->ptr_idx = 0;

	buffer->r_size = PAGESIZE_USB - buffer->ptr_idx;
}

static inline void set_buffer(struct buffer_t *buffer, u8 *ptr)
{
	buffer->ptr_idx = (int)(ptr - buffer->buffer);
	if(buffer->ptr_idx < 0)
		buffer->ptr_idx = 0;

	buffer->r_size = PAGESIZE_USB - buffer->ptr_idx;
}

static inline void reset_buffer(struct buffer_t *buffer)
{
	buffer->ptr_idx = 0;
	buffer->r_size = PAGESIZE_USB;
}

static u8 *sdp_dest = NULL;

static void fs_image_sdp_rx_data(u8 *data_buf, int data_len)
{
	if (sdp_dest)
		memcpy(sdp_dest, data_buf, data_len);
}

static const struct sdp_stream_ops fs_image_sdp_stream_ops = {
	.new_file = NULL,
	.rx_data = fs_image_sdp_rx_data,
};

static int sdp_download(u8 *dest, u32 offset, u32 size)
{
	int ret;

	sdp_dest = dest;

	debug("sdp_download: sdp_dest = 0x%lx\n",(ulong)sdp_dest);

	ret = spl_sdp_stream_single_rx(&fs_image_sdp_stream_ops, true);

	if(ret) {
		printf("SDP: Failed to download 0x%x bytes at offset 0x%x\n", size, offset);
		return -ENODATA;
	}
	return 0;
}

/**
 * Download new page, when buffer is fully read.
 * Return:
 *  remaining size in buffer
 *  -ERRNO when error
*/
static int sdp_download_page(struct buffer_t *buffer, unsigned int offset, unsigned int size)
{
	int ret;

	if(buffer->r_size > 0)
		return buffer->r_size;

	debug("download new page\n\n");

	ret = sdp_download(buffer->buffer, offset, size);
	if (ret)
		return ret;

	reset_buffer(buffer);

#if defined(DEBUG) && 0
	debug_dump_mem(buffer->buffer, PAGESIZE_USB);
#endif

	return buffer->r_size;
}

static u8 *search_fus_header(u8 *ptr, int size)
{
	int i = 0;
	u8 *hdr;
	for (i = 0; (i + 4) <= size; i += 4) {
		hdr = ptr + i;
		if (*(hdr + 0) == 'F' && *(hdr + 1) == 'S' &&
			*(hdr + 2) == 'L' && *(hdr + 3) == 'X')
                        return hdr;
	}

	return NULL;
}

/**
 * sdp_find_fshdr_stream()
 *
 * load next fshdr in SDP stream
 * @fsh: F&S Header that is found
 * @return: 0 if OK, -ERRNO if failure
 */
static int sdp_find_fshdr_stream(struct fs_header_v1_0 *fsh)
{
	int ret;
	int i;
	u8 *phdr = NULL;
	u8 *ptr = NULL;

	/* look within 256K for FSH */
	for (i = 0; i < 256; i++){
		ret = sdp_download_page(&g_buffer, 0, PAGESIZE_USB);
		if(ret < 0)
			return ret;

		ptr = g_buffer.buffer + g_buffer.ptr_idx;
		phdr = search_fus_header(ptr, g_buffer.r_size);

		if(phdr != NULL)
			break;

		seek_buffer(&g_buffer, g_buffer.r_size);
	}

	if(!phdr){
		printf("Can't find F&S Header in 256K range\n");
		return -ENODATA;
	}

	set_buffer(&g_buffer, phdr);

	memcpy(fsh, phdr, FSH_SIZE);
	seek_buffer(&g_buffer, FSH_SIZE);

	return 0;
}

ulong spl_romapi_read(u32 offset, u32 size, void *buf)
{
	u32 off_in_page;
	u32 aligned_size;
	u32 pagesize;
	int ret;
	u8 *tmp_buf;

	ret = get_sdp_pagesize(&pagesize);
	if(ret)
		return ret;

	off_in_page = offset % pagesize;
	aligned_size = ALIGN(size + off_in_page, pagesize);

	if (aligned_size != size) {
		tmp_buf = malloc(aligned_size);
		if (!tmp_buf) {
			printf("%s: Failed to malloc %u bytes\n", __func__, aligned_size);
			return 0;
		}

		if(sdp_download(tmp_buf, offset - off_in_page, aligned_size)) {
			free(tmp_buf);
			return 0;
		}

		memcpy(buf, tmp_buf + off_in_page, size);
		free(tmp_buf);
		return size;
	}

	if(sdp_download(buf, offset, size))
		return 0;

	return size;
}

/**
 * read method for spl_load_info
 */
static ulong sdp_rx_data_stream(struct spl_load_info *load, ulong sector,
	ulong count, void *buf)
{
	int ret, i;
	ulong buf_offset;
	u32 pagesize = 0;

	get_sdp_pagesize(&pagesize);

	for(i = 0; i < count; i++){
		buf_offset = load->bl_len * i;
		debug("read_pages=%d\tcount=%ld\n", i, count);
		debug("load_addr=0x%lx, ptr_idx=%d, r_size=%d\n", (ulong)(buf + buf_offset), g_buffer.ptr_idx, g_buffer.r_size);

		/* Copy FSH chunk */
		memcpy(buf + buf_offset, g_buffer.buffer + g_buffer.ptr_idx, load->bl_len);
		seek_buffer(&g_buffer, load->bl_len);

		/* Load buffer only when end of buffer is reached */
		if (g_buffer.r_size == 0) {
			ret = sdp_download_page(&g_buffer, 0, PAGESIZE_USB);
			if(ret < 0)
				break;
		}
	}

	return i;
}

/**
 * Load next F&S HEADER during stream
 */
int sdp_stream_continue(const struct sdp_stream_ops *stream_ops)
{
	struct fsh_load_info fsh_info;
	struct fs_header_v1_0 fsh;
	struct spl_load_info load_info;
	int ret;

	memset(&fsh, 0, sizeof(struct fs_header_v1_0));
	memset(&load_info, 0, sizeof(struct spl_load_info));

	ret = sdp_find_fshdr_stream(&fsh);
	if(ret){
		printf("Failed to find F&S Header: %d\n", ret);
		return ret;
	}

	load_info.bl_len = FSH_SIZE;
	load_info.read = sdp_rx_data_stream;

	fsh.type[15] = 0; /* just in case */
	debug("Found: %s at buffer idx=%d\n", fsh.type, g_buffer.ptr_idx);

	fsh_info.fsh = &fsh;
	fsh_info.load_info = &load_info;
	fsh_info.offset = 0;

	debug("%s: &fsh_info = 0x%lx\n",__func__,(ulong)&fsh_info);

	stream_ops->new_file((void *)&fsh_info, FSH_SIZE);
	return 0;
}
#endif
