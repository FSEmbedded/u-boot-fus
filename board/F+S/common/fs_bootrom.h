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

#include <spl.h>

#ifndef __FS_BOOTROM_H__
#define __FS_BOOTROM_H__

/**
 * is_boot_from_stream_device
 * 
 * return:
 *  0 if seekable device,
 *  1 if streamable device,
 *  -ERRNO if failure
*/
int is_boot_from_stream_device(void);

ulong bootrom_rx_data_stream(struct spl_load_info *load, ulong sector,
	ulong count, void *buf);

int bootrom_stream_continue(const struct sdp_stream_ops *stream_ops);
int get_bootrom_bootstage(u32 *bstage);

#endif /* __FS_BOOTROM_H__ */