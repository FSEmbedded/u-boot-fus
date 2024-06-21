/*
 * (C) Copyright 2024, F&S
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _FS_DEVICEINFO_COMMON_H
#define _FS_DEVICEINFO_COMMON_H

#include <common.h>
#include <linux/types.h>

/**
 * struct deviceinfo  - structure to hold device info data.
 *
 */

#define FSDI_STD_STRING_LEN		64
#define FSDI_ENETADDR_LEN		6
#define FSDI_ARRAY_LEN			1024

union fsdeviceinfo{

	u32 array[FSDI_ARRAY_LEN];

	struct{
		char boardname[FSDI_STD_STRING_LEN];
		uchar boardtype;
		unsigned int boardrevision;
		unsigned int boardfeatures;

		char nbootversion[FSDI_STD_STRING_LEN];
		char ubootversion[FSDI_STD_STRING_LEN];

		uchar enetaddr0[FSDI_ENETADDR_LEN];
		uchar enetaddr1[FSDI_ENETADDR_LEN];
	}info;
};

void fs_deviceinfo_prepare(void);
void fs_deviceinfo_assemble(void);

#endif /*_FS_DEVICEINFO_COMMON_H */
