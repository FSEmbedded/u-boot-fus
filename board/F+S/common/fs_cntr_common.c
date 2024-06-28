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

#include <spl.h>
#include <malloc.h>
#include <sdp.h>
#include <asm/sections.h>
#include <hang.h>
#include <asm/mach-imx/image.h>

#ifdef CONFIG_AHAB_BOOT
#include <asm/mach-imx/ahab.h>
#endif

#include "fs_dram_common.h"
#include "fs_image_common.h"
#include "fs_cntr_common.h"
#include "fs_bootrom.h"

/* Structure to handle board name and revision separately */
struct bnr {
	char name[MAX_DESCR_LEN];
	unsigned int rev;
};

// static struct bnr compare_bnr;		/* Used for BOARD-ID comparisons */
// static char board_id[MAX_DESCR_LEN + 1]; /* Current board-id */

struct env_info {
	unsigned int start[2];
	unsigned int size;
};

struct ram_info_t {
	const char *type;
	const char *timing;
};

#if defined(CONFIG_SPL_BUILD)
/* Jobs to do when streaming image data */
#define FSIMG_JOB_BOARD_ID BIT(0)
#define FSIMG_JOB_BOARD_CFG BIT(1)
#define FSIMG_JOB_DRAM BIT(2)
#define FSIMG_FW_JOBS (FSIMG_JOB_DRAM)

static unsigned int jobs;
// static unsigned int mmc_hw_part;
// static unsigned int mmc_offset;

static unsigned int get_jobs(void);
#endif

/*-------------- Adapted functions from parse-container.c--------------------*/

/**
 * free imx container
 * @param cntr_info: ptr to image_info for imx_container
 */
static void free_container(struct spl_image_info *cntr_info)
{
#if defined(CONFIG_AHAB_BOOT)
	if (!strcmp(cntr_info->name, AHAB_CNTR_NAME)){
		ahab_auth_release();
		return;
	}
#endif

	free((struct container_hdr *)cntr_info->load_addr);
}

/**
 * reads a imx container hdr
 * this function will initialize ahab.
 * After successful run, 'free_container()' must be called.
 * In case of an unsuccessful run, no need to call 'free_container()'
 * 
 * @param spl_image: struct containes image infos like loadaddr, size, ...
 * @param spl_load_info: struct contains infos about device, like blocksize, read(), ...
 * @param sector: start reading at sector.
 * @returns -ERRNO
 * 
 */
static int read_container_hdr(struct spl_image_info *spl_image,
			       struct spl_load_info *info, ulong sector)
{
	struct container_hdr *cntr = NULL;
	struct container_hdr *authhdr = NULL;
	u16 length;
	u32 count;
	int size, ret = 0;

	size = roundup(CONTAINER_HDR_ALIGNMENT, info->bl_len);
	count = size / info->bl_len;

	cntr = malloc(size);

	if (!cntr)
		return -ENOMEM;

	debug("%s: container: 0x%p sector: %lu count: %u\n", __func__,
	      cntr, sector, count);

	if (info->read(info, sector, count, cntr) != count) {
		ret = -EIO;
		goto free_cntr;
	}

	if (cntr->tag != 0x87 || cntr->version != 0x0) {
		printf("Wrong container header\n");
		ret = -ENOENT;
		goto free_cntr;
	}

	debug("TEST: cntr->num_images=%d\n", cntr->num_images);
	if (!cntr->num_images) {
		printf("Can not find any images in Container\n");
		ret = -ENOENT;
		goto free_cntr;
	}

	length = cntr->length_lsb + (cntr->length_msb << 8);
	debug("Container length 0x%x\n", length);

	if (length > CONTAINER_HDR_ALIGNMENT) {
		struct container_hdr *cntr_tmp = NULL;
		int tmp_size;
		u32 tmp_count;

		tmp_size = roundup(length, info->bl_len);
		tmp_count = tmp_size / info->bl_len;

		cntr_tmp = malloc(tmp_size);
		if (!cntr_tmp) {
			ret = -ENOMEM;
			goto free_cntr;
		}

		memcpy(cntr_tmp, cntr, size);
		free(cntr);

		debug("%s: container: 0x%p sector: %lu count: %u\n",
		      __func__, cntr, sector, tmp_count);

		if (info->read(info, (sector + count), (tmp_count - count), cntr_tmp) !=
					(tmp_count - count)) {
			ret = -EIO;
			goto free_cntr;
		}

		cntr = cntr_tmp;
		size = tmp_size;
		count = tmp_count;
	}

