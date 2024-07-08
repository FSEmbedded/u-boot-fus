// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright 2024 F&S Elektronik Systeme GmbH
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

#include <common.h>
#include <errno.h>
#include <asm/arch/sys_proto.h>
#include <sdp.h>
#include "fs_bootrom.h"
#include "fs_image_common.h"

#define PAGESIZE_USB 0x400
struct buffer_t {
	u8 buffer[PAGESIZE_USB];
	int r_size; /* remaining buffer size */
	int ptr_idx;
};

static struct buffer_t g_buffer;

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

static int bootrom_download(u8 *dest, u32 offset, u32 size);
static inline void align_buffer(struct buffer_t *buffer)
{
	u8 *buf = buffer->buffer;
	if (buffer->ptr_idx == 0 || buffer->r_size == 0)
		return;

	debug("align page\n");

	memcpy(buf, buf + buffer->ptr_idx, buffer->r_size);
	bootrom_download(buf + buffer->r_size, 0, buffer->ptr_idx);
	reset_buffer(buffer);
}

static int get_bootrom_bootdev(u32 *boot)
{
	int ret;
	ret = rom_api_query_boot_infor(QUERY_BT_DEV, boot);
	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at query_boot_info\n");
		return -ENODEV;
	}

	return 0;
}

int is_boot_from_stream_device()
{
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
}

int get_bootrom_bootstage(u32 *bstage)
{
	int ret = 0;
	ret |= rom_api_query_boot_infor(QUERY_BT_STAGE, bstage);

	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at query_boot_info\n");
		return -ENODEV;
	}
	return 0;
}

static int bootrom_download(u8 *dest, u32 offset, u32 size)
{
	int ret;

	ret = rom_api_download_image(dest, offset, size);
	if(ret != ROM_API_OKAY) {
		printf("ROMAPI: Failed to download 0x%x bytes\n", size);
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
static int bootrom_download_page(struct buffer_t *buffer, unsigned int offset, unsigned int size)
{
	int ret;

	if(buffer->r_size > 0)
		return buffer->r_size;

	debug("download new page\n\n");

	ret = bootrom_download(buffer->buffer, offset, size);
	if (ret)
		return ret;

	reset_buffer(buffer);

#if defined(DEBUG) && 0
	{
		int c;
		char * ptr = (char *)buffer->buffer;
		for(c = 0; c < 0x400; c++){
			if (!(c % 16))
				puts("\n");
			if(!(c % 4))
				puts(" ");

			debug("%02x", ptr[c]);
		}
	}
	puts("\n\n");
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
 * bootrom_find_fshdr_stream()
 *
 * load next fshdr in SDPS stream
 * @fsh: F&S Header that is found
 * @return: 0 if OK, -ERRNO if failure
 */
static int bootrom_find_fshdr_stream(struct fs_header_v1_0 *fsh)
{
	int ret;
	int i;
	u8 *phdr = NULL;
	u8 *ptr = NULL;

	/* look within 64K for FSH */
	for (i = 0; i < 64; i++){
		ret = bootrom_download_page(&g_buffer, 0, PAGESIZE_USB);
		if(ret < 0)
			return ret;

		ptr = g_buffer.buffer + g_buffer.ptr_idx;
		phdr = search_fus_header(ptr, g_buffer.r_size);

		if(phdr != NULL)
			break;

		seek_buffer(&g_buffer, g_buffer.r_size);
	}

	if(!phdr){
		printf("Can't find F&S Header in 64K range\n");
		return -ENODATA;
	}

	set_buffer(&g_buffer, phdr);

	/** 
	 * NOTE: Stream seems to be not aligned?!
	 * This should never happen.
	 */
	if(g_buffer.r_size < FSH_SIZE){
		seek_buffer(&g_buffer, g_buffer.r_size);
		return -ENODATA;
	}

	memcpy(fsh, phdr, FSH_SIZE);
	seek_buffer(&g_buffer, FSH_SIZE);

	return 0;
}

/**
 * read method for spl_load_info
 */
ulong bootrom_rx_data_stream(struct spl_load_info *load, ulong sector,
	ulong count, void *buf)
{
	int ret, i;
	ulong buf_offset;

	for(i = 0; i < count; i++){
		buf_offset = PAGESIZE_USB * i;
		align_buffer(&g_buffer);
		debug("read_pages=%d\tcount=%ld\n", i, count);
		debug("load_addr=0x%p, ptr_idx=%d, r_size=%d\n", buf + buf_offset, g_buffer.ptr_idx, g_buffer.r_size);
		ret = bootrom_download_page(&g_buffer, 0, PAGESIZE_USB);
		if(ret < 0)
			break;

		memcpy(buf + buf_offset, g_buffer.buffer, g_buffer.r_size);
		seek_buffer(&g_buffer, g_buffer.r_size);
	}

	return i;
}

int bootrom_find_next_fsh(struct fs_header_v1_0 *fsh)
{
	int ret;
	ret = bootrom_find_fshdr_stream(fsh);
	if(ret){
		printf("Failed to find F&S Header: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * Load next F&S HEADER during stream
 */
int bootrom_stream_continue(const struct sdp_stream_ops *stream_ops)
{
	struct fs_header_v1_0 fsh;
	int ret;

	memset(&fsh, 0, sizeof(struct fs_header_v1_0));
	ret = bootrom_find_next_fsh(&fsh);
	if(ret)
		return ret;

	fsh.type[15] = 0;
	debug("Found: %s at buffer idx=%d\n", fsh.type, g_buffer.ptr_idx);

	stream_ops->new_file((void *)&fsh, FSH_SIZE);
	return 0;
}