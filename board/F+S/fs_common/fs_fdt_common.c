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
	int ofnode;
	const void *prop;

	ofnode = fs_fdt_get_ofnode_by_path(fdt, "/__symbols__");
	if (ofnode < 0){
		return NULL;
	}

	prop = fdt_getprop_namelen(fdt, ofnode, name, namelen, NULL);
	if (prop == NULL){
		printf("WARNING: Symbol %s: %s\n", name,
				fdt_strerror(-FDT_ERR_NOTFOUND));
	}
	return prop;
}

int fs_fdt_get_ofnode_by_path(const void *blob, const char *path){
	int ofnode;

	ofnode = fdt_path_offset(blob, path);
	if (ofnode < 0){
		printf("ofnode for %s not found!: %s\n", path, fdt_strerror(ofnode));
	}
	return ofnode;
}

const char *fs_fdt_get_label(const void *fdt, const char *name)
{
	return fs_fdt_get_label_namelen(fdt, name, strlen(name));
}

int fs_fdt_enable_node_by_path(void* blob, const char* path, bool value)
{
	char *str = value ? "okay" : "disabled";
	const void * val;
	int ofnode, ret, len;

	ofnode = fs_fdt_get_ofnode_by_path((const void*)blob,path);
	if (ofnode < 0){
		return ofnode;
	}

	/* Do not change if status already exists and has this value */
	val = fdt_getprop(blob, ofnode, "status", &len);
	if (val && len && !strcmp(val, str))
		return 0;

	/* Now, set new value */
	ret = fdt_setprop_string(blob, ofnode, "status", str);
	if (ret) {
		printf("Can not set status of node %s: %s\n",
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
		return -FDT_ERR_NOTFOUND;
	}

	return fs_fdt_enable_node_by_path(blob, path, value);
}

int fs_fdt_setprop_by_label(void* blob, const char* label, const char* property, const void *value, int len){
	const char* path;

    path = fs_fdt_get_label(blob, label);
	if (path == NULL){
	    return -FDT_ERR_NOTFOUND;
	}

	return fs_fdt_setprop_by_path(blob, path, property, value, len);
}

int fs_fdt_setprop_by_path(void *blob, const char *path, const char *property, const void *value, int len){
	int ofnode;

	ofnode = fdt_path_offset(blob, path);
	if(ofnode < 0){
		return ofnode;
	}

	return fdt_setprop(blob, ofnode, property, value, len);
}
