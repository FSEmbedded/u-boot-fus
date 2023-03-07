/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * Common ETH code used on GAL boards
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FS_ETH_COMMON_H__
#define __FS_ETH_COMMON_H__

void fs_set_macaddrs(void);

/**
 * @brief set fused_mac property in Ethernet-Port Node
 * 
 * @param blob 
 * @return int 0 = Success; <0 = Error
 */
int fs_add_mac_prop(void *blob, const char *lable);

#endif // __FS_ETH_COMMON_H__