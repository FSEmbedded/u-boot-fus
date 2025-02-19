// SPDX-License-Identifier: GPL-2.0+
/*
 * deviceinfo.c
 *
 * F&S device info file access
 *
 * 2024-04-29 - zutter@fs-net.de - base version
 * 2024-06-18 - mueller@fs-net.de - stripped version
 * 2024-12-10 - friedrich@fs-net.de - port to fsimx93
 */
#include <common.h>
#include <blk.h>
#include <fastboot.h>
#include <image.h>
#include <malloc.h>
#include <part.h>
#include <net.h>
#include <fs.h>
#include <fat.h>
#include <cli.h>
#include <env.h>			/* env_get() */
#include <u-boot/crc.h>			/* crc32() */
#include <version.h>

#include "fs_deviceinfo_common.h"
#include "fs_board_common.h"

static union fsdeviceinfo* pfsdi;
static char boardname[FSDI_STD_STRING_LEN];
static unsigned int displayinterface = -1;
static unsigned int displaypanel = -1;

void fs_deviceinfo_dump(union fsdeviceinfo* pfsdi)
{
	printf("\n-----FS_DeviceInfo-----\n");	
	printf("Board name:    %s\n", pfsdi->info.boardname);
	printf("Board type:    %d\n", pfsdi->info.boardtype);
	printf("Board rev.:    %d\n", pfsdi->info.boardrevision);
	printf("Board feat.:   0x%04x\n", pfsdi->info.boardfeatures);
	printf("UBoot:         %s\n", pfsdi->info.ubootversion);
	printf("NBoot:         %s\n", pfsdi->info.nbootversion);
	printf("Enet addr 0:   %02x %02x %02x %02x %02x %02x\n",
	pfsdi->info.enetaddr0[0], pfsdi->info.enetaddr0[1],
	pfsdi->info.enetaddr0[2], pfsdi->info.enetaddr0[3],
	pfsdi->info.enetaddr0[4], pfsdi->info.enetaddr0[5]);
	printf("Enet addr 1:   %02x %02x %02x %02x %02x %02x\n",
	pfsdi->info.enetaddr1[0], pfsdi->info.enetaddr1[1],
	pfsdi->info.enetaddr1[2], pfsdi->info.enetaddr1[3],
	pfsdi->info.enetaddr1[4], pfsdi->info.enetaddr1[5]);
	printf("**Display**\n");
	printf("Interface:     %d\n", pfsdi->info.displayinterface);
	printf("Panel:         %d\n", pfsdi->info.displaypanel);
	printf("-----FS_DeviceInfo-----\n");
}

void fs_deviceinfo_setcrc32(union fsdeviceinfo* pfsdi, u32 crc32)
{
	pfsdi->array[FSDI_ARRAY_LEN-1] = crc32;
}

u32 fs_deviceinfo_calccrc32(union fsdeviceinfo* pfsdi)
{
	return crc32(0, (unsigned char*)pfsdi, sizeof(union fsdeviceinfo)-sizeof(u32));
}

void fs_deviceinfo_prepare(void)
{
	pfsdi = (union fsdeviceinfo*) CFG_FS_DEVICEINFO_ADDR;

	// Prepare Board name
	const char* bn = get_board_name();
	if(bn)
		memcpy(boardname, bn, strlen(bn));

	char* di = env_get("displayinterface");
	if (di)
		displayinterface = simple_strtoul(di, NULL, 10);
	char* dp = env_get("displaypanel");
	if (dp)
		displaypanel = simple_strtoul(dp, NULL, 10);
}

void fs_deviceinfo_assemble(void)
{
	int id = 0;

	// Clear device info
	memset(pfsdi, 0, sizeof(union fsdeviceinfo));

	// Board name
	memcpy(pfsdi->info.boardname, boardname, strlen(boardname));

	// Board type
	pfsdi->info.boardtype = fs_board_get_type();

	// Board revision
	pfsdi->info.boardrevision = fs_board_get_rev();

	// Board features
	pfsdi->info.boardfeatures = fs_board_get_features();

	// NBoot version
	const char* nbootversion = fs_board_get_nboot_version();
	memcpy(pfsdi->info.nbootversion, nbootversion, strlen(nbootversion));

	// UBoot version
	memcpy(pfsdi->info.ubootversion, U_BOOT_VERSION, strlen(U_BOOT_VERSION));

	// Enet address
	eth_env_get_enetaddr_by_index("eth", id++, pfsdi->info.enetaddr0);
	eth_env_get_enetaddr_by_index("eth", id++, pfsdi->info.enetaddr1);

	// Get display configuration
	pfsdi->info.displayinterface = displayinterface;
	pfsdi->info.displaypanel = displaypanel;

	// CRC
	u32 calc = fs_deviceinfo_calccrc32(pfsdi);
	fs_deviceinfo_setcrc32(pfsdi, calc);

#ifdef DEBUG
	// Debug Output
	fs_deviceinfo_dump(pfsdi);
#endif
}
