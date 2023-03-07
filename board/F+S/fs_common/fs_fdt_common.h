/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * Common code for FDT-Editing
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FS_FDT_COMMON__
#define __FS_FDT_COMMON__


/**
 * @brief fdt_get_label_namelen - retrieve the path referenced by a given label/symbol
 * 
 * @param fdt pointer to the device tree blob
 * @param name name of the label/symbol to look up
 * @param namelen number of characters of name to consider
 * @return const char* 
 */
const char *fs_fdt_get_label_namelen(const void *fdt, const char *name, int namelen);

/**
 * fs_fdt_get_label - retrieve the path referenced by a given alias
 * @param fdt: pointer to the device tree blob
 * @param name: name of the alias th look up
 *
 * fs_fdt_get_label() retrieves the value of a given label.  That is, the
 * value of the property named 'name' in the node /__symbols__.
 *
 * returns:
 *	a pointer to the expansion of the label named 'name', if it exists
 *	NULL, if the given label or the /aliases node does not exist
 */
const char *fs_fdt_get_label(const void *fdt, const char *name);

/**
 * @brief Disable Device-Tree Node with given label/symbol
 * 
 * @param blob pointer to the device tree blob
 * @param label name of the label/symbol to look up
 * @param value 0 = Disable; 1 = Enable
 * @return int (0=success <0=error)
 */
int fs_fdt_enable_node_by_label(void* blob, const char* label, bool value);

/**
 * @brief Disable Device-Tree Node with given path
 * 
 * @param blob pointer to the device tree blob
 * @param label path to look up
 */
int fs_fdt_enable_node_by_path(void* blob, const char* path, bool value);

#endif //__FS_FDT_COMMON__
