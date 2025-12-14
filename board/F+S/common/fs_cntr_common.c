// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 F&S Elektronik Systeme GmbH
 *
 * Format of an NBoot image
 * ------------------------
 *
 * NBoot on one side has to use the i.MX container format to be loadable by
 * the ROM Loader of the CPU. On the other hand, F&S tries to bring in more
 * info so that the content can easily be listed and even be extracted with
 * the fsimage.sh tool. This is basically done by prepending an F&S header to
 * each single image. The F&S header shows the type of the image, a specific
 * image description, and it can also hold a CRC32 checksum to enhance image
 * robustness in the nonsigned case.
 *
 * So an NBoot image has actually two different views:
 *
 * 1. The F&S view starting with the BOOT-INFO header, consisting of a set of
 *    at least five images: BOOT-INFO, BOOT-ID, BOARD-INFO, at least one
 *    DRAM-TYPE and EXTRA. EXTRA holds the scripts that can be used to handle
 *    F&S images: addfsheader to create F&S images, fsimage to list F&S images
 *    and extract subimages. The fsimage script can be extracted from an NBoot
 *    image with:
 *
 *      cat nboot.fs | tr '\0' '\n' | tac | sed "/#\!/q" | tac > fsimage.sh
 *
 *    Now fsimage.sh can be used to extract addfsheader and all other images.
 *
 * 2. The i.MX Container view starting after the first F&S header, consisting
 *    of two boot containers, a container with BOARD-CFGs and at least one
 *    container with DRAM settings, All containers and all images within need
 *    to be aligned to 1KB boundaries. Most alignment is done by the mkimage
 *    tool that is used to build the containers, but the overall padding of
 *    each container needs to be done with respect to the containers, not with
 *    respect to the F&S headers. If the first F&S header ist stripped from
 *    the NBoot image, then the image can be stored 1:1 to the boot partition
 *    and the system should boot (assuming that U-Boot is also present).
 *
 * To be able to have the extra info of F&S images also for all the images
 * within a container, F&S has added an INDEX image to each container. It is
 * the first image in the container and just contains all the F&S headers for
 * all other images in the container. In addition, an additional F&S header is
 * prepended to each container, to hold information of the container itself.
 * The container header at the beginning of each container is handled as extra
 * data. Extra data is skipped when listing F&S images, so from the point of
 * view of the F&S image, the first subimage is actually the INDEX.
 * Nonethelesse fsimage.sh will also output some useful info for the container
 * header.
 *
 * Remark: The NXP boot container with the ELE firmware is signed and thus
 * cannot be modified to hold an own INDEX image. Therefore the ELE firmware
 * can not be listed as extra file, it is just shown as unspecified extra data
 * at the beginning of the BOOT-INFO image.
 *
 * FS:  regular F&S image
 * FSH: F&S header only
 * FSI: F&S image part only, header is in INDEX
 *
 *   +----------------------------------------+
 *   | FS: BOOT-INFO (arch) --> INDEX         |
 *   |   +------------------------------------+--- 1KB aligned
 *   |   | Container Header 1 (NXP)           |
 *   |   +------------------------------------+
 *   |   | Container Header 2 (F&S)           |
 *   |   +------------------------------------+---
 *   |   | ELE Firmware                       |    > Container 1 Content
 *   |   +---+--------------------------------+---
 *   |   | FS: INDEX                          |   \
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: UPOWER (arch)             |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: CORTEX-M (arch)           |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: SPL (arch)                |    > Container 2 Content
 *   |   |---+--------------------------------+--- |
 *   |   | FSI: UPOWER image                  |    |
 *   |   |------------------------------------+--- |
 *   |   | FSI: M33 image                     |    |
 *   |   |------------------------------------+--- |
 *   |   | FSI: SPL image                     |   /
 *   +---+------------------------------------+
 *   | FSH: BOOT-ID (id)                      |
 *   +----------------------------------------+
 *   | FS: BOARD-INFO (arch) --> INDEX        |
 *   |   +------------------------------------+---
 *   |   | Container Header                   |
 *   |   +------------------------------------+---
 *   |   | FS: INDEX                          |   \
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: BOARD-CFG2 (id)           |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: BOARD-CFG (id)            |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: ...                       |    > Container Content
 *   |   +---+--------------------------------+--- |
 *   |   | FSI: BOARD-CFG image               |    |
 *   |   +------------------------------------+--- |
 *   |   | FSI: BOARD-CFG image               |    |
 *   |   +------------------------------------+--- |
 *   |   | FSI: ...                           |   /
 *   +---+---+--------------------------------+
 *   | FS: DRAM-TYPE (type) --> INDEX         |
 *   |   +------------------------------------+---
 *   |   | Container Header                   |
 *   |   +------------------------------------+---
 *   |   | FS: INDEX                          |   \
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: DRAM-FW (type)            |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: DRAM-TIMING (chip)        |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: DRAM-TIMING (chip)        |    |
 *   |   |   +--------------------------------+    |
 *   |   |   | FSH: ...                       |    > Container Content
 *   |   +---+--------------------------------+--- |
 *   |   | FSI: DRAM-FW image                 |    |
 *   |   +------------------------------------+--- |
 *   |   | FSI: DRAM-TIMING image             |    |
 *   |   +------------------------------------+--- |
 *   |   | FSI: DRAM-TIMING image             |    |
 *   |   +------------------------------------+--- |
 *   |   | FSI: ...                           |   /
 *   +---+---+--------------------------------+
 *   | FS: DRAM-TYPE (type)  --> INDEX        |
 *   |   +------------------------------------+---
 *   |   | ...                                |
 *   +---+------------------------------------+
 *   | FS: EXTRA (arch)                       |
 *   |   +------------------------------------+
 *   |   | FS: BASH-SCRIPT (addfsheader)      |
 *   |   +------------------------------------+
 *   |   | FS: BASH-SCRIPT (fsimage)          |
 *   +---+------------------------------------+
 */