	spl_image->size = size;
	spl_image->offset = sector;

#if defined(CONFIG_AHAB_BOOT)
	spl_image->name = AHAB_CNTR_NAME;
	authhdr = ahab_auth_cntr_hdr(cntr, length);
	free(cntr);
	if (!authhdr) {
		ahab_auth_release();
		return -ENOEXEC;
	}
#else
	spl_image->name = IMX_CNTR_NAME;
	authhdr = cntr;
#endif

	spl_image->load_addr = (uintptr_t)authhdr;

	/**
	 * in case of streaming,
	 * padding between hdr and Image needs to be considered
	 */
	if (is_boot_from_stream_device()) {
		void *padding;
		int pad_size;
		int pad_count;
		struct boot_img_t *images;

		padding = malloc(info->bl_len);
		if (!padding) {
			ret = -ENOMEM;
			goto free_cntr;
		}

		images = (struct boot_img_t *)((u8 *)cntr +
				       sizeof(struct container_hdr));

		debug("1st img offset=0x%x\n", images[0].offset);
		pad_size = images[0].offset - size;
		pad_size = roundup(pad_size, info->bl_len);
		pad_count = pad_size / info->bl_len;
		debug("load padding with count=%d\n", pad_count);

		for (;pad_count > 0; pad_count--) {
			info->read(info, 0, 1, padding);
		}

		free(padding);
	}

	return ret;

	free_cntr:
	spl_image->load_addr = (uintptr_t)cntr;
	free_container(spl_image);
	return ret;
}

int fs_cntr_load_imx_container_header(struct spl_image_info *image_info, struct spl_load_info *info, ulong sector)
{
	return read_container_hdr(image_info, info, sector);
}

/**
 * read_auth_image based on arch/arm/mach-imx/parse-container.c
*/
static struct boot_img_t *read_auth_image(struct spl_image_info *spl_image,
					  struct spl_load_info *info,
					  struct container_hdr *container,
					  int image_index,
					  u32 cntr_sector)
{
	struct boot_img_t *images;
	ulong sector;
	u32 count;

	if (image_index >= container->num_images) {
		debug("Invalid image number\n");
		return NULL;
	}

	images = (struct boot_img_t *)((u8 *)container +
				       sizeof(struct container_hdr));

	if (images[image_index].offset % info->bl_len) {
		printf("%s: image%d offset not aligned to %u\n",
		       __func__, image_index, info->bl_len);
		return NULL;
	}

	count = roundup(images[image_index].size, info->bl_len) /
		info->bl_len;
	sector = images[image_index].offset / info->bl_len +
		cntr_sector;

	debug("%s: container: 0x%p sector: %lu sectors: %u\n", __func__,
	      container, sector, count);
	if (info->read(info, sector, count,
		       (void *)images[image_index].dst) != count) {
		printf("%s: failed to load image %d\n", __func__, image_index);
		return NULL;
	}

#ifdef CONFIG_AHAB_BOOT
	if (ahab_verify_cntr_image(&images[image_index], image_index))
		return NULL;
#endif
	return &images[image_index];
}

/* load a specific image from container */
static int fs_cntr_load_single_image(struct spl_image_info *image_info,
				struct spl_image_info *cntr_info,
				struct spl_load_info *load,
				int image_num)
{
	struct boot_img_t *image;

	image = read_auth_image(image_info, load, (struct container_hdr *)cntr_info->load_addr,
					image_num, cntr_info->offset);

	if (!image)
		return -EINVAL;

	image_info->load_addr = image->dst;
	image_info->entry_point = image->entry;
	image_info->size = image->size;

	return 0;
}

/* ------------- Functions only in SPL, not U-Boot ------------------------- */
#if defined(CONFIG_SPL_BUILD)

extern void fs_image_set_board_id(void);

static unsigned int get_jobs()
{
	return jobs;
}

static void set_jobs(unsigned int j)
{
	jobs = j;
}

enum fsimg_state {
	FSIMG_STATE_BOARD_ID,
	FSIMG_STATE_BOARD_CFG,
	FSIMG_STATE_DRAM,
	FSIMG_STATE_DONE,
};

__weak int fs_board_basic_init(void)
{
	debug("%s: no init\n", __func__);
	return 0;
}

static int fs_handle_board_id(struct fs_header_v1_0 *fsh)
{
	/* State and file does not match */
	if (!fs_image_match(fsh, "BOARD-ID", NULL)){
		debug("F&S HDR is not type BOARD-ID. Got %s", fsh->type);
		return -EINVAL;
	}

	debug("BOARD-ID is %s\n", fsh->param.descr);

	/* Save ID and add job to load BOARD-CFG */
	fs_image_set_compare_id(fsh->param.descr);
	fs_image_set_board_id();

	return 0;
}

