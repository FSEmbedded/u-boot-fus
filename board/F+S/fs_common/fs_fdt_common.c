/*
 * (C) Copyright 2023
 * F&S Elektronik Systeme GmbH
 *
 * Common code used on Layerscape Boards
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <fdt_support.h>
#include "fs_fdt_common.h"

const char *fs_fdt_get_label_namelen(const void *fdt,
				  const char *name, int namelen)
{
	int aliasoffset;

	aliasoffset = fdt_path_offset(fdt, "/__symbols__");
	if (aliasoffset < 0){
		return NULL;
	}

	return fdt_getprop_namelen(fdt, aliasoffset, name, namelen, NULL);
}

const char *fs_fdt_get_label(const void *fdt, const char *name)
{
	return fs_fdt_get_label_namelen(fdt, name, strlen(name));
}

int fs_fdt_enable_node_by_path(void* blob, const char* path, bool value)
{
     char *str = value ? "okay" : "disabled";
     const void * val;
     int offset, ret, len;

     offset = fdt_path_offset((const void*)blob,path);
	if (offset < 0)
		return offset;

	/* Do not change if status already exists and has this value */
	val = fdt_getprop(blob, offset, "status", &len);
	if (val && len && !strcmp(val, str))
		return 0;

	/* Now, set new value */
	ret = fdt_setprop_string(blob, offset, "status", str);
	if (ret) {
		printf("## Can not set status of node %s: err=%s\n",
		       path, fdt_strerror(ret));
		return ret;
	}

	return  0;
}

int fs_fdt_enable_node_by_label(void* blob, const char* label, bool value)
{
     const char * path;
     path = fs_fdt_get_label(blob,label);
     if (path == NULL){
		printf("Path for %s not found!\n", label);
        return -FDT_ERR_NOTFOUND;
	 }

     return fs_fdt_enable_node_by_path(blob, path, value);
}