#ifdef __UBOOT__
#include <common.h>
#include <spl.h>
#include <malloc.h>
#include <sdp.h>
#include <asm/sections.h>
#include <hang.h>
#include <asm/arch/sys_proto.h>
#include <fdt_support.h>
#include <memalign.h>
#include <u-boot/sha256.h>
#include <u-boot/sha512.h>
#include <hash.h>
#include <image.h>
#ifdef CONFIG_AHAB_BOOT
#include <asm/mach-imx/ahab.h>
#endif

#include "fs_dram_common.h"
#include "fs_bootrom.h"

#else

#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/u-boot/sha256.h"
#include "../../../include/u-boot/sha512.h"
#include "linux_helpers.h"
#endif /* __UBOOT__ */

#include <hash.h>
#include "fs_image_common.h"
#include "fs_cntr_common.h"

struct ram_info_t {
	const char *type;
	const char *timing;
};

#if defined(CONFIG_SPL_BUILD)
/* Jobs to do when streaming image data */
#define FSIMG_JOB_BOARD_ID BIT(0)
#define FSIMG_JOB_BOARD_CFG BIT(1)
#define FSIMG_JOB_DRAM BIT(2)
#define FSIMG_JOB_UBOOT BIT(3)
#define FSIMG_FW_JOBS (FSIMG_JOB_DRAM | FSIMG_JOB_UBOOT)

static unsigned int jobs;
static struct fsh_load_info uboot_info;
// static unsigned int mmc_hw_part;
// static unsigned int mmc_offset;

static unsigned int get_jobs(void);
#endif


enum sha_types {
	SHA256,
	SHA384,
	SHA512,
};

/*
 * Use a "constant-length" time compare function for this
 * hash compare:
 *
 * https://crackstation.net/hashing-security.htm
 */
static int slow_equals(u8 *a, u8 *b, int len)
{
	int diff = 0;
	int i;

	for (i = 0; i < len; i++)
		diff |= a[i] ^ b[i];

	return diff == 0;
}

static void __maybe_unused print_hash(u8 *hash, int hash_size)
{
	int i;
	for (i = 0; i < hash_size; i++) {
		printf("%02x", hash[i]);
	}
	puts("\n");
}

bool cntr_image_check_sha(struct boot_img_t *img, void *blob)
{
	unsigned int sha_type;
	const char *algo_name;
	u8 sha[HASH_MAX_DIGEST_SIZE];
	int sha_size;
	int ret;

	sha_type = ((img->hab_flags & GENMASK(10,8)) >> 8);
	switch(sha_type) {
	case SHA256:
		sha_size = SHA256_SUM_LEN;
		algo_name = "sha256";
		break;
	case SHA384:
		sha_size = SHA384_SUM_LEN;
		algo_name = "sha384";
		break;
	case SHA512:
		sha_size = SHA512_SUM_LEN;
		algo_name = "sha512";
		break;
	default:
		algo_name = "NONE";
		break;
	}

	debug("%s: validate %ssum\n", __func__ , algo_name);
	ret = calculate_hash(blob, img->size, algo_name, sha, &sha_size);
	if(ret){
		printf("failed to calc hash: %d\n", ret);
		return false;
	}

#ifdef DEBUG
	puts("CNTR_HASH is ");
	print_hash(img->hash, sha_size);
	puts("Calc HASH is ");
	print_hash(sha, sha_size);
#endif

	return slow_equals(sha, img->hash, sha_size);
}