/**
 * init ram_info struct with values from board-cfg
 * retrun: 0 if ok; .-1 if failure;
*/
static int init_ram_info(struct ram_info_t *ram_info)
{
	void *fdt = fs_image_get_cfg_fdt();
	int offs = fs_image_get_board_cfg_offs(fdt);
	int rev_offs;

	memset(ram_info, 0, sizeof(struct ram_info_t));

	rev_offs = fs_image_get_board_rev_subnode(fdt, offs);
	ram_info->type = fs_image_getprop(fdt, offs, rev_offs, "dram-type", NULL);
	ram_info->timing = fs_image_getprop(fdt, offs, rev_offs, "dram-timing", NULL);

	if(!ram_info->type[0] || !ram_info->timing[0])
		return -1;

	return 0;
}

static int fs_load_cntr_board_cfg(struct fs_header_v1_0 *fsh)
{
	struct spl_load_info load_info;
	struct spl_image_info cntr_info;
	struct spl_image_info cfg_info;
	struct container_hdr *cntr;
	struct fs_header_v1_0 *cfg_fsh;

	int sector = 0;
	int ret = 0;
	int idx;
	int num_imgs;

	memset(&load_info, 0, sizeof(struct spl_load_info));

	if(is_boot_from_stream_device()){
		load_info.bl_len = 0x400;
		load_info.read = bootrom_rx_data_stream;
	} else {
	 	/* TODO: MMC INFO */
	 	// load_info.bl_len = MMC_BLKSIZE;
	 	// load_info.read = MMC_READ_FUNC;
	}

	ret = fs_cntr_load_imx_container_header(&cntr_info, &load_info, sector);
	if(ret)
		return ret;

	debug("Found IMX-CONTAINER FOR BOARD_CFG\n");
	cntr = (struct container_hdr *)cntr_info.load_addr;
	num_imgs = cntr->num_images;

	for (idx = 0; idx < num_imgs; idx++){
		ret = fs_cntr_load_single_image(&cfg_info,
					&cntr_info,
					&load_info,
					idx);
		if(ret)
			continue;

		cfg_fsh = (struct fs_header_v1_0 *)cfg_info.load_addr;

		if(fs_image_match_board_id(cfg_fsh))
			break;

		/* when no img found */
		ret = -EINVAL;
	}

	if(!ret)
		debug("FSIMG: FOUND %s (%s)\n", cfg_fsh->type, cfg_fsh->param.descr);

	free_container(&cntr_info);

	return ret;
}

static int fs_handle_board_cfg(struct fs_header_v1_0 *fsh, struct ram_info_t *ram_info)
{
	int ret;

	/* State and file does not match */
	if (!fs_image_match(fsh, "BOARD-INFO", NULL)){
		debug("F&S HDR is not type BOARD-INFO. Got %s\n", fsh->type);
		return -EINVAL;
	}

	ret = fs_load_cntr_board_cfg(fsh);
	if(ret) {
		debug("FSIMG: board_cfg not found!: %d\n", ret);
		return -EINVAL;
	}

	ret = init_ram_info(ram_info);
	if(ret) {
		debug("FSIMG: dram definition not in board_cfg");
		return -EINVAL;
	}

	return 0;
}

static int fs_load_cntr_dram_info(struct fs_header_v1_0 *fsh, struct ram_info_t *ram_info)
{
	struct spl_load_info load_info;
	struct spl_image_info cntr_info;
	struct spl_image_info dram_info;
	struct container_hdr *cntr;
	struct fs_header_v1_0 *dram_fsh;

	int sector = 0;
	int ret = 0;
	int idx;
	int num_imgs;

	memset(&load_info, 0, sizeof(struct spl_load_info));

	/* TODO: maybe in a func?
	 * HOW to consider second src?
	 */
	if(is_boot_from_stream_device()){
		load_info.bl_len = 0x400;
		load_info.read = bootrom_rx_data_stream;
	} else {
		/* TODO: MMC INFO */
		// load_info.bl_len = MMC_BLKSIZE;
		// load_info.read = MMC_READ_FUNC;
	}

	ret = fs_cntr_load_imx_container_header(&cntr_info, &load_info, sector);
	if(ret)
		return ret;

	cntr = (struct container_hdr *)cntr_info.load_addr;
	num_imgs = cntr->num_images;

	ret = fs_cntr_load_single_image(&dram_info, &cntr_info, &load_info, 0);
	if(ret){
		free_container(&cntr_info);
		return ret;
	}

	dram_fsh = (struct fs_header_v1_0 *)dram_info.load_addr;
	if(!fs_image_match(dram_fsh, "DRAM-FW", ram_info->type)) {
		free_container(&cntr_info);
		return -EINVAL;
	}
	debug("FSIMG: FOUND %s (%s)\n", dram_fsh->type, dram_fsh->param.descr);

	memcpy(&_end, (void *)(dram_info.load_addr + FSH_SIZE), dram_info.size);

	for (idx = 1; idx < num_imgs; idx++) {
		ret = fs_cntr_load_single_image(&dram_info, 
					&cntr_info,
					&load_info,
					idx);
		if(ret)
			continue;

		dram_fsh = (struct fs_header_v1_0 *)dram_info.load_addr;
		debug("FSIMG: FOUND %s(%s)\n", dram_fsh->type, dram_fsh->param.descr);
		if(fs_image_match(dram_fsh, "DRAM-TIMING", ram_info->timing))
			break;

		/* when no img found */
		ret = -EINVAL;
	}

	if(!ret){
		fs_board_init_dram_data((void *)(dram_info.load_addr + FSH_SIZE));
	}

	free_container(&cntr_info);

	return ret;
}

