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

#include <sdp.h>
#include <asm/arch/sys_proto.h>

#ifndef __FS_BOOTROM_H__
#define __FS_BOOTROM_H__

#define PAGESIZE_USB 0x400

#ifdef CONFIG_SPL_BUILD
int bootrom_stream_continue(const struct sdp_stream_ops *stream_ops);
int bootrom_seek_continue(const struct sdp_stream_ops *stream_ops);
#endif

int is_boot_from_stream_device(void);
int get_bootrom_bootdev(u32 *bdev);
int get_bootrom_bootstage(u32 *bstage);
int get_bootrom_bootimg_offset(u32 *offset);
int get_bootrom_offset(u32 *offset);
int get_bootrom_pagesize(u32 *pagesize);
void print_bootstage(void);
void print_devinfo(void);

#endif /* __FS_BOOTROM_H__ */