bool fs_cntr_is_valid_signature(struct container_hdr *cntr_hdr)
{
	__maybe_unused struct container_hdr *authhdr = NULL;
	u16 cntr_length;
	int i;
	bool ret = false;
	struct boot_img_t *img_idx;


	if(!valid_container_hdr(cntr_hdr)){
		return false;
	}

	cntr_length = cntr_hdr->length_lsb + (cntr_hdr->length_msb << 8);;

#if CONFIG_IS_ENABLED(AHAB_BOOT)
	authhdr = ahab_auth_cntr_hdr(cntr_hdr, cntr_length);
	if(!authhdr)
		goto ahab_release;
#endif

	if(!cntr_hdr->num_images)
		goto ahab_release;

	img_idx = (struct boot_img_t *)((u8 *)cntr_hdr + sizeof(struct container_hdr));

	for (i = 0; i < cntr_hdr->num_images; i++) {
		void *img_ptr;

		img_ptr = (void *)cntr_hdr + img_idx[i].offset;

		ret = cntr_image_check_sha(&img_idx[i], img_ptr);
		if(!ret)
			goto ahab_release;
	}

	ahab_release:
#if CONFIG_IS_ENABLED(AHAB_BOOT)
	ahab_auth_release();
#endif
	return ret;
}

bool fs_cntr_is_signed(struct container_hdr *cntr)
{
	/* CHECK CNTR FLAG SRK Set*/
	return !!(cntr->flags & GENMASK(1,0) );
}

/*-------------- Adapted functions from parse-container.c--------------------*/

/**
 * free imx container
 * @param cntr_info: ptr to image_info for imx_container
 */