/**
 *  fs_handle_dram()
 * @fsh: fsh, which should announce a dram-info
 * return: 0 if type matches; -ERRNO if type does not match
 */
static int fs_handle_dram(struct fs_header_v1_0 *fsh, struct ram_info_t *ram_info)
{
	int ret = 0;

	/* State and file does not match */
	if (!fs_image_match(fsh, "DRAM-INFO", NULL)){
		debug("F&S HDR is not type DRAM-INFO.\n");
		return -EINVAL;
	}

	ret = fs_load_cntr_dram_info(fsh, ram_info);
	if(ret) {
		debug("%s: data not found!: %d", __func__, ret);
		return ret;
	}

	return ret;
}

/* State machine: Use F&S HDR as transition and handle state */
static void fs_cntr_handle(struct fs_header_v1_0 *fsh)
{
	static enum fsimg_state state = FSIMG_STATE_BOARD_ID;

	// const char *arch = fs_image_get_arch();
	static struct ram_info_t ram_info;

	enum fsimg_state next;
	unsigned int jobs = get_jobs();

	next = state;

	switch (state) {
	case FSIMG_STATE_BOARD_ID:
		if (fs_handle_board_id(fsh)){
			next = FSIMG_STATE_DONE;
			break;
		}
		jobs &= ~FSIMG_JOB_BOARD_ID;
		next = FSIMG_STATE_BOARD_CFG;
		break;
	case FSIMG_STATE_BOARD_CFG:
		if(fs_handle_board_cfg(fsh, &ram_info)){
			next = FSIMG_STATE_DONE;
			break;
		}
		jobs &= ~FSIMG_JOB_BOARD_CFG;
		next = FSIMG_STATE_DRAM;
		break;
	case FSIMG_STATE_DRAM:
		if(fs_handle_dram(fsh, &ram_info))
			break;
		jobs &= ~FSIMG_JOB_DRAM;
		next = FSIMG_STATE_DONE;
		break;
	default:
		debug("Current State %d is not considered\n", state);
		printf("FSIMG: FSM violation\n");
 		hang();
		break;
	}
	state = next;
	set_jobs(jobs);
}

/**
 * fs_cntr_new_header
 */
static void fs_cntr_new_header(void *dnl_address, int size)
{
	struct fs_header_v1_0 *fsh;

	if(size != FSH_SIZE)
		return;

	fsh = (struct fs_header_v1_0 *)dnl_address;

	fs_cntr_handle(fsh);
}

static const struct sdp_stream_ops fs_image_sdp_stream_ops = {
	.new_file = fs_cntr_new_header, /* handels F&S ID and Info-Header */
	.rx_data = NULL,
};

/* Load FIRMWARE and optionally BOARD-CFG via SDPS from BOOTROM */
void fs_cntr_all_stream(bool need_cfg)
{
	unsigned int jobs_todo = FSIMG_FW_JOBS;

	if (need_cfg){
		jobs_todo |= FSIMG_JOB_BOARD_CFG;
		jobs_todo |= FSIMG_JOB_BOARD_ID;
	}

	set_jobs(jobs_todo);

	// basic_init_callback = basic_init;

	/* Stream until NBOOT Files are downloaded */
	while (get_jobs()) {
		debug("Jobs not done: 0x%x\n", jobs);
		bootrom_stream_continue(&fs_image_sdp_stream_ops);
	};
}

void fs_cntr_init(bool need_cfg)
{
	if(is_boot_from_stream_device()){
		debug("FSIMG: BOOT FROM STREAMDEV\n");
		fs_cntr_all_stream(need_cfg);
		return;
	}

	debug("FSIMG: BOOT FROM BLOCK DEVICE\n");
	// fs_cntr_all_mmc(need_cfg);
}

#endif /* CONFIG_SPL_BUILD */
/* ------------------------------------------------------------------------- */