#ifdef __UBOOT__ /* unused outside of u-boot and spl */
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

	debug("%s: container: 0x%p sector: 0x%lx count: 0x%x\n", __func__,
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

	debug("%s: cntr->num_images=%d\n", __func__,cntr->num_images);
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

		debug("%s: container: 0x%p sector: 0x%lx count: 0x%x\n",
		      __func__, cntr, sector, tmp_count);

		if (info->read(info, (sector + count),
				(tmp_count - count),
				(void *)((ulong)(cntr_tmp) + size)) !=
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

		images = (struct boot_img_t *)((ulong)cntr +
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
 * read_auth_image()
 * based on arch/arm/mach-imx/parse-container.c
 * 
 * @spl_image: returns image_info for image[image_index]
 * @container: ptr to container header
 * @image_index: idx for image in image array.
 * @cntr_sector: startsector of container hdr
 * @returns: ptr if success; else NULL
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
		debug("%s: Invalid image number\n", __func__);
		return NULL;
	}

	images = (struct boot_img_t *)((u8 *)container +
				       sizeof(struct container_hdr));

	if (images[image_index].offset % info->bl_len) {
		printf("%s: image[%d].offset not aligned to %u\n",
		       __func__, image_index, info->bl_len);
		return NULL;
	}

	count = roundup(images[image_index].size, info->bl_len) /
		info->bl_len;
	sector = (images[image_index].offset / info->bl_len) +
		cntr_sector;

	debug("%s: container: 0x%p sector: 0x%lx sectors: 0x%x\n", __func__,
			container, sector, count);
	debug("%s: img_idx=%d, loadaddr=0x%lx \n", __func__,
			image_index, (ulong)images[image_index].dst);
	if (info->read(info, sector, count,
		       (void *)images[image_index].dst) != count) {
		printf("%s: failed to load image %d\n", __func__, image_index);
		return NULL;
	}

#ifdef CONFIG_AHAB_BOOT
/* NOTE:
 * On i.MX8ULP, the ELE configures tRDC (RWX flags) when using SSRAM, preventing
 * overwriting of already validated images and causing resets. To avoid this,
 * only CNTR-HDR signatures are validated via AHAB; image hashes are checked in software.
 */

#if defined(CONFIG_TARGET_FSIMX8ULP)
	if (!cntr_image_check_sha(&images[image_index], (void *)images[image_index].dst)) {
		printf("ERROR: image %d failed SHA check\n", image_index);
		hang();
		return NULL;
	}
#else
	if (ahab_verify_cntr_image(&images[image_index], image_index))
		return NULL;
#endif
#endif
	return &images[image_index];
}

/**
 * fs_cntr_load_single_image()
 * 
 * @image_info: returns image_info for image[image_num]
 * @cntr_info: ptr to spl_image_info for container hdr
 * @load: ptr to load_info
 * @image_num: idx for image in image array.
 * @returns: 0 if success; else -ERRNO;
 */
static int __maybe_unused fs_cntr_load_single_image(struct spl_image_info *image_info,
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

/**
 * fs_cntr_skip_streamed_images()
 * 
 * @image_info: returns image_info form last image
 * @cntr_info: ptr to spl_image_info for container hdr
 * @load: ptr to load_info
 * @start_from_idx: idx for the first image to load.
 * @stop_at_idx: idx for the last image to be load
 * @returns: 0 if success; else -ERRNO;
 */
static int __maybe_unused fs_cntr_skip_streamed_images(struct spl_image_info *image_info,
				struct spl_image_info *cntr_info,
				struct spl_load_info *load,
				int start_from_idx,
				int stop_at_idx)
{
	struct container_hdr *cntr = (struct container_hdr *)cntr_info->load_addr;
	int idx;
	int ret;

	for(idx=start_from_idx; idx <= stop_at_idx && idx < cntr->num_images; idx++){
		ret = fs_cntr_load_single_image(image_info, cntr_info, load, idx);
		if(ret)
			return ret;
	}

	return 0;
}

/**
 * fs_cntr_load_all_images()
 * 
 * @image_info: returns image_info for image[image_num]
 * @cntr_info: ptr to spl_image_info for container hdr
 * @load: ptr to load_info
 * @fst_idx: image info for image[idx].
 * @returns: 0 if success; else -ERRNO;
 */
static int __maybe_unused fs_cntr_load_all_images(struct spl_image_info *image_info,
				struct spl_image_info *cntr_info,
				struct spl_load_info *load,
				int fst_idx)
{
	struct container_hdr *cntr;
	struct spl_image_info info;
	int idx;
	int num_images;
	int ret;

	cntr = (struct container_hdr *)cntr_info->load_addr;
	num_images = (int)cntr->num_images;

	for (idx = 0; idx < num_images; idx++) {
		ret = fs_cntr_load_single_image(&info, cntr_info, load, idx);
		if(idx == fst_idx)
			memcpy(image_info, &info, sizeof(struct spl_image_info));
	}

	debug("image[%d]: load_addr=0x%lx, entry_point=0x%lx, image_size=0x%x\n",
			fst_idx, image_info->load_addr, image_info->entry_point, image_info->size);

	return ret;
}
#endif /* __UBOOT__ */

enum sha_types {
	SHA_TYPE_256,
	SHA_TYPE_384,
	SHA_TYPE_512,
};

/*
 * Use a "constant-length" time compare function for this
 * hash compare:
 *
 * https://crackstation.net/hashing-security.htm
 */
static int __maybe_unused slow_equals(u8 *a, u8 *b, int len)
{
	int diff = 0;
	int i;

	for (i = 0; i < len; i++)
		diff |= a[i] ^ b[i];

	return diff == 0;
}

static void __maybe_unused print_hash(u8 *hash, int hash_size)
{
	int i;
	for (i = 0; i < hash_size; i++) {
		printf("%02x", hash[i]);
	}
	puts("\n");
}

bool cntr_image_check_sha(struct boot_img_t *img, void *blob)
{
	int sha_size = HASH_MAX_DIGEST_SIZE;
	unsigned int sha_type;
	const char *algo_name;
	bool ret = true;

	sha_type = ((img->hab_flags & GENMASK(10,8)) >> 8);
	switch (sha_type) {
	case SHA_TYPE_256:
		algo_name = "sha256";
		break;
	case SHA_TYPE_384:
		algo_name = "sha384";
		break;
	case SHA_TYPE_512:
		algo_name = "sha512";
		break;
	default:
		sha_size = 0;
		algo_name = "NONE";
		break;
	}

	/* No hash in use */
	if (sha_size > 0) {
#ifdef __UBOOT__
		u8 sha[HASH_MAX_DIGEST_SIZE];
		int err;

		err = hash_block(algo_name, blob, img->size, sha, &sha_size);
		if (err) {
			printf("Failed to calc %s: error %d\n", algo_name, err);
			return false;
		}
#ifdef DEBUG
		printf("CNTR %s is ", algo_name);
		print_hash(img->hash, sha_size);
		printf("Calc %s is ", algo_name);
		print_hash(sha, sha_size);
#endif
		ret = slow_equals(sha, img->hash, sha_size);
#else
		printf("Warning: %s not implemented; assuming correct hash\n",
		       algo_name);
#endif /* __UBOOT__ */
	}

	return ret;
}

bool fs_cntr_is_valid_signature(struct container_hdr *cntr_hdr)
{
	__maybe_unused struct container_hdr *authhdr = NULL;
	__maybe_unused u16 cntr_length;
	int i;
	bool ret = false;
	struct boot_img_t *img_idx;


	if(!valid_container_hdr(cntr_hdr)){
		return false;
	}

//TODO: is skipped in linux because the functionality is not fully implemented
//      yet.
#ifdef __UBOOT__
#if CONFIG_IS_ENABLED(AHAB_BOOT)
	cntr_length = cntr_hdr->length_lsb + (cntr_hdr->length_msb << 8);;
	authhdr = ahab_auth_cntr_hdr(cntr_hdr, cntr_length);
	if(!authhdr)
		goto ahab_release;
#endif
#endif

	if(!cntr_hdr->num_images)
		goto ahab_release;

	img_idx = (struct boot_img_t *)((u8 *)cntr_hdr + sizeof(struct container_hdr));

	for (i = 0; i < cntr_hdr->num_images; i++) {
		void *img_ptr;

		img_ptr = (void *)cntr_hdr + img_idx[i].offset;

		ret = cntr_image_check_sha(&img_idx[i], img_ptr);
		if(!ret)
			goto ahab_release;
	}

	ahab_release:
//TODO: skip for now because of missing functionality
#ifdef __UBOOT__
#if CONFIG_IS_ENABLED(AHAB_BOOT)
	ahab_auth_release();
#endif
#endif
	return ret;
}

bool fs_cntr_is_signed(struct container_hdr *cntr)
{
	/* CHECK CNTR FLAG SRK Set*/
	return !!(cntr->flags & GENMASK(1,0) );
}

/* ------------- Functions only in SPL, not U-Boot ------------------------- */
/**
 * The following Code is supposed to load images
 * (BOARD-ID/BOARD-INFO/DRAM-INFO) via BOOTROM.
 */
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
	FSIMG_STATE_UBOOT,
	FSIMG_STATE_DONE,
};

static int fs_handle_board_id(struct fsh_load_info *fsh_info)
{

	struct fs_header_v1_0 *fsh;
	fsh = fsh_info->fsh;

	/* State and file does not match */
	if (!fs_image_match(fsh, "BOARD-ID", NULL)){
		debug("F&S HDR is not type BOARD-ID. Got %s", fsh->type);
		return -EINVAL;
	}

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
	int rev_offs = fs_image_get_board_rev_subnode(fdt, offs);;

	memset(ram_info, 0, sizeof(struct ram_info_t));

	ram_info->type = fs_image_getprop(fdt, offs, rev_offs, "dram-type", NULL);
	ram_info->timing = fs_image_getprop(fdt, offs, rev_offs, "dram-timing", NULL);

	debug("%s: type at 0x%p; timing at 0x%p\n", __func__, ram_info->type, ram_info->timing);
	if(!ram_info->type || !ram_info->timing)
		return -1;

	return 0;
}

static int fs_load_cntr_board_cfg(struct fsh_load_info *fsh_info)
{
	struct fs_header_v1_0 *fsh;
	struct spl_load_info *load_info;
	struct spl_image_info cntr_info;
	struct spl_image_info cfg_info;
	struct container_hdr *cntr;
	struct fs_header_v1_0 *cfg_fsh;
	int idx;
	int num_imgs;
	uint sector;
	uint offset;
	int ret = 0;

	fsh = fsh_info->fsh;
	load_info = fsh_info->load_info;

	offset = roundup(fsh_info->offset, CONTAINER_HDR_ALIGNMENT);
	offset = ALIGN(offset, load_info->bl_len);
	sector = offset / load_info->bl_len;

	ret = fs_cntr_load_imx_container_header(&cntr_info, load_info, sector);
	if(ret)
		return ret;

	debug("FSCNTR: Found IMX-CONTAINER FOR BOARD-INFO\n");
	cntr = (struct container_hdr *)cntr_info.load_addr;
	num_imgs = cntr->num_images;

	/* load index image */
	ret = fs_cntr_load_single_image(&cfg_info,
				&cntr_info,
				load_info,
				0);
	if(ret){
		free_container(&cntr_info);
		return ret;
	}

	cfg_fsh = (struct fs_header_v1_0 *)cfg_info.load_addr;

	if(!fs_image_match(cfg_fsh, "INDEX", NULL)){
		free_container(&cntr_info);
		return -EINVAL;
	}

	/* search board-cfg fsh within index */
	for(idx = 1; idx < num_imgs; idx++){
		if(fs_image_match_board_id(&cfg_fsh[idx]))
			break;
	}

	if(idx >= num_imgs){
		free_container(&cntr_info);
		return -EINVAL;
	}

	debug("FSCNTR: FOUND %s (%s)\n", cfg_fsh[idx].type, cfg_fsh[idx].param.descr);

	/* place fsh in OCRAM */
	memcpy((void *)CFG_FUS_BOARDCFG_ADDR, &cfg_fsh[idx],
			sizeof(struct fs_header_v1_0));

	/* We need to skip images in stream */
	if(is_boot_from_stream_device()){
		ret = fs_cntr_skip_streamed_images(&cfg_info,
				&cntr_info,
				load_info,1,idx-1);
		if(ret){
			free_container(&cntr_info);
			return ret;
		}
	}

	/* load board-cfg.dtb */
	ret = fs_cntr_load_single_image(&cfg_info,
				&cntr_info,
				load_info,
				idx);

	if(ret){
		free_container(&cntr_info);
		return ret;
	}

	{
		/**
		 * TODO: A simple workaround to load data in other sram areas using Cortex-A.
		 * Check between Loadaddr and CFG_FUS_BOARDCFG_ADDR and copy the binary.
		 */
		ulong load_addr = (CFG_FUS_BOARDCFG_ADDR + sizeof(struct fs_header_v1_0));

		if ((ulong)cfg_info.load_addr != load_addr){
			debug("FSCNTR: move board-cfg to another location!\n");
			memcpy((void *)load_addr, (void *)cfg_info.load_addr, cfg_info.size);
		}
	}

	free_container(&cntr_info);
	return ret;
}

static int fs_handle_board_cfg(struct fsh_load_info *fsh_info, struct ram_info_t *ram_info)
{
	struct fs_header_v1_0 *fsh;
	int ret;

	fsh = fsh_info->fsh;

	/* State and file does not match */
	if (!fs_image_match(fsh, "BOARD-INFO", NULL)){
		debug("F&S HDR is not type BOARD-INFO. Got %s\n", fsh->type);
		return -EINVAL;
	}

	ret = fs_load_cntr_board_cfg(fsh_info);
	if(ret) {
		debug("FSCNTR: board_cfg not found!: %d\n", ret);
		return -EINVAL;
	}

	ret = init_ram_info(ram_info);
	if(ret) {
		debug("FSCNTR: dram definition not in board_cfg\n");
		return -EINVAL;
	}

	/* Set Board ID in BOARD-CFG Header */
	fsh = fs_image_get_regular_cfg_addr();
	fs_image_board_cfg_set_board_rev(fsh);
	printf("BOARD-ID: %s\n", fs_image_get_board_id());

	return 0;
}

static int fs_load_cntr_dram_info(struct fsh_load_info *fsh_info, struct ram_info_t *ram_info)
{
	struct fs_header_v1_0 *fsh;
	struct spl_load_info *load_info;
	struct spl_image_info cntr_info;
	struct spl_image_info dram_info;
	struct container_hdr *cntr;
	struct fs_header_v1_0 *dram_fsh;
	int fw_idx = 0;
	int idx;
	int num_imgs;
	uint sector;
	uint offset;
	u32 *dram_crc;
	int ret = 0;

	fsh = fsh_info->fsh;
	load_info = fsh_info->load_info;

	offset = roundup(fsh_info->offset, CONTAINER_HDR_ALIGNMENT);
	offset = ALIGN(offset, load_info->bl_len);
	sector = offset / load_info->bl_len;

	ret = fs_cntr_load_imx_container_header(&cntr_info, load_info, sector);
	if(ret)
		return ret;

	debug("FSCNTR: Found IMX-CONTAINER FOR DRAM-INFO\n");
	cntr = (struct container_hdr *)cntr_info.load_addr;
	num_imgs = cntr->num_images;

	/* load index image */
	ret = fs_cntr_load_single_image(&dram_info,
				&cntr_info,
				load_info,
				0);
	if(ret){
		free_container(&cntr_info);
		return ret;
	}

	dram_fsh = (struct fs_header_v1_0 *)dram_info.load_addr;

	if(!fs_image_match(dram_fsh, "INDEX", NULL)){
		free_container(&cntr_info);
		return -EINVAL;
	}

	/* search dram-fw within index */
	for(idx = 1; idx < num_imgs; idx++){
		if(fs_image_match(&dram_fsh[idx], "DRAM-FW", ram_info->type))
			break;
	}

	/* load dram fw*/
	if(idx < num_imgs){
		fw_idx = idx;
		/* We need to skip images in stream */
		if(is_boot_from_stream_device()){
			ret = fs_cntr_skip_streamed_images(&dram_info,
					&cntr_info,
					load_info,1,fw_idx-1);
			if(ret){
				free_container(&cntr_info);
				return ret;
			}
		}

		debug("FSCNTR: FOUND %s(%s)\n", dram_fsh[idx].type, dram_fsh[idx].param.descr);
		ret = fs_cntr_load_single_image(&dram_info, 
					&cntr_info,
					load_info,
					fw_idx);
		if(ret){
			free_container(&cntr_info);
			return ret;
		}

		memcpy(&_end, (void *)dram_info.load_addr, dram_info.size);
	}

	/* search dram-timing */
	for(idx = 1; idx < num_imgs; idx++){
		if(fs_image_match(&dram_fsh[idx], "DRAM-TIMING", ram_info->timing))
			break;
	}

	if(idx >= num_imgs){
		free_container(&cntr_info);
		return -EINVAL;
	}

	/* We need to skip images in stream */
	if(is_boot_from_stream_device()){
		if(fw_idx >= idx)
			return -EINVAL;

		ret = fs_cntr_skip_streamed_images(&dram_info,
				&cntr_info,
				load_info,fw_idx+1,idx-1);
		if(ret){
			free_container(&cntr_info);
			return ret;
		}
	}

	/* load dram-timing */
	debug("FSCNTR: FOUND %s(%s)\n", dram_fsh[idx].type, dram_fsh[idx].param.descr);
	ret = fs_cntr_load_single_image(&dram_info,
					&cntr_info,
					load_info,
					idx);
	if(ret){
		free_container(&cntr_info);
		return ret;
	}


	dram_crc = (void *)&dram_fsh->type[12];

	printf("DRAM-CRC32: 0x%08x\n", *dram_crc);
	fs_board_init_dram_data((void *)(dram_info.load_addr));

	free_container(&cntr_info);
	return ret;
}

/**
 *  fs_handle_dram()
 * 
 * @fsh: fsh, which should announce a dram-info
 * return: 0 if type matches; -ERRNO if type does not match
 */
static int fs_handle_dram(struct fsh_load_info *fsh_info, struct ram_info_t *ram_info)
{
	struct fs_header_v1_0 *fsh;
	int ret = 0;

	fsh = fsh_info->fsh;

	/* State and file does not match */
	if (!fs_image_match(fsh, "DRAM-INFO", NULL)){
		debug("F&S HDR is not type DRAM-INFO.\n");
		return -EINVAL;
	}

	ret = fs_load_cntr_dram_info(fsh_info, ram_info);
	if(ret) {
		debug("%s: data not found!: %d", __func__, ret);
		return ret;
	}

	return ret;
}

static int set_uboot_info(struct fsh_load_info *fsh_info)
{
	struct fs_header_v1_0 *fsh;
	struct spl_load_info *load_info;

	fsh = malloc(sizeof(struct fs_header_v1_0));
	if(!fsh)
		return -ENOMEM;

	load_info = malloc(sizeof(struct spl_load_info));
	if(!load_info)
		return -ENOMEM;

	memcpy(fsh, fsh_info->fsh, sizeof(struct fs_header_v1_0));
	memcpy(load_info, fsh_info->load_info, sizeof(struct spl_load_info));

	uboot_info.fsh = fsh;
	uboot_info.load_info = load_info;
	uboot_info.offset = fsh_info->offset;
	return 0;
}

static struct fsh_load_info *get_uboot_info(void)
{
	return &uboot_info;
}

/**
 * fs_handle_uboot()
 * 
 * @fsh: fsh, which should announce a U-BOOT-INFO
 * return: 0 if type matches; -ERRNO if not
*/
static int fs_handle_uboot(struct fsh_load_info *fsh_info)
{
	struct fs_header_v1_0 *fsh = fsh_info->fsh;
	struct fs_header_v1_0 *cfg_fsh = fs_image_get_cfg_addr();
	void *fdt = fs_image_get_cfg_fdt();
	unsigned int uboot_size;
	unsigned int uboot_offset;
	unsigned int nboot_start;
	unsigned int nboot_size;
	int ret = 0;

	/* State and file does not match */
	if (!fs_image_match(fsh, "U-BOOT-INFO", NULL)){
		debug("F&S HDR is not type U-BOOT-INFO.\n");
		return -EINVAL;
	}

	ret = set_uboot_info(fsh_info);
	if(ret){
		printf("ERROR: %d\n", ret);
		hang();
	}

	/* Update BOARD-CFG */
	if(!is_boot_from_stream_device()){
		uboot_size = fs_image_get_size(fsh, true);
		uboot_offset = fsh_info->offset;
		nboot_start = fdt_getprop_u32_default(fdt,
						"/nboot-info/emmc-boot", "nboot-start", 0);
		nboot_size = uboot_offset - nboot_start;

		uboot_size = cpu_to_fdt32(uboot_size);
		nboot_size = cpu_to_fdt32(nboot_size);
		uboot_offset = cpu_to_fdt32(uboot_offset);

		fdt_find_and_setprop(fdt, "/nboot-info/emmc-boot",
				"nboot-size", &nboot_size, sizeof(uint), 0);
		fdt_find_and_setprop(fdt, "/nboot-info/emmc-boot",
				"uboot-start", &uboot_offset, sizeof(uint), 0);
		fdt_find_and_setprop(fdt, "/nboot-info/emmc-boot",
				"uboot-size", &uboot_size, sizeof(uint), 0);

		fs_image_update_header(cfg_fsh, fdt_totalsize(fdt), cfg_fsh->info.flags);
	}

	return ret;
}

/* State machine: Use F&S HDR as transition and handle state */
static void fs_cntr_handle(struct fsh_load_info *fsh_info)
{
	static enum fsimg_state state = FSIMG_STATE_BOARD_ID;
	static struct ram_info_t ram_info;

	enum fsimg_state next;
	unsigned int jobs = get_jobs();

	next = state;

	switch (state) {
	case FSIMG_STATE_BOARD_ID:
		if (fs_handle_board_id(fsh_info)){
			next = FSIMG_STATE_DONE;
			break;
		}
		jobs &= ~FSIMG_JOB_BOARD_ID;
		next = FSIMG_STATE_BOARD_CFG;
		break;
	case FSIMG_STATE_BOARD_CFG:
		if(fs_handle_board_cfg(fsh_info, &ram_info)){
			next = FSIMG_STATE_DONE;
			break;
		}
		jobs &= ~FSIMG_JOB_BOARD_CFG;
		next = FSIMG_STATE_DRAM;
		break;
	case FSIMG_STATE_DRAM:
		if(fs_handle_dram(fsh_info, &ram_info))
			break;
		jobs &= ~FSIMG_JOB_DRAM;
		next = FSIMG_STATE_UBOOT;
		break;
	case FSIMG_STATE_UBOOT:
		if(fs_handle_uboot(fsh_info))
			break;
		jobs &= ~FSIMG_JOB_UBOOT;
		next = FSIMG_STATE_DONE;
		break;
	default:
		debug("%s: Current State %d is not considered\n", __func__, state);
		printf("FSCNTR: FSM violation\n");
 		hang();
		break;
	}
	state = next;
	set_jobs(jobs);
}

/**
 * fs_cntr_new_header
 */
static void fs_cntr_new_header(void *dnl_address, uint size)
{
	struct fsh_load_info *fsh_info;

	if(size != FSH_SIZE)
		return;

	fsh_info = (struct fsh_load_info *)dnl_address;

	fs_cntr_handle(fsh_info);
}

static const struct sdp_stream_ops fs_image_sdp_stream_ops = {
	.new_file = fs_cntr_new_header, /* handels F&S ID and Info-Header */
	.rx_data = NULL,
};

/* Load FIRMWARE and optionally BOARD-CFG via SDPS from BOOTROM */
void fs_cntr_nboot_stream(bool need_cfg)
{
	unsigned int jobs_todo = FSIMG_FW_JOBS;

	if (need_cfg){
		jobs_todo |= FSIMG_JOB_BOARD_CFG;
	}

	set_jobs(jobs_todo);

	/* Stream until NBOOT Files are downloaded */
	while (get_jobs()) {
		debug("%s: Jobs not done: 0x%x\n", __func__, jobs);
		bootrom_stream_continue(&fs_image_sdp_stream_ops);
	};
}

void fs_cntr_nboot_mmc(bool need_cfg)
{
	unsigned int jobs_todo = FSIMG_FW_JOBS;

	if (need_cfg){
		jobs_todo |= FSIMG_JOB_BOARD_CFG;
	}

	set_jobs(jobs_todo);

	/* load files, until nboot is done */
	while (get_jobs()) {
		debug("%s: Jobs not done: 0x%x\n", __func__, jobs);
		bootrom_seek_continue(&fs_image_sdp_stream_ops);
	};
}

int fs_cntr_init(bool need_cfg)
{
	int ret = 0;

	if(is_boot_from_stream_device()){
		debug("FSCNTR: BOOT FROM STREAMDEV\n");
		fs_cntr_nboot_stream(need_cfg);
		return ret;
	}

	debug("FSCNTR: BOOT FROM BLOCK DEVICE\n");
	fs_cntr_nboot_mmc(need_cfg);
	return ret;
}

static void fs_cntr_board_id_stream(void)
{
	unsigned int jobs_todo = FSIMG_JOB_BOARD_ID;

	set_jobs(jobs_todo);

	/* Stream until BOARD-ID HDR is downloaded */
	while (get_jobs()) {
		debug("%s: Jobs not done: 0x%x\n", __func__, jobs);
		bootrom_stream_continue(&fs_image_sdp_stream_ops);
	};
}

static void fs_cntr_board_id_mmc(void)
{
	unsigned int jobs_todo = FSIMG_JOB_BOARD_ID;

	set_jobs(jobs_todo);

	/* load files, until BOARD-ID HDR is done */
	while (get_jobs()) {
		debug("%s: Jobs not done: 0x%x\n", __func__, jobs);
		bootrom_seek_continue(&fs_image_sdp_stream_ops);
	};
}

int fs_cntr_load_board_id()
{
	int ret = 0;

	if(is_boot_from_stream_device()){
		debug("FSCNTR: GET BOARD-ID FROM STREAMDEV\n");
		fs_cntr_board_id_stream();
		return ret;
	}

	debug("FSCNTR: GET BOARD-ID FROM BLOCK DEVICE\n");
	fs_cntr_board_id_mmc();
	return ret;
}
#endif /* CONFIG_SPL_BUILD */
/* ------------------------------------------------------------------------- */

/* ------------- Functions only in SPL, not U-Boot ------------------------- */
/**
 *  The following Code is supposed to load U-BOOT-INFO as SPL Load Method
 */
#if defined(CONFIG_SPL_BUILD)

static int load_uboot(struct spl_image_info *spl_image)
{
	struct fsh_load_info *uboot_info;
	struct spl_load_info *load_info;
	struct spl_image_info cntr_info;
	uint offset;
	uint sector;
	int ret = 0;

	uboot_info = get_uboot_info();
	load_info = uboot_info->load_info;

	offset = uboot_info->offset;
	offset = roundup(offset, CONTAINER_HDR_ALIGNMENT);
	offset = ALIGN(offset, load_info->bl_len);
	sector = offset / load_info->bl_len;

	ret = fs_cntr_load_imx_container_header(&cntr_info, load_info, sector);
	if(ret)
		return ret;

	ret = fs_cntr_load_all_images(spl_image, &cntr_info, load_info, 1);

	free_container(&cntr_info);

	return ret;
}

int board_return_to_bootrom(struct spl_image_info *spl_image,
			    struct spl_boot_device *bootdev)
{
	return load_uboot(spl_image);
}
#endif /* CONFIG_SPL_BUILD */
