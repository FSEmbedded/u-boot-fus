#ifdef __UBOOT__
#include <common.h>
#include <fslib.h>
#include <fslib_common.h>
#include <fslib_board_common.h>
#include <image.h>
#include <fdt_support.h>
#include <mmc.h>
#include <malloc.h>
#include <dm/device.h>
#include <asm/arch/sys_proto.h>
#include <linux/err.h>
#include <console.h>			/* confirm_yesno() */
#include <imx_container.h>
#include "../board/F+S/common/fs_bootrom.h"
#include <u-boot/crc.h>
#else
#include <stdio.h>
#include <errno.h>
#include "fsimage_backend.h"
#include "../../board/F+S/common/fs_image_common.h"
#include "../../include/fdt_support.h"
#include "../../include/linux/libfdt.h"
#include "../../include/linux/libfdt_env.h"
#include <stdlib.h>
#include <ctype.h>
#include "../../board/F+S/common/crc32.h"

#define FIT_IMAGES_PATH		"/images"
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)

char saved_nboot_buffer[1024*1024*4];
char nboot_buffer[1024*1024*4];
int current_boot_part = 0;

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0)
#endif

#define FIT_DATA_SIZE_PROP	"data-size"
/**
 * Get 'data-size' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_size: holds the data-size property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_size(const void *fit, int noffset, int *data_size)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_SIZE_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_size = fdt32_to_cpu(*val);

	return 0;
}

#define FIT_DATA_OFFSET_PROP	"data-offset"
/**
 * Get 'data-offset' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_offset: holds the data-offset property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */
int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_OFFSET_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_offset = fdt32_to_cpu(*val);

	return 0;
}

/**
 * Get 'data-position' property from a given image node.
 *
 * @fit: pointer to the FIT image header
 * @noffset: component image node offset
 * @data_position: holds the data-position property
 *
 * returns:
 *     0, on success
 *     -ENOENT if the property could not be found
 */

#define FIT_DATA_POSITION_PROP	"data-position"
int fit_image_get_data_position(const void *fit, int noffset,
				int *data_position)
{
	const fdt32_t *val;

	val = fdt_getprop(fit, noffset, FIT_DATA_POSITION_PROP, NULL);
	if (!val)
		return -ENOENT;

	*data_position = fdt32_to_cpu(*val);

	return 0;
}

/**
 * fit_get_end - get FIT image size
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     size of the FIT image (blob) in memory
 */
static inline ulong fit_get_size(const void *fit)
{
	return fdt_totalsize(fit);
}

/**
 * fit_get_name - get FIT node name
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     NULL, on error
 *     pointer to node name, on success
 */
static inline const char *fit_get_name(const void *fit_hdr,
		int noffset, int *len)
{
	return fdt_get_name(fit_hdr, noffset, len);
}

static void fit_get_debug(const void *fit, int noffset,
		char *prop_name, int err)
{
	debug("Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
	      prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
	      fdt_strerror(err));
}

#define FIT_DATA_PROP		"data"
/**
 * fit_image_get_data - get data property and its size for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @data: double pointer to void, will hold data property's data address
 * @size: pointer to size_t, will hold data property's data size
 *
 * fit_image_get_data() finds data property in a given component image node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_data(const void *fit, int noffset,
		const void **data, size_t *size)
{
	int len;

	*data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
	if (*data == NULL) {
		fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
		*size = 0;
		return -1;
	}

	*size = len;
	return 0;
}

// Digest is for compatibility between nboot and linux function signature,
// always NULL when called and unused for Linux implementatiom.
ulong parse_loadaddr(char *filename, void *digest) {
    FILE *file = fopen(filename, "ro");
	if(!file) {
		printf("Error opening %s, exiting...\n", filename);
		return -ENOENT;
	}
	size_t bytes_read = fread(nboot_buffer, 1, 4*1024*1024, file);
	if(!bytes_read) {
		printf("Error reading data from %s, exiting...\n", filename);
		return -EINVAL;
	}	
	fclose(file);
	return (ulong)nboot_buffer;
}

ulong get_loadaddr(void){
    return (ulong)saved_nboot_buffer;
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base) {
	return strtoul(cp, endp, base);
}

long simple_strtol(const char *cp, char **endp, unsigned int base) {
	return strtol(cp, endp, base);
}

//TODO: DD klappt das??
#define cpu_to_fdt32(x) __builtin_bswap32(x)

static u32 fdt_getprop_u32_default_node(const void *fdt, int off, int cell,
				const char *prop, const u32 dflt)
{
	const fdt32_t *val;
	int len;

	val = fdt_getprop(fdt, off, prop, &len);

	/* Check if property exists */
	if (!val)
		return dflt;

	/* Check if property is long enough */
	if (len < ((cell + 1) * sizeof(uint32_t)))
		return dflt;

	return fdt32_to_cpu(*val);
}

/**
 * fdt_find_and_setprop: Find a node and set it's property
 *
 * @fdt: ptr to device tree
 * @node: path of node
 * @prop: property name
 * @val: ptr to new value
 * @len: length of new property value
 * @create: flag to create the property if it doesn't exist
 *
 * Convenience function to directly set a property given the path to the node.
 */
int fdt_find_and_setprop(void *fdt, const char *node, const char *prop,
			 const void *val, int len, int create)
{
	int nodeoff = fdt_path_offset(fdt, node);

	if (nodeoff < 0)
		return nodeoff;

	if ((!create) && (fdt_get_property(fdt, nodeoff, prop, NULL) == NULL))
		return 0; /* create flag not set; so exit quietly */

	return fdt_setprop(fdt, nodeoff, prop, val, len);
}

enum boot_stage_type {
	BT_STAGE_PRIMARY = 0x6,
	BT_STAGE_SECONDARY = 0x9,
	BT_STAGE_RECOVERY = 0xa,
	BT_STAGE_USB = 0x5,
};

int get_bootrom_bootstage(u32 *bstage)
{
	return -ENODEV;
}

int get_container_size(ulong addr, u16 *header_length)
{
	struct container_hdr *phdr;
	struct boot_img_t *img_entry;
	struct signature_block_hdr *sign_hdr;
	u8 i = 0;
	u32 max_offset = 0, img_end;

	phdr = (struct container_hdr *)addr;
	if (!valid_container_hdr(phdr)) {
		debug("Wrong container header\n");
		return -EFAULT;
	}

	max_offset = phdr->length_lsb + (phdr->length_msb << 8);
	if (header_length)
		*header_length = max_offset;

	img_entry = (struct boot_img_t *)(addr + sizeof(struct container_hdr));
	for (i = 0; i < phdr->num_images; i++) {
		img_end = img_entry->offset + img_entry->size;
		if (img_end > max_offset)
			max_offset = img_end;

		debug("img[%u], end = 0x%x\n", i, img_end);

		img_entry++;
	}

	if (phdr->sig_blk_offset != 0) {
		sign_hdr = (struct signature_block_hdr *)(addr + phdr->sig_blk_offset);
		u16 len = sign_hdr->length_lsb + (sign_hdr->length_msb << 8);

		if (phdr->sig_blk_offset + len > max_offset)
			max_offset = phdr->sig_blk_offset + len;

		debug("sigblk, end = 0x%x\n", phdr->sig_blk_offset + len);
	}

	return max_offset;
}
#endif


/* Argument of option -e in fsimage save */
static uint early_support_index;

static void fs_image_get_flash_mmc_generic(struct flash_info *fi)
{
#if __UBOOT__
	struct udevice *mmc_dev = dev_get_parent(fi->bdev);
	struct blk_desc *bdesc = dev_get_uclass_plat(fi->bdev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(mmc_dev);
	struct mmc *mmc =  upriv->mmc;

	/* Determine hwpart (when command starts) and boot hwpart */
	fi->old_hwpart = bdesc->hwpart;
	fi->boot_hwpart = EXT_CSD_EXTRACT_BOOT_PART(mmc->part_config);
	if (fi->boot_hwpart > 2)
		fi->boot_hwpart = 0;

	/* Temporary buffer is for one block */
	fi->temp_size = bdesc->blksz;

	/* Set device name */
	sprintf(fi->devname, "mmc%d", bdesc->devnum);
#else
//TODO: DD Das muss implementiert werden
	sprintf(fi->devname, "mmc%d", 0);
	fi->temp_size = 0x200;
	fi->old_hwpart = 0;
	fi->boot_hwpart = 1;
	printf("shortcut... cannot decide for flash info values dynamically at this time...\n");
#endif
}

/* Build lowercase nboot-info property name from upper-case region name */
static void fs_image_build_nboot_info_name(char *name, const char *prefix,
				const char *suffix)
{
	char c;

	/* Copy prefix in lowercase, drop hyphens from region name */
	do {
		c = *prefix++;
		if (c && (c != '-')) {
			if ((c >= 'A') && (c <= 'Z'))
				c += 'a' - 'A';
			*name++ = c;
		}
	} while (c);

	/* Append suffix */
	do {
		c = *suffix++;
		*name++ = c;
	} while (c);
}

/* Get start[0..1] and size for a storage info */
static int fs_image_get_si(void *fdt, int offs, uint align, const char *type,
			   struct storage_info *si)
{
	int err;
	char name[20];

	si->type = type;

	/* Create the property name for nboot-info and get value */
	fs_image_build_nboot_info_name(name, type, "-start");

	err = fs_image_get_fdt_val(fdt, offs, name, align, 2, si->start);
	if (err)
		return err;

	/* Create the property name for nboot-info and get value */
	fs_image_build_nboot_info_name(name, type, "-size");

	return fs_image_get_fdt_val(fdt, offs, name, align, 1, &si->size);
}

/* Parse nboot-info for MMC settings and fill struct */
static int fs_image_get_nboot_info_mmc(struct flash_info *fi, void *fdt,
				       int offs, struct nboot_info *ni,
				       int boot_hwpart, bool show)
{
#if __UBOOT__
	struct udevice *mmc_dev = dev_get_parent(fi->bdev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(mmc_dev);
	struct mmc *mmc = upriv->mmc;
#endif
	int layout;
	const char *layout_name;
	int err;
	uint align = FSH_SIZE;
	u8 first = (boot_hwpart < 0) ? fi->boot_hwpart : boot_hwpart;
	u8 second = first ? (3 - first) : first;

	/* Go to layout subnode if present */
	layout_name = first ? "emmc-boot" : "sd-user";
	layout = fdt_subnode_offset(fdt, offs, layout_name);
	if (layout < 0) {
		layout_name = "old";
		layout = offs;
	}

	/* Pre 2023.08, everything was in the same boot hwpart on fsimx8mm */
	if (!(ni->flags & NI_EMMC_BOTH_BOOTPARTS))
		second = first;

	/* Get SPL storage info */
	err = fs_image_get_si(fdt, layout, align, "SPL", &ni->spl);
	if (err)
		return err;
	ni->spl.hwpart[0] = first;
	ni->spl.hwpart[1] = second;

	/* Get NBoot storage info */
	err = fs_image_get_si(fdt, layout, align, "NBOOT", &ni->nboot);
	if (err)
		return err;
	ni->nboot.hwpart[0] = first;
	ni->nboot.hwpart[1] = second;

	/* Get U-Boot storage info */
	err = fs_image_get_si(fdt, layout, align, "U-BOOT", &ni->uboot);
	if (err)
		return err;
	if (ni->flags & NI_UBOOT_EMMC_BOOTPART) {
		ni->uboot.hwpart[0] = first;
		ni->uboot.hwpart[1] = second;
		/* Limit U-Boot size to boot part size */
#if __UBOOT__
		if (ni->uboot.size > (u32)(mmc->capacity_boot))
			ni->uboot.size = (u32)(mmc->capacity_boot);
#endif
	} else {
		ni->uboot.hwpart[0] = 0;
		ni->uboot.hwpart[1] = 0;
	}

#ifndef CONFIG_IMX8MM
	/*
	 * In the old layout some addresses were given for the User hwpart
	 * only, so we had to know the alternative addresses when booting from
	 * a boot partiton. This does not contradict with the new layout, so
	 * we can keep them as they are.
	 */
	if (first) {
		/* SPL always starts on sector 0 in boot1/2 hwpart */
		ni->spl.start[0] = 0;
		ni->spl.start[1] = 0;

		/* Both NBoot copies start on same sector in boot1/2 */
		ni->nboot.start[1] = ni->nboot.start[0];
	}
#endif

	/* Get env info from nboot-info */
	err = fs_image_get_si(fdt, layout, align, "ENV", &ni->env);
	if (err == -ENOENT) {
		/*
		 * No env data found in nboot-info, fall back to some known
		 * values. Use option -e to select index.
		 */
		err = fs_image_get_known_env_mmc(early_support_index,
						 ni->env.start, &ni->env.size);
	}
	if (err)
		return err;

	/*
	 * In the past, both environment copies were on the same hwpart. But
	 * if the two addresses are equal, they are obviously on different
	 * hwparts.
	 */
	ni->env.hwpart[0] = first;
	ni->env.hwpart[1] = first;
	if (first && (ni->env.start[0] == ni->env.start[1]))
		ni->env.hwpart[1] = second;

#ifndef DEBUG
	if (!show)
		return 0;
#endif

	printf("- nboot-info@0x%lx (%s layout): Booting from %s hwpart %d\n",
	       (ulong)fdt, layout_name, fi->devname, first);
	if (ni->board_cfg_size)
		printf("- board-cfg-size=0x%08x\n", ni->board_cfg_size);
	printf("- spl:   start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->spl.hwpart[0], ni->spl.start[0],
	       ni->spl.hwpart[1], ni->spl.start[1], ni->spl.size);
	printf("- nboot: start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->nboot.hwpart[0], ni->nboot.start[0],
	       ni->nboot.hwpart[1], ni->nboot.start[1], ni->nboot.size);
	printf("- uboot: start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->uboot.hwpart[0], ni->uboot.start[0],
	       ni->uboot.hwpart[1], ni->uboot.start[1], ni->uboot.size);
	printf("- env:   start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->env.hwpart[0], ni->env.start[0],
	       ni->env.hwpart[1], ni->env.start[1], ni->env.size);
	return 0;
}

static int fs_image_set_hwpart_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si)
{
#if __UBOOT__
	struct blk_desc *bdesc = dev_get_uclass_plat(fi->bdev);
	int err;
	uint hwpart = si->hwpart[copy];

	err = blk_select_hwpart(fi->bdev, hwpart);
	if (err)
		printf("  Cannot switch to hwpart %d on mmc%d for %s (%d)\n",
		       hwpart, bdesc->devnum, si->type, err);

	return err;
#else
	current_boot_part = copy;
	return 0;
#endif
}

/* Switch to partition where reion is located and show region info */
static int fs_image_prepare_region_mmc(struct flash_info *fi, int copy,
				       struct storage_info *si)
{
	int err;

	err = fs_image_set_hwpart_mmc(fi, copy, si);
	if (err)
		return err;

	printf("  -- %s (hwpart %d) --\n", si->type, si->hwpart[copy]);

	return 0;
}

/* Save some data (only full pages) to NAND; return 1 if new bad block */
static int fs_image_write_mmc(struct flash_info *fi, uint offs, uint size,
			      uint lim, uint flags, u8 *buf)
{
#if __UBOOT__
	struct blk_desc *bdesc = dev_get_uclass_plat(fi->bdev);
	ulong count;
	ulong blksz = bdesc->blksz;
	lbaint_t blk = offs / blksz;
	lbaint_t blk_count = (size + blksz - 1) / blksz;;

	/* Bad block handling is done by eMMC controller */
	debug("  -> mmc_write to offs 0x%x (block 0x" LBAF ") size 0x%x\n",
	      offs, blk, size);

	count = blk_write(fi->bdev, blk, blk_count, buf);
	if (count < blk_count)
		return -EIO;
	else if (IS_ERR_VALUE(count))
		return (int)count;
#else
	char devicename[32];
	snprintf(devicename, 32, "/dev/mmcblk0boot%x", current_boot_part);
	FILE *mmc = fopen(devicename, "w+b");
	if(!mmc) {
		printf("Error opening %s, exiting...\n", devicename);
		return -EINVAL;
	}

	int seek = fseek(mmc, offs, SEEK_SET);
	if(seek != 0) {
		printf("Error (0x%x) while seeking in %s, exiting...\n", seek, devicename);
		return -EINVAL;
	}

	size_t bytes_read = fwrite(buf, 1, size, mmc);
	if(bytes_read != size) {
		printf("Error (0x%lx) while writing %s, exiting...\n", bytes_read, devicename);
		return -EINVAL;
	}
	fclose(mmc);
#endif
	return 0;
}

static void fs_image_drop_temp(struct flash_info *fi);
static void fs_image_show_sub_status(int err);
/* Invalidate an image by overwriting the first block with zeroes */
static int fs_image_invalidate_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si)
{
	uint offs = si->start[copy];
	uint lim = offs + si->size;
	int err;

	printf("  Invalidating %s at offset 0x%08x size 0x%x...",
	       si->type, offs, si->size);
	debug("\n");

	fs_image_drop_temp(fi);
	err = fi->ops->write(fi, offs, fi->temp_size, lim, 0, fi->temp);

	fs_image_show_sub_status(err);

	return err;
}

/* Read image at offset with given size */
static int fs_image_read_mmc(struct flash_info *fi, uint offs, uint size,
			     uint lim, uint flags, u8 *buf)
{
#if __UBOOT__
	struct blk_desc *bdesc = dev_get_uclass_plat(fi->bdev);
	ulong count;
	ulong blksz = bdesc->blksz;
	lbaint_t blk = offs / blksz;
	lbaint_t blk_count = (size + blksz - 1) / blksz;

	debug("  -> mmc_read from offs 0x%x (block 0x" LBAF ") size 0x%x\n",
	      offs, blk, size);

	count = blk_read(fi->bdev, blk, blk_count, buf);
	if (count < blk_count)
		return -EIO;
	else if (IS_ERR_VALUE(count))
		return (int)count;

	return 0;
#else
	char devicename[32];
	snprintf(devicename, 32, "/dev/mmcblk0boot%x", current_boot_part);
	FILE *mmc = fopen(devicename, "rb");
	if(!mmc) {
		printf("Error opening %s, exiting...\n", devicename);
		return -EINVAL;
	}
	int seek = fseek(mmc, offs, SEEK_SET);
	if(seek != 0) {
		printf("Error (0x%x) while seeking in %s, exiting...\n", seek, devicename);
		return -EINVAL;
	}
	size_t bytes_read = fread(buf, 1, size, mmc);
	if(bytes_read != size) {
		printf("Error while reading %s, exiting...\n", devicename);
		return -EINVAL;
	}
	fclose(mmc);
	return 0;
#endif
}

static void fs_image_put_flash_mmc(struct flash_info *fi)
{
#if __UBOOT__
	if (blk_select_hwpart(fi->bdev, fi->old_hwpart)) {
		printf("Cannot switch back to original hwpart %d\n",
		       fi->old_hwpart);
	}
#else
	current_boot_part = 0;
#endif
}

static int fs_image_set_boot_hwpart_mmc(struct flash_info *fi, int boot_hwpart)
{
#if __UBOOT__
	int err;

	if ((boot_hwpart < 0) || (boot_hwpart == fi->boot_hwpart))
		return 0;

	printf("\nSwitching %s to boot hwpart %d...", fi->devname, boot_hwpart);

	err = blk_select_hwpart(fi->bdev, boot_hwpart);

	if (!err)
		fi->boot_hwpart = boot_hwpart;

	return err;
#else
	current_boot_part = boot_hwpart;
	return 0;
#endif
}

/* Check if hwpart, start address or size differs */
static bool fs_image_si_differs_mmc(const struct storage_info *si1,
				    const struct storage_info *si2)
{
	return ((si1->size != si2->size)
		|| (si1->hwpart[0] != si2->hwpart[0])
		|| (si1->start[0] != si2->start[0])
		|| (si1->hwpart[1] != si2->hwpart[1])
		|| (si1->start[1] != si2->start[1]));
}

static int fs_image_load_sub(struct flash_info *fi, uint offs, uint size,
			     uint lim, uint flags, u8 *buf);
static int fs_image_get_size_from_fit(struct sub_info *sub, uint *size);
static int fs_image_get_size_from_fsh_or_ivt(struct sub_info *sub, uint *size);
static int fs_image_check_env_crc32(void *env, uint size);
static int fs_image_check_all_crc32(struct fs_header_v1_0 *fsh);

/* Load the image of given type/descr from eMMC at given offset */
static int fs_image_load_image_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si,
				   struct sub_info *sub)
{
	uint size;
	uint offs = si->start[copy] + sub->offset;
	uint lim = si->start[copy] + si->size;
	uint hwpart = si->hwpart[copy];
	int err;

	sub->size = 0;

	printf("  Loading copy %d from hwpart %d offset 0x%08x", copy, hwpart,
	       offs);
	debug("\n");

	/* Clear the temp buffer (read cache) */
	fs_image_drop_temp(fi);

	err = fs_image_set_hwpart_mmc(fi, copy, si);
	if (err)
		return err;

	if (sub->flags & SUB_IS_ENV) {
		size = si->size;
	} else {
		/* Read F&S header, IVT or FIT header */
		size = sizeof(union any_header);
		err = fs_image_load_sub(fi, offs, size, lim, 0, sub->img);
		if (err)
			return err;

		/* Get image size */
		if (fdt_magic(sub->img) == FDT_MAGIC) {
			/* Read the FDT part of the FIT image to get size */
			size = fdt_totalsize(sub->img);
			err = fs_image_load_sub(fi, offs, size, lim, 0,
						sub->img);
			if (!err)
				err = fs_image_get_size_from_fit(sub, &size);
		} else {
			err = fs_image_get_size_from_fsh_or_ivt(sub, &size);
#ifdef CONFIG_IMX8MM
			/* Remove virtual IVT offset */
			if (sub->flags & SUB_IS_SPL)
				size -= 0x400;
#endif
		}
		if (err)
			return err;
	}

	printf(" size 0x%x...", size);
	debug("\n");

	/* Load whole image incl. header */
	err = fs_image_load_sub(fi, offs, size, lim, sub->flags, sub->img);
	if (err)
		return err;

	if (sub->flags & SUB_IS_ENV) {
		err = fs_image_check_env_crc32(sub->img, size);
	} else if (sub->flags & SUB_HAS_FS_HEADER) {
		err = fs_image_check_all_crc32(sub->img);
		if (err < 0)
			return err;
	} else if (fs_image_is_fs_image(sub->img)) {
		/*
		 * We found an F&S header on an image that may or may not have
		 * one (e.g. U-BOOT). Check the CRC32, but then remove the
		 * header, because the caller has already inserted an empty
		 * header before sub->img and will fill it after we return.
		 */
		err = fs_image_check_all_crc32(sub->img);
		size -= FSH_SIZE;
		memmove(sub->img, sub->img + FSH_SIZE, size);
	}

	sub->size = size;

	return 0;
}

static int fs_image_save_region(struct flash_info *fi, int copy,
				struct region_info *ri);
static int fs_image_get_start_copy(void);
/* Save NBOOT and SPL region to MMC */
static int fs_image_save_nboot_mmc(struct flash_info *fi,
				   struct region_info *nboot_ri,
				   struct region_info *spl_ri)
{
	int failed;
	int copy, start_copy;

	/*
	 * When saving NBoot, start with "the other" copy first, i.e. if
	 * running form Primary SPL, update the Secondary copy first and if
	 * running from Secondary SPL, update the Primary copy first. The
	 * reason is that the current copy is apparently working, but the
	 * other copy may very well be broken. So it makes sense to first
	 * update the broken version and repair it by doing so, before
	 * touching the working version.
	 *
	 * For example if currently running on the Secondary copy, this means
	 * that the Primary copy is damaged. So if the Secondary copy was
	 * updated first and this failed for some reason, then both copies
	 * would be non-functional and the board would be bricked. But if the
	 * damaged Primary copy is updated first and this succeeds, the
	 * Primary copy is repaired and provides a working fallback when
	 * writing the Secondary copy afterwards.
	 *
	 * Start with the "other" copy:
	 *
	 *  1. Invalidate the "other" NBOOT region by overwriting the first
	 *     block. This immediately invalidates the F&S header of the
	 *     BOARD-CFG so that this copy will definitely not be loaded
	 *     anymore.
	 *  2. Write all of the "other" NBOOT but the first block. If
	 *     interrupted, the BOARD-CFG ist still invalid and will not be
	 *     loaded.
	 *  3. Write first block of the "other" NBOOT. This adds the F&S
	 *     header and makes NBOOT valid.
	 *  4. Invalidate the "other" SPL region. This immediately invalidates
	 *     SPL (IVT) so that this copy will definitely not be loaded
	 *     anymore.
	 *  5. Write all of the "other" SPL but the first block. If
	 *     interrupted, SPL is still invalid and will not be loaded.
	 *  6. Write the first block of the "other" SPL. This adds the IVT and
	 *     makes SPL valid.
	 *  7. On i.MX8MM: If booting from User partition and the "other" copy
	 *     is the Secondary copy, update the information block for the
	 *     secondary SPL.
	 *
	 * If interrupted somewhere in steps 1 to 7, the "current" copy is
	 * still available and will continue to boot. After step 6, the
	 * "other" copy is fully functional. So if the "other" copy is the
	 * Primary copy, it will be booted after Step 6 again.
	 *
	 *  8. Update the "current" NBOOT in the same sequence.
	 *  9. Update the "current" SPL in the same sequence.
	 * 10. On i.MX8MM: If booting from User partition and the "current" copy
	 *     is the Secondary copy: Update the information block for the
	 *     secondary SPL.
	 *
	 * The worst case happens if interrupted in step 8 and if "current" is
	 * the Primary copy. Then the "current" (=Primary) but still old SPL
	 * will boot, but fails to load the "current" (=Primary) NBOOT,
	 * because it is invalid right now. So it will fall back to load the
	 * "other" (=Secondary) NBOOT, which is the new version already. This
	 * may or may not work, depending on how compatible the old and new
	 * versions are.
	 *
	 * If interrupted in step 9, the "other" (=Secondary) copy is loaded,
	 * which is the new version already. This is OK.
	 *
	 * ### TODO:
	 * The sequence above assumes that SPL can detect correctly from which
	 * copy it was booting. Currently this is not true on i.MX8MN/MP/X.
	 */
	failed = 0;
	start_copy = fs_image_get_start_copy();
	copy = start_copy;
	do {
		printf("\nSaving copy %d to %s:\n", copy, fi->devname);
		if (fs_image_save_region(fi, copy, nboot_ri))
			failed |= BIT(copy);

#if __UBOOT__
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
		fs_image_set_spl_secondary_bit(spl_ri, copy);
#endif
#endif
		if (fs_image_save_region(fi, copy, spl_ri))
			failed |= BIT(copy);

#if __UBOOT__
#ifdef CONFIG_IMX8MM
		struct storage_info *si = spl_ri->si;

		/* Write Secondary Image Table */
		if (fs_image_write_secondary_table(fi, copy, si))
			failed = BIT(copy);

		/* If in boot part, write another copy to the other boot part */
		if (si->hwpart[copy] != si->hwpart[1-copy]) {
			si->hwpart[copy] = 3 - si->hwpart[copy];
			if (fs_image_save_region(fi, copy, spl_ri))
				failed |= BIT(copy);
			if (fs_image_write_secondary_table(fi, copy, si))
				failed = BIT(copy);
			si->hwpart[copy] = 3 - si->hwpart[copy];
		}
#endif
#endif
		copy = 1 - copy;
	} while (copy != start_copy);

	return failed;
}

//TODO: DD rename wenn der andere raus ist
struct flash_ops flash_ops_mmc_generic = {
	.check_for_uboot = NULL,
	.check_for_nboot = NULL,
	.get_nboot_info = fs_image_get_nboot_info_mmc,
	.si_differs = fs_image_si_differs_mmc,
	.read = fs_image_read_mmc,
	.load_image = fs_image_load_image_mmc,
	.load_extra = NULL,
	.invalidate = fs_image_invalidate_mmc,
	.write = fs_image_write_mmc,
	.prepare_region = fs_image_prepare_region_mmc,
	.save_nboot = fs_image_save_nboot_mmc,
	.set_boot_hwpart = fs_image_set_boot_hwpart_mmc,
	.get_flash = fs_image_get_flash_mmc_generic,
	.put_flash = fs_image_put_flash_mmc,
};

/* Clean up flash info */
static void fs_image_put_flash_info(struct flash_info *fi)
{
	fi->ops->put_flash(fi);
	free(fi->temp);
}

static int fs_image_get_nboot_info(struct flash_info *fi, void *fdt,
				   struct nboot_info *ni, int hwpart, bool show)
{
	int offs = fs_image_get_nboot_info_offs(fdt);

	if (offs < 0) {
		puts("Cannot find nboot-info in BOARD-CFG\n");
		return -ENOENT;
	}

	memset(ni, 0, sizeof(*ni));

	/* Parse generic NBoot capablities here */
	ni->board_cfg_size = fdt_getprop_u32_default_node(fdt, offs, 0,
							  "board-cfg-size", 0);
#if 0
	/* ### Debug: Needed to test against versions since the env addresses
               were moved to nboot-info */
	ni->flags |= NI_SUPPORT_CRC32 | NI_SAVE_BOARD_ID | NI_UBOOT_WITH_FSH;
#endif
	if (fdt_getprop(fdt, offs, "support-crc32", NULL))
		ni->flags |= NI_SUPPORT_CRC32;
	if (fdt_getprop(fdt, offs, "save-board-id", NULL))
		ni->flags |= NI_SAVE_BOARD_ID;
	if (fdt_getprop(fdt, offs, "uboot-with-fsh", NULL))
		ni->flags |= NI_UBOOT_WITH_FSH;
	if (fdt_getprop(fdt, offs, "uboot-emmc-bootpart", NULL))
		ni->flags |= NI_UBOOT_EMMC_BOOTPART;
#ifdef CONFIG_IMX8MM
	/* Have a flag that not everything is in one boot partition */
	if (fdt_getprop(fdt, offs, "emmc-both-bootparts", NULL))
		ni->flags |= NI_EMMC_BOTH_BOOTPARTS;
#else
	/* This has always been the default on all other architectures */
	ni->flags |= NI_EMMC_BOTH_BOOTPARTS;
#endif

	/* Parse flash specific settings individually */
	return fi->ops->get_nboot_info(fi, fdt, offs, ni, hwpart, show);
}

void fs_image_print_line(struct fs_header_v1_0 *fsh, uint offs, int level)
{
	char info[MAX_DESCR_LEN + 1];
	int i;

	/* Show info for this image */
	printf("%08x %08x", offs, fs_image_get_size(fsh, false));
	for (i = 0; i < level; i++)
#ifdef __UBOOT__
		putc(' ');
#else
		printf(" ");
#endif
	if (fsh->type[0]) {
		memcpy(info, fsh->type, MAX_TYPE_LEN);
		info[MAX_TYPE_LEN] = '\0';
		printf(" %s", info);
	}
	if ((fsh->info.flags & FSH_FLAGS_DESCR) && fsh->param.descr[0]) {
		memcpy(info, fsh->param.descr, MAX_DESCR_LEN);
		info[MAX_DESCR_LEN] = '\0';
		printf(" (%s)", info);
	}
	printf("\n");
}

void fs_image_print_crc(struct fs_header_v1_0 *fsh_parent, struct fs_header_v1_0 *fsh, uint offs, int level)
{
	struct index_info idx_info;
	char info[MAX_DESCR_LEN + 1];
	u32 *pcs;
	bool crc_valid = false;
	int i;

	if(!fsh_parent)
		fsh_parent = fsh;

	pcs = (u32 *)&fsh->type[12];
	fs_image_find(fsh_parent, fsh->type, fsh->param.descr, &idx_info);
	if (fs_image_check_crc32_offset(fsh, idx_info.offset) >= 0)
		crc_valid = true;

	/* Show info for this image */
	printf("0x%08x ", *pcs);
	crc_valid ? puts("okay") : puts("fail");
	puts(" ");

	for (i = 0; i < level; i++)
#ifdef __UBOOT__
		putc(' ');
#else
		printf(" ");
#endif

	if (fsh->type[0]) {
		memcpy(info, fsh->type, MAX_TYPE_LEN);
		info[MAX_TYPE_LEN] = 0;
		printf(" %s", info);
	}

	if ((fsh->info.flags & FSH_FLAGS_DESCR) && fsh->param.descr[0]) {
		memcpy(info, fsh->param.descr, MAX_DESCR_LEN);
		info[MAX_DESCR_LEN] = 0;
		printf(" (%s)", info);
	}
	puts("\n");
}

void fs_image_parse_index_image(enum parse_type ptype,
		struct fs_header_v1_0 *fsh_parent, ulong addr, uint offs,
		int level, uint remaining)
{
	struct fs_header_v1_0 *idx_fsh;
	uint num_images;
	int i;

	idx_fsh = (struct fs_header_v1_0 *)(addr + offs);
	num_images = fs_image_index_get_n(idx_fsh);

	if(ptype == PARSE_CONTENT)
		fs_image_print_line(idx_fsh, offs, level);
	else if(ptype == PARSE_CHECKSUM)
		fs_image_print_crc(fsh_parent, idx_fsh, offs, level);

	/* skip index image */
	offs += fs_image_get_size(idx_fsh, true);
	remaining -= fs_image_get_size(idx_fsh, true);	
	level++;

	for(i=1; i<=num_images; i++){
		if(fs_image_is_fs_image(&idx_fsh[i])){
			if(ptype == PARSE_CONTENT)
				fs_image_print_line(&idx_fsh[i], offs, level);
			else if(ptype == PARSE_CHECKSUM)
				fs_image_print_crc(fsh_parent, &idx_fsh[i], offs, level);

			/* Find next underlying subimage */
			fs_image_parse_image(ptype, addr, offs, level + 1);
			offs += fs_image_get_size(&idx_fsh[i], false);
			remaining -= fs_image_get_size(&idx_fsh[i], false);
		}else {
			continue;
		}
	}

	if(ptype == PARSE_CONTENT && remaining > 0) {
		level--;
		printf("%08x %08x", offs, remaining);
		for (i = 0; i < level; i++)
#if __UBOOT__
				putc(' ');
#else
				printf(" ");
#endif
		printf(" [padding/unknown data]\n");
	}
}

void fs_image_parse_subimage(enum parse_type ptype,
		ulong addr, uint offs, int level, uint remaining)
{
	struct fs_header_v1_0 *fsh;
	uint size;
	bool had_sub_image = false;
	int i;

	while (remaining > 0) {
		fsh = (struct fs_header_v1_0 *)(addr + offs);
		if (fs_image_is_fs_image(fsh)) {
			had_sub_image = true;

			/* Print line and find next underlying subimage */
			fs_image_parse_image(ptype, addr, offs, level);
			size = fs_image_get_size(fsh, true);
		} else {
			size = remaining;
			if (had_sub_image && ptype == PARSE_CONTENT) {
				printf("%08x %08x", offs, size);
				for (i = 0; i < level; i++)
#if __UBOOT__
					putc(' ');
#else
					printf(" ");
#endif
				printf(" [padding/unknown data]\n");
			}
		}

		offs += size;
		remaining -= size;
	}
}

void fs_image_parse_image(enum parse_type ptype, ulong addr, uint offs, int level)
{
	struct fs_header_v1_0 *fsh = (struct fs_header_v1_0 *)(addr + offs);
	struct fs_header_v1_0 *fsh_sub;
	uint remaining;
	uint extra_size;
	int i;

	if(!fs_image_is_fs_image(fsh))
		return;

	extra_size = fs_image_get_extra_size(fsh);
	remaining = fs_image_get_size(fsh, false);

	if(ptype == PARSE_CONTENT){
		fs_image_print_line(fsh, offs, level);

		offs += FSH_SIZE;
		if(extra_size){
			printf("%08x %08x", offs, extra_size);
			for (i = 0; i < level; i++)
#ifdef __UBOOT__
					putc(' ');
#else
					printf(" ");
#endif
			printf(" %s\n", "[header/extra data]");
		}
	}else if(ptype == PARSE_CHECKSUM){
		fs_image_print_crc(NULL, fsh, offs, level);
		offs += FSH_SIZE;
	}

	offs += extra_size;
	remaining -= extra_size;
	level++;

	fsh_sub = (struct fs_header_v1_0 *)(addr + offs);
	if(!fs_image_is_fs_image(fsh_sub))
		return;

	if(fs_image_is_index(fsh_sub)){
		fs_image_parse_index_image(ptype, fsh, addr, offs, level, remaining);
	} else {
		fs_image_parse_subimage(ptype, addr, offs, level, remaining);
	}
}

/* Set all fields of the F&S header */
static void fs_image_set_header(struct fs_header_v1_0 *fsh, const char *type,
				const char *descr, uint size, uint fsh_flags)
{
	/* Set basic members */
	memset(fsh, 0, FSH_SIZE);
	fsh->info.magic[0] = 'F';
	fsh->info.magic[1] = 'S';
	fsh->info.magic[2] = 'L';
	fsh->info.magic[3] = 'X';
	fsh->info.version = 0x10;
#if __UBOOT__
	strncpy(fsh->type, type, sizeof(fsh->type));
#else
	memcpy(fsh->type, type, sizeof(fsh->type));
#endif
	strncpy(fsh->param.descr, descr, sizeof(fsh->param.descr));

	/* Set size, flags and padsize, calculate CRC32 if requested */
	fs_image_update_header(fsh, size, fsh_flags);
}


struct fs_header_v1_0 *fs_image_find_index(struct fs_header_v1_0 *fsh_idx,
		const char *type,
		const char *descr,
		struct index_info *idx_info)
{
	uint img_offset = fs_image_get_size(fsh_idx, false);
	uint num_images = fs_image_index_get_n(fsh_idx);
	int i;

	if(fs_image_match(fsh_idx, type, descr))
		return fsh_idx;

	for (i = 1; i <= num_images; i++){
		img_offset -= FSH_SIZE;
		if(!fs_image_is_fs_image(&fsh_idx[i]))
			continue;

		/* search F&S HEADER within Image blob */
		if(!fs_image_match(&fsh_idx[i], type, descr)){
			void *img_blob;
			img_blob = (void *)((ulong)&fsh_idx[i] + img_offset);
			img_blob = fs_image_find(img_blob, type, descr, idx_info);
			if(img_blob)
				return img_blob;

			img_offset += fs_image_get_size(&fsh_idx[i], false);
			continue;
		}

		break;
	}

	if(i > num_images)
		return NULL;

	if(idx_info)
	{
		idx_info->fsh_idx = fsh_idx;
		idx_info->fsh_idx_entry = &fsh_idx[i];
		idx_info->offset = img_offset;
	}
	return &fsh_idx[i];
}

static void fs_image_region_create(struct region_info *ri,
				   struct storage_info *si,
				   struct sub_info *sub)
{
	ri->si = si;
	ri->sub = sub;
	ri->count = 0;
}

/**
 * Return pointer to the header of the given sub-image or NULL if not found
 * @param *fsh: fs header to search for.
 * @param *type: type to search for
 * @param *decr: descr to search for or NULL
 * @param *idx_info: struct holds additional infos if fsh is index. NULL is allowed.
 * @return ptr to fsh or NULL if not found
 */
struct fs_header_v1_0 *fs_image_find(struct fs_header_v1_0 *fsh,
		const char *type,
		const char *descr,
		struct index_info *idx_info)
{
	struct fs_header_v1_0 *fsh_found;
	uint size;
	uint extra_size;
	uint remaining;

	if(idx_info){
		idx_info->fsh_idx = NULL;
		idx_info->fsh_idx_entry = NULL;
		idx_info->offset = 0;
	}

	if (!fs_image_is_fs_image(fsh))
			return NULL;

	if (fs_image_match(fsh, type, descr))
			return fsh;

	extra_size = fs_image_get_extra_size(fsh);
	remaining = fs_image_get_size(fsh, false);
	remaining -= extra_size;

	/* Get first subimg */
	fsh++;
	fsh = (void *)((ulong)fsh + extra_size);
	while (remaining > 0) {
		if (!fs_image_is_fs_image(fsh)){
			return NULL;
		}

		if (fs_image_match(fsh, type, descr))
			return fsh;

		if(fs_image_is_index(fsh)){
			/* Search iterative:
			 * a combination of SUB and INDEX images is not
			 * supportet. An Image can have ether a SUB
			 * structure, or INDEX structure. If an indexed
			 * image_blob is F&S Image, then this will be checked
			 * by fs_image_find_index(); 
			 */
			return fs_image_find_index(fsh, type, descr, idx_info);
		}

		/* Search recursively */
		fsh_found = fs_image_find(fsh, type, descr, idx_info);
		if (fsh_found)
			return fsh_found;

		/* Go to next sub-image */
		size = fs_image_get_size(fsh, true);
		fsh = (void *)((ulong)fsh + size);
		remaining -= size;
	}

	return NULL;
}

/*
 * Add a subimage with any format to the region. Return offset for next
 * subimage or 0 in case of error.
 */
static void fs_image_region_add_raw(
	struct region_info *ri, void *img, const char *type, const char *descr,
	uint woffset, uint flags, uint size)
{
	struct sub_info *sub;

	sub = &ri->sub[ri->count++];
	sub->type = type;
	sub->descr = descr;
	sub->img = img;
	sub->size = size;
	sub->offset = woffset;
	sub->flags = flags;

	debug("- %s(%s): 0x%08lx -> offset 0x%x size 0x%x\n", type, descr,
	      (ulong)img, woffset, size);
}

/*
 * Add a subimage with given data to the region. Return offset for next
 * subimage or 0 in case of error.
 */
static uint fs_image_region_add(
	struct region_info *ri, struct fs_header_v1_0 *fsh, const char *type,
	const char *descr, uint woffset, uint flags)
{
	uint size;

	size = fs_image_get_size(fsh, true);
	if (!(flags & SUB_HAS_FS_HEADER)) {
		fsh++;
		size -= FSH_SIZE;
	}

	if (woffset + size > ri->si->size) {
		printf("%s does not fit into target slot\n", type);
		return 0;
	}

	fs_image_region_add_raw(ri, fsh, type, descr, woffset, flags, size);

	return woffset + size;
}

/* Show status after handling a subimage */
static void fs_image_show_sub_status(int err)
{
	switch (err) {
	case 0:
		printf(" OK\n");
		break;
	case 1:
		printf(" BAD BLOCKS\n");
		break;
	default:
		printf(" FAILED (%d)\n", err);
		break;
	}
}

/* Show status after saving an image and return CMD_RET code */
static int fs_image_show_save_status(int failed, const char *type)
{
	printf("\nSaving %s ", type);

	/* Each bit identifies a copy that failed */
	if (!failed) {
		puts("complete\n");
		return 0;
	}

	if (failed != 3) {
		puts("incomplete!\n\n"
		     "*** WARNING! One copy failed, the system is unstable!\n");
		return 0;
	}

	printf("\nFAILED!\n\n"
	       "*** ATTENTION!\n"
	       "*** Do not switch off or restart the board before you have\n"
	       "*** installed a working %s version. Otherwise the board will\n"
	       "*** most probably fail to boot.\n", type);

	return -EINVAL;
}

#ifndef __UBOOT__
	int confirm_yesno(void) {
		char input[8];
		if(!fgets(input, sizeof(input), stdin)) {
			return 0;
		}
		input[strcspn(input, "\n")] = '\0';
		for(char *p = input; *p; ++p) {
			*p = tolower((unsigned char) *p);
		}
		if((strcmp(input, "y") == 0) || (strcmp(input, "yes") == 0)) {
			return 1;
		}
		return 0;
	}
#endif

static int fs_image_confirm(void)
{
	int yes;

	puts("Are you sure? [y/N] ");
	yes = confirm_yesno();
	if (!yes)
		printf("Aborted by user, nothing was changed %x\n", yes);

	return yes;
}

static int _fs_image_get_start_copy(const char *img_type)
{
	u32 bstage;
	int start_copy = 0;
	int ret;

	ret = get_bootrom_bootstage(&bstage);
	if(ret){
		printf("Failed to get bootstage from bootrom, assume Primary\n");
		bstage = BT_STAGE_PRIMARY;
	}

	switch (bstage) {
	case BT_STAGE_PRIMARY:
		start_copy = 1;
		break;
	case BT_STAGE_SECONDARY:
		start_copy = 0;
		break;
	default:
		start_copy = 1;
		break;
	}

	printf("Booted from %s %s, so starting with copy %d\n",
	       start_copy ? "Primary" : "Secondary", img_type, start_copy);

	return start_copy;
	return 0;
}

static int fs_image_get_start_copy(void)
{
	return _fs_image_get_start_copy("SPL");
}

static int fs_image_get_start_copy_uboot(void)
{
	return _fs_image_get_start_copy("U-BOOT");
}

/* Invalidate the temp buffer read cache */
static void fs_image_drop_temp(struct flash_info *fi)
{
	fi->write_pos = 0;
	fi->bb_extra_offs = 0;
	memset(fi->temp, fi->temp_fill, fi->temp_size);
}

static int fs_image_fill_temp(struct flash_info *fi, uint base_offs, uint lim,
			      uint flags)
{
	int err;
	fi->write_pos = 0;

	debug("  - Fill temp from offs 0x%x\n", base_offs);
	err = fi->ops->read(fi, base_offs, fi->temp_size, lim, flags, fi->temp);
	if (err)
		return err;

	fi->base_offs = base_offs;
	return 0;
}

static int fs_image_load_sub(struct flash_info *fi, uint offs, uint size,
			     uint lim, uint flags, u8 *buf)
{
	int err;
	uint read_pos;
	uint base_offs;
	uint chunk_size;
	uint chunk_mask = fi->temp_size - 1;
	uint remaining = size;

	/*
	 * Step 1: If reading starts in the middle of a page/block and we do
	 * not have this page/block cached in the temp buffer yet, load the
	 * page/block to the temp buffer.
	 */
	base_offs = offs & ~chunk_mask;
	read_pos = offs & chunk_mask;
	if ((read_pos) && (!fi->write_pos || (base_offs != fi->base_offs))) {
		err = fs_image_fill_temp(fi, base_offs, lim, flags);
		if (err)
			return err;
	}

	/*
	 * Step 2: If the start of the data is already cached in the
	 * temp/buffer, take it from there.
	 */
	if (read_pos && (base_offs == fi->base_offs)) {
		chunk_size = fi->temp_size - read_pos;
		if (chunk_size > remaining)
			chunk_size = remaining;
		debug("  - Copy leading bytes from temp pos 0x%x size 0x%x"
		      " to 0x%lx\n", read_pos, chunk_size, (ulong)buf);
		memcpy(buf, fi->temp + read_pos, chunk_size);
		buf += chunk_size;
		offs += chunk_size;
		remaining -= chunk_size;
	}

	/*
	 * Step 3: Read the middle part consisting of full pages/blocks.
	 */
	chunk_size = remaining & ~chunk_mask;
	if (chunk_size) {
		debug("  - Read from offs 0x%x lim 0x%x size 0x%x to 0x%lx\n",
		      offs, lim, chunk_size, (ulong)buf);
		err = fi->ops->read(fi, offs, chunk_size, lim, flags, buf);
		if (err)
			return err;
		buf += chunk_size;
		offs += chunk_size;
		remaining -= chunk_size;
	}

	/*
	 * Step 4: Read the last page/block, from which we only need a part
	 * of, to the temp buffer and take the remaining bytes from there.
	 */
	if (remaining) {
		base_offs = offs & ~chunk_mask;
		err = fs_image_fill_temp(fi, base_offs, lim, flags);
		if (err)
			return err;

		read_pos = offs & chunk_mask;
		debug("  - Copy trailing bytes from temp pos 0x%x size 0x%x to"
		      " 0x%lx\n", read_pos, remaining, (ulong)buf);
		memcpy(buf, fi->temp + read_pos, remaining);
	}

	return 0;
}

static int fs_image_load_image(struct flash_info *fi,
			       const struct storage_info *si,
			       struct sub_info *sub)
{
	struct fs_header_v1_0 *fsh;
	void *copy0, *copy1;
	uint size0 = 0;
	int err;

	printf("Loading %s from %s\n", sub->type, fi->devname);

	/* Add room for FS header if image has none */
	fsh = sub->img;
	if (!(sub->flags & SUB_HAS_FS_HEADER))
		sub->img += FSH_SIZE;

	/* Load first copy; on error, sub->size is 0, i.e. copy1 == copy0 */
	copy0 = sub->img;
	err = fi->ops->load_image(fi, 0, si, sub);
	fs_image_show_sub_status(err);
	size0 = sub->size;
	sub->img += size0;

	/* Load second copy; this overwrites first copy if it had an error */
	copy1 = sub->img;
	err = fi->ops->load_image(fi, 1, si, sub);
	fs_image_show_sub_status(err);

	if (err && (copy0 == copy1)) {
		printf("  Error, cannot load %s\n", sub->type);
		return -ENOENT;
	} else if (err || (copy0 == copy1)) {
		printf("  Warning! One copy corrupted! Saving NBoot again may"
		       " fix this.\n");
	}

	if (!err) {
		if (copy0 == copy1)
			size0 = sub->size;
		else if ((size0 != sub->size) || memcmp(copy0, copy1, size0))
			printf("  Warning! Images differ, taking copy 0\n");
	}

	/* Align the size to 16 Bytes, pad with 0 and fill FS header */
	sub->size = ALIGN(size0, 16);
	sub->img = copy0 + sub->size;
	memset(copy0 + size0, 0, sub->size - size0);

	if (!(sub->flags & SUB_HAS_FS_HEADER))
		fs_image_set_header(fsh, sub->type, sub->descr, size0, 0);

	return 0;
}

/* Check CRC32 for an environment of given size */
static int fs_image_check_env_crc32(void *env, uint size)
{
	u32 expected;
	/*
	 * Check CRC32 of environment. The enviroment starts with the
	 * CRC32 checksum. In case of redundant env, there is an additional
	 * status byte. Then follows the environment itself.
	 */
	expected = *(u32 *)env;
	if (crc32(0, env + 5, size - 5) == expected)
		return 0;
#if 0
	/* Check for non-redundant environment */
	if (crc32(0, env + 4, size - 4) == expected)
		return -EILSEQ;
#endif

	return 0;
}

/* Load one ENV */
static int fs_image_load_env(struct flash_info *fi, struct storage_info *si,
			     void *env_addr, int copy)
{
	uint size = 0;
	int err;
	struct sub_info sub;

	sub.type = copy ? "ENV-RED" : "ENV";
	sub.descr = fs_image_get_arch();
	sub.img = env_addr + FSH_SIZE;
	sub.offset = 0;
	sub.flags = SUB_IS_ENV;

	err = fi->ops->load_image(fi, copy, si, &sub);
	fs_image_show_sub_status(err);
	if (err)
		return err;

	//### TODO: Change env size if new one differs

	size = sub.size;
	sub.size = ALIGN(size, 16);
	memset(sub.img + size, 0, sub.size - size);
	fs_image_set_header(env_addr, sub.type, sub.descr, size, 0);

	return 0;
}


/* Load U-Boot to given address
 * If SUB_HAS_FS_HEADER is not set as sub_flags, then
 * fs_image_load_image() will create a new one. If the
 * image actually has a header in this case
 * (new U-BOOT versions are stored with header), it is used for
 * CRC32 checking, then removed, and the own new header is used
 * instead.
 */
static int fs_image_load_uboot(struct flash_info *fi, struct nboot_info *ni,
				void *addr, uint sub_flags)
{
	struct sub_info sub;
	struct fs_header_v1_0 *uboot_fsh = addr;
	uint fsh_flags = 0;
	int err;

#if __UBOOT__
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	sub.type = "U-BOOT-INFO";
	fsh_flags |= (FSH_FLAGS_INDEX | FSH_FLAGS_EXTRA);
#else
	sub.type = "U-BOOT";
#endif
#else
	sub.type = "U-BOOT-INFO";
	fsh_flags |= (FSH_FLAGS_INDEX | FSH_FLAGS_EXTRA);
#endif
	sub.descr = fs_image_get_arch();
	sub.img = addr;
	sub.offset = 0;
	sub.flags |= sub_flags;

	err = fs_image_load_image(fi, &ni->uboot, &sub);
	if (err)
		return err;

	/* Compute CRC32 if it is missing */
	if(!(uboot_fsh->info.flags & (FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE)))
		fs_image_update_header((void *)addr, sub.size,
			       FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE | fsh_flags);
	return 0;
}

/* Flush the temp buffer to flash */
static int fs_image_flush_temp(struct flash_info *fi, uint lim, uint flags)
{
	int err;

	if (!fi->write_pos)
		return 0;

	debug("  - Flush temp (filled to pos 0x%x) to offs 0x%x\n",
	      fi->write_pos, fi->base_offs);
	err = fi->ops->write(fi, fi->base_offs, fi->temp_size, lim, flags,
			     fi->temp);
	fi->write_pos = 0;
	memset(fi->temp, fi->temp_fill, fi->temp_size);

	return err;
}

static int fs_image_save_sub(struct flash_info *fi, uint offs, uint size,
			     uint lim, uint flags, u8 *buf)
{
	int err;
	uint write_pos;
	uint base_offs;
	uint chunk_size;
	uint chunk_mask = fi->temp_size - 1;
	uint remaining = size;

	debug("\n");
	/*
	 * Step 1: If the temp buffer was used and we are not continuing in
	 * the same page/block, write back the temp buffer first.
	 */
	base_offs = offs & ~chunk_mask;
	if (fi->write_pos && (base_offs != fi->base_offs)) {
		err = fs_image_flush_temp(fi, lim, 0);
		if (err)
			return err;
	}

	/*
	 * Step 2: Handle the beginning of the sub-image if it does not start
	 * on a page/block boundary. Write the beginning up to the next
	 * page/block boundary in the temp buffer and write back the temp
	 * buffer. If the data is very small so that it does not fill the temp
	 * buffer completely, then this is handled below in Step 4 instead.
	 */
	write_pos = offs & chunk_mask;
	chunk_size = fi->temp_size - write_pos;
	if (write_pos && (remaining >= chunk_size)) {

		/* Fill TEMP with DATA from FLASH */
		if(!fi->write_pos || (base_offs != fi->base_offs)) {
			err = fs_image_fill_temp(fi, base_offs, lim, flags);
			if(err)
				return err;
		}

		fi->base_offs = base_offs;
		fi->write_pos = write_pos + chunk_size;
		debug("  - Copy leading bytes from 0x%lx size 0x%x"
			" to temp pos 0x%x\n", (ulong)buf, chunk_size,
			write_pos);
		memcpy(fi->temp + write_pos, buf, chunk_size);
		err = fs_image_flush_temp(fi, lim, flags);
		if (err)
			return err;
		buf += chunk_size;
		offs += chunk_size;
		remaining -= chunk_size;
	}

	/*
	 * Step 3: Write the middle part consisting of full pages/blocks.
	 */
	chunk_size = remaining & ~chunk_mask;
	if (chunk_size) {
		debug("  - Write from 0x%lx size 0x%x to offs 0x%x lim 0x%x\n",
		      (ulong)buf, chunk_size, offs, lim);

		err = fi->ops->write(fi, offs, chunk_size, lim, flags, buf);
		if (err)
			return err;
		buf += chunk_size;
		offs += chunk_size;
		remaining -= chunk_size;
	}

	/*
	 * Step 4: Put the remaining part, which does not fill a full
	 * page/block anymore, in the temp buffer. If SUB_SYNC is not given,
	 * this will be written in one of the next sub-images with SUB_SYNC
	 * flags, or call fs_image_flush_temp() at end of save process.
	 */
	if (remaining) {
		base_offs = offs & ~chunk_mask;
		write_pos = offs & chunk_mask;

		if(!fi->write_pos || (base_offs != fi->base_offs)){
			/* Fill TEMP with DATA from FLASH */
			err = fs_image_fill_temp(fi, base_offs, lim, flags);
			if(err)
				return err;
		}

		debug("  - Copy trailing bytes from 0x%lx size 0x%x to temp"
		      " pos 0x%x\n", (ulong)buf, remaining, write_pos);
		memcpy(fi->temp + write_pos, buf, remaining);

		fi->write_pos += fi->write_pos + remaining;
		fi->base_offs = base_offs;
		if (flags & SUB_SYNC) {
			debug("  - SYNC\n");
			err = fs_image_flush_temp(fi, lim, flags);
			if (err)
				return err;
		}
	}

	return 0;
}

/* Save the given region to flash */
static int fs_image_save_region(struct flash_info *fi, int copy,
				struct region_info *ri)
{
	void *buf;
	int err;
	struct sub_info *s;
	struct storage_info *si = ri->si;
	uint lim = si->start[copy] + si->size;
	uint offset;
	uint size;
	uint temp_size = fi->temp_size;
	bool pass2;
	const char *action;

	err = fi->ops->prepare_region(fi, copy, si);
	if (err)
		return err;

repeat:
	/* Clear the temp buffer (write cache) */
	fs_image_drop_temp(fi);

	err = fi->ops->invalidate(fi, copy, si);
	if (err)
		return err;

	/*
	 * Write region in two passes:
	 *
	 * Pass 1:
	 * Write everything of the image but the first page/block with the
	 * header. If this is interrupted (e.g. due to a power loss), then the
	 * image will not be seen as valid when loading because of the missing
	 * header. So there is no problem with half-written files.
	 *
	 * Pass 2:
	 * Write only the first page/block with the header; if this succeeds,
	 * then we know that the whole image is completely written. If this is
	 * interrupted, then loading will fail either because of a bad header
	 * or because of a bad ECC. So again this prevents loading files that
	 * are not fully written.
	 */
	pass2 = false;
	do {
		fi->bb_extra_offs = 0;
		debug("  - Pass %d\n", pass2 ? 2 : 1);
		action = "Writing";
		for (s = ri->sub; s < ri->sub + ri->count; s++) {
			offset = s->offset;
			size = s->size;
			buf = s->img;

			if (!pass2) {
				/* Skip image completely? */
				if (offset + size < temp_size)
					continue;

				/* Skip only first part of image? */
				if (offset < temp_size) {
					size -= temp_size - offset;
					buf += temp_size - offset;
					offset = temp_size;
				}
			} else {
				/* Behind first page/block, i.e. done? */
				if (offset >= temp_size)
					break;

				/* Write only first part of image? */
				if (offset + size > temp_size) {
					size = temp_size - offset;
					action = "Completing";
				}
			}
			offset += si->start[copy];

			printf("  %s %s at offset 0x%08x size 0x%x...",
			       action, s->type, offset, size);

			err = fs_image_save_sub(fi, offset, size, lim,
						s->flags, buf);
			fs_image_show_sub_status(err);
			if (err == 1) {
				/* We had new bad blocks when writing */
				printf("Repeating copy %d\n", copy);
				goto repeat;
			}
			if (err)
				return err;
		}
		pass2 = !pass2;
	} while (pass2);

	return 0;
}

static int fs_image_save_uboot(struct flash_info *fi, struct region_info *ri)
{
	int failed;
	int copy, start_copy;

	failed = 0;
	start_copy = fs_image_get_start_copy_uboot();
	copy = start_copy;
	do {
		printf("\nSaving copy %d to %s:\n", copy, fi->devname);
		if (fs_image_save_region(fi, copy, ri))
			failed |= BIT(copy);
		copy = 1 - copy;
	} while (copy != start_copy);

	return failed;
}

static int fs_image_get_boot_dev(void *fdt, enum boot_device *boot_dev,
				 const char **boot_dev_name)
{
	int offs;
	int rev_offs;
	const char *boot_dev_prop;

	offs = fs_image_get_board_cfg_offs(fdt);
	if (offs < 0) {
		puts("Cannot find BOARD-CFG\n");
		return -ENOENT;
	}
	rev_offs = fs_image_get_board_rev_subnode(fdt, offs);
	boot_dev_prop = fs_image_getprop(fdt, offs, rev_offs, "boot-dev", NULL);
	if (boot_dev_prop < 0) {
		puts("Cannot find boot-dev in BOARD-CFG\n");
		return -ENOENT;
	}
	*boot_dev = fs_board_get_boot_dev_from_name(boot_dev_prop);
	if (*boot_dev == UNKNOWN_BOOT) {
		printf("Unknown boot device %s in BOARD-CFG\n", boot_dev_prop);
		return -EINVAL;
	}

	*boot_dev_name = fs_board_get_name_from_boot_dev(*boot_dev);

	return 0;
}

/* Check boot device; Return 0: OK, 1: Not fused yet, <0: Error */
static int fs_image_check_boot_dev_fuses(enum boot_device boot_dev,
					 const char *action)
{
	enum boot_device boot_dev_fuses;

	boot_dev_fuses = fs_board_get_boot_dev_from_fuses();
	if (boot_dev_fuses == boot_dev)
		return 0;		/* Match, no change */

	if ((boot_dev_fuses == USB_BOOT)
	|| (boot_dev_fuses == USB2_BOOT))
		return 1;		/* Not fused yet */

	printf("Error: New BOARD-CFG wants to boot from %s but board is\n"
	       "already fused for %s. Refusing to %s this configuration.\n",
	       fs_board_get_name_from_boot_dev(boot_dev),
	       fs_board_get_name_from_boot_dev(boot_dev_fuses), action);

	return -EINVAL;
}

static int fs_image_check_index_crc32(struct fs_header_v1_0 *fsh_idx)
{
	uint img_offset = fs_image_get_size(fsh_idx, true);
	uint num_images = fs_image_index_get_n(fsh_idx);
	int i;
	int err = 0;

	for(i=1; i<= num_images; i++){
		void *img_blob;
	
		img_offset -= FSH_SIZE;

		if(!fs_image_is_fs_image(&fsh_idx[i]))
			continue;

		err = fs_image_check_crc32_offset(&fsh_idx[i], img_offset);
		fs_image_print_crc32_status(&fsh_idx[i], err);
		if(err)
			return err;

		img_blob = (void *)((ulong)(fsh_idx) + img_offset);
		if(fs_image_is_fs_image(img_blob))
			err = fs_image_check_all_crc32(img_blob);

		if(err)
			return err;

		img_offset += fs_image_get_size(&fsh_idx[i], false);
	}

	return err;
}

/* Check CRC32 from image and all sub-images */
static int fs_image_check_all_crc32(struct fs_header_v1_0 *fsh)
{
	uint size;
	uint remaining;
	uint extra_size;
	int err;

	debug("  - %s", fsh->type);
	err = fs_image_check_crc32(fsh);
	fs_image_print_crc32_status(fsh, err);
	if(err)
		return err;

	extra_size = fs_image_get_extra_size(fsh);
	remaining = fs_image_get_size(fsh++, false);
	remaining -= extra_size;

	while (remaining > 0) {
		if (!fs_image_is_fs_image(fsh))
			break;

		/* check indexed image or recursivly */
		if(fs_image_is_index(fsh))
			err = fs_image_check_index_crc32(fsh);
		else
			err = fs_image_check_all_crc32(fsh);

		if (err)
			return err;

		/* Go to next sub-image */
		size = fs_image_get_size(fsh, true);
		fsh = (void *)fsh + size;
		remaining -= size;
	}

	return 0;
}

static int fs_image_validate_signed(struct fs_header_v1_0 *fsh){
	if(!fs_image_is_valid_signature(fsh)){
		puts("Error: Invalid signature, refusing to save\n");
		return -EILSEQ;
	}

	puts("Signature OK\n");
	return 0;
}

/* Validate an image, either check signature or CRC32; 0: OK, <0: Error */
static int fs_image_validate(struct fs_header_v1_0 *fsh, const char *type,
			     const char *descr, ulong addr)
{
	int err;
	if (!fs_image_match(fsh, type, descr)) {
		printf("Error: No %s image for %s found at address 0x%lx\n",
		       type, descr, addr);
		return -EINVAL;
	}

	if (fs_image_is_signed(fsh)) {
		printf("Found signed %s image at 0x%08lx\n", type, addr);

		return fs_image_validate_signed(fsh);
	}

	printf("Found unsigned %s image at 0x%08lx\n", type, addr);

	if (fs_board_is_closed()) {
		puts("\nError: Board is closed, refusing to save unsigned"
		     " image\n");
		return -EINVAL;
	}

	err = fs_image_check_crc32(fsh);
	fs_image_print_crc32_status(fsh, err);

	if(err >= 0)
		return 0;

	return err;
}

/* Get the full size of a FIT image, including all external images */
static int fs_image_get_size_from_fit(struct sub_info *sub, uint *size)
{
	void *fit = sub->img;
	uint fit_size = ALIGN(fit_get_size(fit), 4);
	int images;
	int node;
	int offs;
	int img_size;
	uint maxsize = fit_size;
	const void *dummy_data;
	size_t dummy_size;
	int err;

	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0)
		return -ENOENT;

		/* Parse all images to find the last one (with highest offset) */
	fdt_for_each_subnode(node, fit, images) {
		/* If image data is embedded, this will not increase size */
		if (!fit_image_get_data(fit, node, &dummy_data, &dummy_size))
			continue;
		/*
		 * If image data is external (given by data_position or
		 * data_offset, look for end of image data and keep highest
		 * value.
		 */
		if (fit_image_get_data_position(fit, node, &offs)) {
			if (fit_image_get_data_offset(fit, node, &offs))
				return -ENOENT;
			offs += fit_size;
		}
		err =  fit_image_get_data_size(fit, node, &img_size);
		if (err < 0)
			return -ENOENT;
		img_size = ALIGN(img_size, 4);
		offs += img_size;
		if ((uint)offs > maxsize)
			maxsize = (uint)offs;
	}

	*size = maxsize;

	return 0;
}

/* Get image length from F&S header or IVT (SPL, signed U-Boot) */
static int fs_image_get_size_from_fsh_or_ivt(struct sub_info *sub, uint *size)
{
	if (fs_image_is_fs_image(sub->img)) {
		struct fs_header_v1_0 *fsh = sub->img;

		/* Check image type and get image size from F&S header */
		if (!fs_image_match(fsh, sub->type, sub->descr))
			return -ENOENT;
		*size = fs_image_get_size(fsh, true);
#ifdef __UBOOT__
#if CONFIG_IS_ENABLED(IMX_HAB)
	} else {
		struct ivt *ivt = sub->img;
		struct boot_data *boot_data;

		/* Get image size from IVT/boot_data */
		if ((ivt->hdr.magic != IVT_HEADER_MAGIC)
		    || (ivt->boot != ivt->self + IVT_TOTAL_LENGTH))
			return -ENOENT;
		boot_data = (struct boot_data *)(ivt + 1);
		*size = boot_data->length;
#endif
#endif
	}

	return 0;
}

static struct fs_header_v1_0 *find_board_info_in_imx8_img(
					struct fs_header_v1_0 * fsh,
					bool force,
					const char *action)
{
	struct fs_header_v1_0 *cfg;
	const char *arch = fs_image_get_arch();
	int err;

	/* Authenticate signature or check CRC32 */
	err = fs_image_validate(fsh, "NBOOT", arch, (ulong)fsh);
	if (err)
		return NULL;
#if __UBOOT__
#if CONFIG_IS_ENABLED(IMX_HAB)
	else {
		if(fs_image_is_signed(fsh)){
			memcpy((void *)(fsh + 0x40), (void *)(fsh + 0x80), fsh->info.file_size_low + 0x2000);
		}
	}
#endif
#endif

	/* Look for BOARD-INFO subimage */
	cfg = fs_image_find(fsh, "BOARD-INFO", arch, NULL);
	if (!cfg) {
		/* Fall back to BOARD-CONFIGS for old NBoot variants */
		cfg = fs_image_find(fsh, "BOARD-CONFIGS", arch, NULL);
		if (!cfg) {
			printf("No BOARD-INFO/CONFIGS found for %s\n", arch);
			return NULL;
		}
	}

	return cfg;
}

static struct fs_header_v1_0 *find_board_info_in_cntr_imgs(
					struct fs_header_v1_0 *fsh,
					bool force, const char *action)
{
	struct fs_header_v1_0 *cfg = fsh;
	const char *arch = fs_image_get_arch();

	if (!fs_image_match(fsh, "BOOT-INFO", arch))
		return NULL;

	cfg = (void *)cfg + fs_image_get_size(cfg, true);

	if (!fs_image_match(cfg, "BOARD-ID", NULL))
		return NULL;

	cfg = (void *)cfg + fs_image_get_size(cfg, true);

	if (fs_image_validate(cfg, "BOARD-INFO", arch, (ulong)cfg))
		return NULL;

	return cfg;
}

/*
 * Get pointer to BOARD-CFG image that is to be used and to NBOOT part
 * Returns: <0: error; 0: aborted by user; 1: same ID; 2: new ID
 */
static int fs_image_find_board_cfg(ulong addr, bool force,
				   const char *action,
				   struct fs_header_v1_0 **used_cfg,
				   struct fs_header_v1_0 **nboot)
{
	struct fs_header_v1_0 *fsh = (struct fs_header_v1_0 *)addr;
	struct fs_header_v1_0 *cfg = NULL;
	const char *id;
	const char *arch = fs_image_get_arch();
	const char *nboot_version;
	void *fdt;
	uint size, remaining, extra_size;
	int ret = 1;

	*used_cfg = NULL;


	if (!fs_image_is_fs_image(fsh)) {
		printf("No F&S image found at address 0x%lx\n", addr);
		return -ENOENT;
	}

	/* In case of an NBoot image with prepended BOARD-ID, use this ID */
	if (fs_image_match(fsh, "BOARD-ID", NULL)) {
		const char *old_id = fs_image_get_board_id();
		char new_id[MAX_DESCR_LEN + 1];

		memcpy(new_id, fsh->param.descr, MAX_DESCR_LEN);
		new_id[MAX_DESCR_LEN] = '\0';
		if (strncmp(new_id, old_id, MAX_DESCR_LEN)) {
#if __UBOOT__
#if CONFIG_IS_ENABLED(FS_SECURE_BOOT) && CONFIG_IS_ENABLED(IMX_HAB)
			if (imx_hab_is_enabled()) {
				printf("Error: Current board is %s and board"
				       " is closed\nRefusing to %s for %s\n",
				       old_id, action, new_id);
				return -EINVAL;
			}
#endif
#endif
			printf("Warning! Current board is %s but you will\n"
			       "%s for %s\n", old_id, action, new_id);
			if (!force && !fs_image_confirm()) {
				return 0; /* used_cfg == NULL in this case */
			}

			/* Set this BOARD-ID as compare_id */
			fs_image_set_compare_id(fsh->param.descr);
		}
		fsh++;
	}

	id = fs_image_get_board_id();

	/* In case of an imx8m NBoot image */
	if (fs_image_match(fsh, "NBOOT", arch))
		cfg = find_board_info_in_imx8_img(fsh, force, action);
	else if (fs_image_match(fsh, "BOOT-INFO", arch))
		cfg = find_board_info_in_cntr_imgs(fsh, force, action);
	else
		return -EINVAL;

	if(!cfg)
		return -ENOENT;

	extra_size = fs_image_get_extra_size(cfg);
	remaining = fs_image_get_size(cfg, false);
	remaining -= extra_size;

	cfg = (void *)cfg + FSH_SIZE + extra_size;

	while (1) {
		if (!remaining || !fs_image_is_fs_image(cfg)) {
			printf("No BOARD-CFG found for BOARD-ID %s\n", id);
			return -ENOENT;
		}
		if (fs_image_match_board_id(cfg))
			break;
		size = fs_image_get_size(cfg, true);
		remaining -= size;
		cfg = (struct fs_header_v1_0 *)((void *)cfg + size);
	}

	/* Get and show NBoot version as noted in BOARD-CFG */
	fdt = fs_image_find_cfg_fdt(cfg);

	nboot_version = fs_image_get_nboot_version(fdt);
	if (!nboot_version) {
		printf("Unknown NBOOT version, refusing to %s\n", action);
		return -EINVAL;
	}
	printf("Found NBOOT version %s\n", nboot_version);

	*used_cfg = cfg;
	if (nboot)
		*nboot = fsh;

	return ret;
}

/* Get flash information for given boot device */
static int fs_image_get_flash_info(struct flash_info *fi, void *fdt)
{
	int err;

	memset(fi, 0, sizeof(struct flash_info));

	err = fs_image_get_boot_dev(fdt, &fi->boot_dev, &fi->boot_dev_name);
	if (err)
		return err;

	/* Prepare flash information from where to load */
	switch (fi->boot_dev) {
#if __UBOOT__
#ifdef CONFIG_NAND_MXS
	case NAND_BOOT:
		fi->mtd = get_nand_dev_by_index(0);
		if (!fi->mtd) {
			puts("NAND not found\n");
			return -ENODEV;
		}
		fi->ops = &flash_ops_nand;
		break;
#endif

#ifdef CONFIG_MMC
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
//TODO: DD bessere emulation von fi
		err = blk_get_device(UCLASS_MMC, fi->boot_dev - MMC1_BOOT, &fi->bdev);
		if (err) {
			printf("blkdev %d not found\n", fi->boot_dev);
			return -ENODEV;
		}

		fi->ops = &flash_ops_mmc_generic;
		break;
#endif
#else
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
		fi->ops = &flash_ops_mmc_generic;
		break;
#endif
	default:
		printf("Cannot handle %s boot device\n", fi->boot_dev_name);
		return -ENODEV;
		break;
	}

	fi->ops->get_flash(fi);

	fi->temp = malloc(fi->temp_size);
	if (!fi->temp) {
		puts("Cannot allocate temp buffer\n");
		return -ENOMEM;
	}
	memset(fi->temp, fi->temp_fill, fi->temp_size);

	return 0;
}

struct _image_list{
	struct fs_header_v1_0 *fsh;
	struct _image_list *next;
};

/* append fsh at end of img list */
static int append_image_list(struct _image_list *img_list, struct fs_header_v1_0 *fsh)
{
	struct _image_list *ptr = img_list;
	struct _image_list *next_entry;

	next_entry = malloc(sizeof(struct _image_list));
	if(!next_entry)
		return -ENOMEM;

	next_entry->fsh = fsh;
	next_entry->next = NULL;

	while(ptr->next){
		ptr = ptr->next;
	}

	ptr->next = next_entry;

	return 0;
}

/* remove next entry in image list */
static void remove_next_image(struct _image_list *ptr)
{
	struct _image_list *tmp = ptr->next;

	ptr->next = ptr->next->next;
	free(tmp);
}

static void free_image_list(struct _image_list *img_list){
	struct _image_list *tmp;

	while(img_list){
		tmp = img_list;
		img_list = img_list->next;
		free(tmp);
	}
}

/* Create list of padded Images */
static int create_image_list(struct _image_list **img_list,
		ulong addr, uint *file_size)
{
	struct fs_header_v1_0 *fsh = (void *)addr;
	uint size = 0;
	int ret = 0;

	*img_list = malloc(sizeof(struct _image_list));
	if(!(*img_list))
		return -ENOMEM;

	/* set first entry */
	if(!fs_image_is_fs_image(fsh)){
		free(*img_list);
		return -EINVAL;
	}

	(*img_list)->fsh = fsh;
	(*img_list)->next = NULL;

	addr += fs_image_get_size(fsh, true);
	size += fs_image_get_size(fsh, true);
	fsh = (void *)addr;

	/* set entries for padded images */
	while(fs_image_is_fs_image(fsh)){
		ret = append_image_list(*img_list, fsh);
		if(ret){
			free_image_list(*img_list);
			return ret;
		}
		addr += fs_image_get_size(fsh, true);
		size += fs_image_get_size(fsh, true);
		fsh = (void *)addr;
	}

	if(file_size)
		*file_size = size;

	return 0;
}

static bool is_img_list_valid(struct _image_list *img_list)
{
	struct _image_list *ptr;
	bool ret = true;

	if(!img_list)
		return false;

	ptr = img_list;

	while(ptr){
		if(fs_image_match(ptr->fsh, "BOARD-ID", NULL)) {
			ptr = ptr->next;
			continue;
		}

		if(fs_image_validate(ptr->fsh, ptr->fsh->type, NULL, (ulong)ptr->fsh))
			ret = false;

		ptr = ptr->next;
	}

	return ret;
}


static struct fs_header_v1_0 *get_fsh_from_list(struct _image_list *img_list,
		const char* type, const char* descr)
{
	struct _image_list *ptr = img_list;

	while(ptr){
		if(fs_image_match(ptr->fsh, type, descr))
			break;

		ptr = ptr->next;
	}

	if(!ptr)
		return NULL;

	return ptr->fsh;
}

static uint get_nboot_cntr_size(struct _image_list *img_list)
{
	struct _image_list *ptr = img_list;
	ulong size = 0;

	if(!img_list)
		return 0;

	/* skip BOOT-INFO */
	ptr = ptr->next;
	while(ptr){
		if(fs_image_match(ptr->fsh, "U-BOOT-INFO", NULL) ||
				fs_image_match(ptr->fsh, "ENV", NULL))
			break;

		size += fs_image_get_size(ptr->fsh, true);
		ptr = ptr->next;
	}

	return size;
}

/**
 * Unlike the loading method for imx8, the cntr method does not use nboot-infos
 * to load the complete boot firmware. This is because nboot-infos are not
 * fully available when booting with fastboot. Properties such as <>-start and
 * <>-size are determined dynamically during the boot process via mmc/nand.
 * To load images during fastboot or after an update, the 1KiB padding is used
 * to find images. All images including U-BOOT-INFO within 3MiB are searched
 * for.
 */
static int fsimage_cntr_load(ulong addr, bool load_uboot, int boot_hwpart)
{
	struct fs_header_v1_0 *fsh = (void *)addr;
	struct _image_list *img_list = NULL;
	struct container_hdr *cntr;
	struct boot_img_t *img_entry;
	const char *arch = fs_image_get_arch();
	struct flash_info fi;
	struct nboot_info ni;
	uint flash_offset = 0;
	ulong ram_offset = addr;
	uint size = 0x80000; // 512KiB
	uint filesize;
	uint lim;
	void *fdt;
	int i;
	int ret;

	fdt = fs_image_get_cfg_fdt();
	ret = fs_image_get_flash_info(&fi, fdt);
	if(ret)
		return -EINVAL;

	ret = fs_image_get_nboot_info(&fi, fdt, &ni, -1, false);
	if(ret){
		fs_image_put_flash_info(&fi);
		return -EINVAL;
	}

	if(load_uboot) {
		if(!ni.uboot.start[0] || !ni.uboot.start[1]){
			puts("Failed to load U-BOOT. "
					"Try to load complete Firmware");
			fs_image_put_flash_info(&fi);
			return -EINVAL;
		}

		ret = fs_image_load_uboot(&fi, &ni, (void *)addr, 0);
		if (ret){
			fs_image_put_flash_info(&fi);
			return -EINVAL;
		}

		printf("U-Boot successfully loaded to 0x%lx\n", addr);
		fs_image_put_flash_info(&fi);
		return 0;
	}

	fi.boot_hwpart = 0; //FORCE TO SET HWPART
	fi.ops->set_boot_hwpart(&fi, boot_hwpart);
	fs_image_set_header(fsh, "BOOT-INFO", arch, 0, 0);
	flash_offset = ni.spl.start[0];
	ram_offset += FSH_SIZE;
	lim = flash_offset + size;
	fi.ops->read(&fi, flash_offset, size, lim, 0, (void *)(ram_offset));

	flash_offset += size;
	ram_offset += size;

	/**
	 * BOOT-INFO provides two Container.
	 * search directly for the second container, which is 1KiB aligned
	 */
	for(i = 1; i < 8; i++)	{
		cntr = (void *)(fsh + 1);
		cntr = (void *)((ulong)cntr + (i * CONTAINER_HDR_ALIGNMENT));
		debug("search imx_cntr at 0x%lx\n", (ulong)cntr);
		if(valid_container_hdr(cntr))
			break;
	}

	if(i >= 8){
		fs_image_put_flash_info(&fi);
		puts("Failed to find BOOT-INFO Container\n");
		return -EINVAL;
	}
	debug("found cntr at 0x%lx\n", (ulong)cntr);
	filesize = i * CONTAINER_HDR_ALIGNMENT;
	filesize += get_container_size((ulong)cntr, NULL);
	filesize += 0x380; // PADDING TO NEXT FSH
	img_entry = (struct boot_img_t *)((ulong)cntr + sizeof(struct container_hdr));

	/* Update FSH and set Extra Data offset */
	fs_image_update_header(fsh, filesize,
			FSH_FLAGS_CRC32 | FSH_FLAGS_INDEX |FSH_FLAGS_EXTRA);
	fsh->param.p32[7] = i * CONTAINER_HDR_ALIGNMENT;
	fsh->param.p32[7] += img_entry->offset;

	/**
	 * Load all Images including U-Boot
	 * Search until 3MiB is loaded.
	 * U-Boot must be placed within 3MiB!.
	 */
	for (i = 0, fsh = NULL; i < 5; i++) {
		ret = create_image_list(&img_list, addr, &filesize);
		if(ret)
			break;

		fsh = get_fsh_from_list(img_list, "U-BOOT-INFO", NULL);
		if(fsh)
			break;

		/* Load next 512KiB and search again */
		free_image_list(img_list);
		img_list = NULL;
		lim = flash_offset + size;
		debug("load 0x%x at 0x%x into 0x%lx", size, flash_offset, ram_offset);
		fi.ops->read(&fi, flash_offset, size, lim, 0, (void *)(ram_offset));
		flash_offset += size;
		ram_offset += size;
	}

	if(!fsh){
		fs_image_put_flash_info(&fi);
		free_image_list(img_list);
		return -EINVAL;
	}

	debug("found U-BOOT at 0x%lx", (ulong)fsh);
	/* load rest of U-BOOT */
	if(filesize > flash_offset) {
		lim = filesize;
		debug("load 0x%x at 0x%x into %0lx", size, flash_offset, ram_offset);
		fi.ops->read(&fi, flash_offset, filesize - flash_offset,
				lim, 0, (void *)(ram_offset));
		flash_offset += filesize - flash_offset;
		ram_offset += filesize - flash_offset;
	}

	if(!is_img_list_valid(img_list)){
		puts("WARNING: Firmware is invalid\n");
		fs_image_put_flash_info(&fi);
		free_image_list(img_list);
		return -EINVAL;
	}

	fs_image_put_flash_info(&fi);
	free_image_list(img_list);
	return 0;
}

static int prepare_nboot_cntr_images(ulong addr, void *fdt_new,
		struct flash_info *fi, struct region_info *spl_ri,
		struct region_info *nboot_ri, struct region_info *uboot_ri,
		struct region_info *env_ri, struct nboot_info *ni_new)
{
	struct _image_list *img_list = NULL;
	struct _image_list *ptr;
	struct fs_header_v1_0 *tmp_fsh;
	struct nboot_info ni_old;
	const char *arch = fs_image_get_arch();
	const char board_id[MAX_DESCR_LEN + 1] = {0};
	const char *dram_type;
	void *fdt_old = fs_image_get_cfg_fdt();
 	ulong uboot_addr, env_addr;
	uint woffset;
	int offs = fs_image_get_board_cfg_offs(fdt_new);
 	int rev_offs = fs_image_get_board_rev_subnode(fdt_new, offs);
	uint file_size;
	bool need_uboot = false;
	bool need_env = false;
	int ret;

	/* get Board-ID from compare id */
	fs_image_get_compare_id((char *)board_id, MAX_DESCR_LEN + 1);

	/* --- Get a list of available Images to save into flash --- */
	ret = create_image_list(&img_list, addr, &file_size);
	if (ret)
		return ret;

	ptr = img_list;

	if(!fs_image_match(ptr->fsh, "BOOT-INFO", arch)) {
		free_image_list(img_list);
 		return -EINVAL;
	}

	ptr = ptr->next;

	if(ptr && !fs_image_match(ptr->fsh, "BOARD-ID", NULL)) {
		free_image_list(img_list);
		return -EINVAL;
	}

	/* Update Board-ID Description */
	memset(ptr->fsh->param.descr, 0, MAX_DESCR_LEN);
#ifdef __UBOOT__
	strncpy(ptr->fsh->param.descr, board_id, MAX_DESCR_LEN);
#else
	memcpy(ptr->fsh->param.descr, board_id, MAX_DESCR_LEN);
#endif

	/* Update CRC32 if available*/
	fs_image_update_header(ptr->fsh, fs_image_get_size(ptr->fsh, false), ptr->fsh->info.flags);

	/* get current DRAM-INFO descrp*/
 	dram_type = fs_image_getprop(fdt_new, offs, rev_offs, "dram-type", NULL);
 	if(!dram_type){
		free_image_list(img_list);
		return -EINVAL;
	}

	/* remove unneeded Container or FS-Images */
	while(ptr->next) {
		/* Remove unneeded DRAM-INFO */
		if(fs_image_match(ptr->next->fsh, "DRAM-INFO", NULL) &&
				!fs_image_match(ptr->next->fsh, "DRAM-INFO", dram_type)){
			remove_next_image(ptr);
			continue;
		}

		/* Remove EXTRA */
		if(fs_image_match(ptr->next->fsh, "EXTRA", NULL)){
			remove_next_image(ptr);
			continue;
		}

		ptr = ptr->next;
	}

	/* Check for DRAM-CNTR */
	if(!get_fsh_from_list(img_list, "DRAM-INFO", dram_type)){
		free_image_list(img_list);
		return -EINVAL;
	}

	/* --- get nboot-info --- */
	ret = fs_image_get_nboot_info(fi, fdt_new, ni_new, -1, false);

	if(ret){
		free_image_list(img_list);
		return ret;
	}

	/** Update new nboot info
	 * nboot/uboot-start and nboot/uboot-size values are only
	 * provided in OCRAM Board-CFG, if board was booted via flash.
	 */
	ni_new->spl.start[0] = 0;
	ni_new->spl.start[1] = 0;
	ni_new->spl.size = fs_image_get_size(img_list->fsh, false);
	ni_new->nboot.start[0] = ni_new->spl.start[0] + ni_new->spl.size;
	ni_new->nboot.start[1] = ni_new->spl.start[1] + ni_new->spl.size;
	ni_new->nboot.size = get_nboot_cntr_size(img_list);
	ni_new->uboot.start[0] = ni_new->nboot.size + ni_new->nboot.start[0];
	ni_new->uboot.start[1] = ni_new->nboot.size + ni_new->nboot.start[1];

	ret = fs_image_get_nboot_info(fi, fdt_old, &ni_old, -1, false);
	if(ret){
		free_image_list(img_list);
		return ret;
	}

	/* --- Get missing Images --- */
	tmp_fsh = get_fsh_from_list(img_list, "U-BOOT-INFO", arch);
	if(!tmp_fsh){
		/**
		 * Check if U-Boot and/or environment need to be relocated.
		 * Old NBOOT-Info is only available, if device boots from flash.
		 * These Informations can not be set during compile-time.
		 * Therefore SPL will provide missing NBOOT-INFOs during
		 * fs_handle_uboot(). When Infos are missing, than load complete
		 * firmware into $loadaddr.
		 */
		if(ni_old.uboot.start[0] && ni_old.uboot.size){
			/* Since new U-Boot is not provided, we need to know the old size */
			ni_new->uboot.size = ni_old.uboot.size;

			/* Check if there are changes */
			need_uboot = fi->ops->si_differs(&ni_new->uboot, &ni_old.uboot);
			need_env = fi->ops->si_differs(&ni_new->env, &ni_old.env);
		} else {
			puts("WARNING: U-Boot is not found. System will not BOOT!\n");
			puts("WARNING: Load U-Boot and then re-run fsimage save!\n");
		}
	}else{
		ni_new->uboot.size = fs_image_get_size(tmp_fsh, true);
	}
	
	/* load U-Boot from Flash, if needed */
	if(need_uboot) {
		puts("Need to move U-BOOT-INFO\n");
		uboot_addr = addr + (ulong)file_size;
		ret = fs_image_load_uboot(fi, &ni_old,
				(void *)uboot_addr, SUB_HAS_FS_HEADER);
		if(ret) {
			free_image_list(img_list);
			return -EIO;
		}

		if(!fs_image_match((void *)uboot_addr, "U-BOOT-INFO", arch)){
			free_image_list(img_list);
			return -EINVAL;
		}

		ni_new->uboot.size = fs_image_get_size((void *)uboot_addr, true);
		file_size += ni_new->uboot.size;

		ret = append_image_list(img_list, (void *)uboot_addr);
		if(ret){
			free_image_list(img_list);
			return ret;
		}
	}

	/* Load Env from Flash, if Needed */
	if(need_env) {
		puts("Need to move U-Boot Environment\n");
		printf("Loading ENV from %s\n", fi->devname);

		env_addr = addr + (ulong)file_size;

		ret = fs_image_load_env(fi, &ni_old.env, (void *)env_addr, 0);
		if (ret){
			free_image_list(img_list);
			return -EIO;
		}

		file_size += fs_image_get_size((void *)env_addr, true);

		ret = append_image_list(img_list, (void *)env_addr);
		if(ret){
			free_image_list(img_list);
			return ret;
		}
	}

	if(!is_img_list_valid(img_list)){
		free_image_list(img_list);
		return -EINVAL;
	}

	ptr = img_list;

	/* --- Prepare SPL region --- */
	woffset = fs_image_region_add(spl_ri, ptr->fsh, ptr->fsh->type,
					   arch, 0, SUB_IS_SPL | SUB_SYNC);
	if (!woffset){
		free_image_list(img_list);
		return -ENOMEM;
	}
	
	ptr = ptr->next;
	woffset = 0;

	/* --- Prepare NBOOT region --- */
	while(ptr){
		if(fs_image_match(ptr->fsh, "U-BOOT-INFO", arch) ||
				fs_image_match(ptr->fsh, "ENV", NULL))
			break;

		woffset = fs_image_region_add(nboot_ri, ptr->fsh,
			ptr->fsh->type, ptr->fsh->param.descr,
			woffset, SUB_HAS_FS_HEADER | SUB_SYNC);

		if (!woffset){
			free_image_list(img_list);
			return -ENOMEM;
		}

		ptr = ptr->next;
	}

	/* --- Prepare UBOOT Region --- */
	tmp_fsh = get_fsh_from_list(img_list, "U-BOOT-INFO", arch);
	if(tmp_fsh)
	{
		struct fs_header_v1_0 *uboot_fsh = tmp_fsh;

		woffset = fs_image_region_add(uboot_ri, uboot_fsh,
			uboot_fsh->type, uboot_fsh->param.descr,
			0, SUB_HAS_FS_HEADER | SUB_SYNC);

		if (!woffset){
			free_image_list(img_list);
			return -ENOMEM;
		}
	}

	tmp_fsh = get_fsh_from_list(img_list, "ENV", arch);
	if(tmp_fsh){
		struct fs_header_v1_0 *env_fsh = tmp_fsh;

		woffset = fs_image_region_add(env_ri, env_fsh,
			env_fsh->type, env_fsh->param.descr,
			0, SUB_SYNC);

		if (!woffset){
			free_image_list(img_list);
			return -ENOMEM;
		}
	}

	free_image_list(img_list);

	return 0;
}

static void update_board_cfg(struct nboot_info *ni)
{
	uint uboot_size, nboot_size, uboot_offset;
	void *fdt = fs_image_get_cfg_fdt();

	nboot_size = cpu_to_fdt32(ni->nboot.size);
	uboot_offset = cpu_to_fdt32(ni->uboot.start[0]);
	uboot_size = cpu_to_fdt32(ni->uboot.size);

	fdt_find_and_setprop(fdt, "/nboot-info/emmc-boot",
				"nboot-size", &nboot_size, sizeof(uint), 0);
	fdt_find_and_setprop(fdt, "/nboot-info/emmc-boot",
				"uboot-start", &uboot_offset, sizeof(uint), 0);
	fdt_find_and_setprop(fdt, "/nboot-info/emmc-boot",
				"uboot-size", &uboot_size, sizeof(uint), 0);
}

static int fsimage_cntr_save_uboot(ulong addr, uint boot_hwpart, bool force)
{
	struct fs_header_v1_0 *uboot_fsh = (void *)addr;
	struct flash_info fi;
	struct nboot_info ni;
	struct region_info uboot_ri;
	struct sub_info uboot_sub;
	const char *arch = fs_image_get_arch();
	struct fs_header_v1_0 *cfg_fsh = fs_image_get_cfg_addr();
	uint cfg_size = fs_image_get_size(cfg_fsh, false);
	void *fdt = fs_image_get_cfg_fdt();
	int ret = 0;

	if(fs_image_validate(uboot_fsh, "U-BOOT-INFO", arch, (ulong) uboot_fsh))
		return -EINVAL;

	fs_image_region_create(&uboot_ri, &ni.uboot, &uboot_sub);

	if (fs_image_get_flash_info(&fi, fdt))
		return -EINVAL;

	ret = fs_image_get_nboot_info(&fi, fdt, &ni, -1, false);
	if(ret)
		goto put_fi;

	if(!ni.uboot.start[0]){
		puts("FAILED TO SAVE U-BOOT.\n");
		puts("Boot from MMC or provide complete Firmware (NBOOT + UBOOT) in RAM\n");
		ret = -EINVAL;
		goto put_fi;
	}

	ni.uboot.size = fs_image_get_size(uboot_fsh, true);

	/* --- Prepare UBOOT Region --- */
	ret = fs_image_region_add(&uboot_ri, uboot_fsh,
			uboot_fsh->type, uboot_fsh->param.descr,
			0, SUB_HAS_FS_HEADER | SUB_SYNC);

	if (!ret)
		goto put_fi;

	ret = fs_image_save_uboot(&fi, &uboot_ri);
	ret = fs_image_show_save_status(ret, "U-BOOT");

	put_fi:
	fs_image_put_flash_info(&fi);
	if(ret < 0){
		printf("Failed to Save U-BOOT: %d", ret);
		return -EINVAL;
	}

	update_board_cfg(&ni);

	/* calc new crc32 */
	fs_image_update_header(cfg_fsh, cfg_size, cfg_fsh->info.flags);
	return ret;
}

static int fsimage_cntr_save(ulong addr, int boot_hwpart, bool force)
{
	const char *arch = fs_image_get_arch();
	struct fs_header_v1_0 *cfg_fsh;
	struct flash_info fi;
	struct nboot_info ni_new;
	struct region_info nboot_ri, spl_ri;
	struct region_info uboot_ri, env_ri;
	struct sub_info spl_sub, nboot_sub[MAX_SUB_IMGS];
	struct sub_info uboot_sub, env_sub;
	void *fdt_new;
	int failed = 0;

	int ret = 0;

	/* If this is an U-Boot image, skip nboot handling */
	if (fs_image_match((void *)addr, "U-BOOT-INFO", NULL))
		return fsimage_cntr_save_uboot(addr, boot_hwpart, force);

	if(!fs_image_match((void *)addr, "BOOT-INFO", arch) &&
			!fs_image_match((void *)addr, "BOARD-ID", NULL))
		return -EINVAL;

	/* This call will set new board-id if available */
	ret = fs_image_find_board_cfg(addr, force, "save", &cfg_fsh, NULL);
	if (ret <= 0)
		return -EINVAL;

	fdt_new = fs_image_find_cfg_fdt(cfg_fsh);
	if(!fdt_new)
		return -EINVAL;

	/* When new ID is provided, skip to BOOT-INFO */
	if(fs_image_match((void *)addr, "BOARD-ID", NULL))
		addr += fs_image_get_size((void *)addr, true);

	/* Get Flash-Info */
	if (fs_image_get_flash_info(&fi, fdt_new))
		return -EINVAL;

	ret = fs_image_check_boot_dev_fuses(fi.boot_dev, "save");
	if (ret < 0)
		goto put_fi;
	if (ret > 0) {
		printf("Warning! Boot fuses not yet set, remember to burn"
				" them for %s\n", fi.boot_dev_name);
	}

	/* set HWPART, if Available */
	fi.ops->set_boot_hwpart(&fi, boot_hwpart);

	/* Prepare Regions */
	fs_image_region_create(&spl_ri, &ni_new.spl, &spl_sub);
	fs_image_region_create(&nboot_ri, &ni_new.nboot, nboot_sub);
	fs_image_region_create(&uboot_ri, &ni_new.uboot, &uboot_sub);
	fs_image_region_create(&env_ri, &ni_new.env, &env_sub);

	ret = prepare_nboot_cntr_images(addr, fdt_new, &fi,
					&spl_ri, &nboot_ri,
					&uboot_ri, &env_ri, &ni_new);
	if(ret)
		goto put_fi;

	/* --- Found all sub-images, everything is prepared, go and save --- */
	failed = 0;

	/**
	 *  Save is done in two stages.
	 *  In the first stage all new Images are stored as Secondary.
	 *  Then the Images are stored as Primary
	 */

	/* Save BOOT-INFO + NBOOT */
	if (failed != 3) {
		int nboot_failed = fi.ops->save_nboot(&fi, &nboot_ri, &spl_ri);

		if (!failed || (nboot_failed == 3))
			failed = nboot_failed;
	}

	/* Save U-Boot if needed */
	if (uboot_ri.count) {
		int uboot_failed = fs_image_save_uboot(&fi, &uboot_ri);

		if (!failed || (uboot_failed == 3))
			failed = uboot_failed;
	}

	/* Save ENV if needed */
	if ((failed != 3) && env_ri.count) {
		int env_failed = 0;

		printf("\nSaving copy 0 to %s:\n", fi.devname);
		if (fs_image_save_region(&fi, 0, &env_ri))
			env_failed |= BIT(0);

		printf("\nSaving copy 1 to %s:\n", fi.devname);
		if (fs_image_save_region(&fi, 1, &env_ri))
			env_failed |= BIT(0);

		if (failed || (env_failed == 3))
			failed = env_failed;
	}

	fs_image_flush_temp(&fi, 0, 0);


	ret = fs_image_show_save_status(failed, "NBoot");

	if (ret)
		goto put_fi;

	/* Success: Activate new BOARD-CFG by copying it to OCRAM */
	memcpy(fs_image_get_cfg_addr(), cfg_fsh,
	       fs_image_get_size(cfg_fsh, true));

	cfg_fsh = fs_image_get_cfg_addr();
	update_board_cfg(&ni_new);
	fs_image_board_cfg_set_board_rev(cfg_fsh);
	puts("New BOARD-CFG is now active\n");

	put_fi:
	fs_image_put_flash_info(&fi);
	return ret;
}

//------------- direct api
int do_load_from_system(int argc, char* const argv[]) {
	struct fs_header_v1_0 *fsh;
	ulong addr;
	bool force;
	bool load_uboot = false;

	early_support_index = 0;
	if ((argc > 1) && (argv[1][0] == '-')) {
		if (strcmp(argv[1], "-f"))
			return -EINVAL;
		force = true;
		argv++;
		argc--;
	}

	if (argc > 1) {
		size_t len = strlen(argv[1]);

		if (!strncmp(argv[1], "uboot", len)) {
			load_uboot = true;
			argv++;
			argc--;
		} else if (!strncmp(argv[1], "nboot", len)) {
			/* Accept "nboot", too, but it is the default anyway */
			argv++;
			argc--;
		}
	}

	if (argc > 1)
		addr = parse_loadaddr(argv[1], NULL);
	else
		addr = get_loadaddr();

	/* Ask for confirmation if there is already an F&S image at addr */
	fsh = (struct fs_header_v1_0 *)addr;
	if (fs_image_is_fs_image(fsh)) {
		printf("Warning! This will overwrite F&S image at RAM address"
		       " 0x%lx\n", addr);
		if (force && !fs_image_confirm())
			return -EINVAL;
	}

	/* Invalidate any old image */
	memset(fsh->info.magic, 0, 4);

#if __UBOOT__
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	if(!fsimage_cntr_load(addr, load_uboot, 1))
		return CMD_RET_SUCCESS;

	return fsimage_cntr_load(addr, load_uboot, 2);
#else
	return fsimage_imx8_load(addr, load_uboot);
#endif
#else
	if(!fsimage_cntr_load(addr, load_uboot, 1))
		return 0;

	return fsimage_cntr_load(addr, load_uboot, 2);
#endif
}

int do_list(int argc, char* const argv[]) {
	ulong addr;
	ulong offs = 0;
	struct fs_header_v1_0 *fsh;

	if (argc > 1)
		addr = parse_loadaddr(argv[1], NULL);
	else
		addr = get_loadaddr();

	fsh = (struct fs_header_v1_0 *)addr;
	if (!fs_image_is_fs_image(fsh)) {
		printf("No F&S image found at addr 0x%lx\n", addr);
		return -EINVAL;
	}
	printf("Content of F&S image at addr 0x%lx\n\n", addr);

	printf("offset   size     type (description)\n");

	/* Find padded Images if available */
	do{
	     printf("------------------------------------------------------------"
	     "-------------------\n");
		fs_image_parse_image(PARSE_CONTENT, addr, offs, 0);
		offs += fs_image_get_size(fsh, true);
		fsh = (struct fs_header_v1_0 *)(addr + offs);
	}while(fs_image_is_fs_image(fsh));

	return 0;
}

int fs_image_check_saved_cfg(void) {
	void *found_cfg;
	void *expected_cfg;

	/*
	 * All fsimage commands will access the BOARD-CFG in OCRAM. Make sure
	 * it is still valid and not compromised in any way.
	 */
	if (!fs_image_is_ocram_cfg_valid()) {
		printf("Error: BOARD-CFG in OCRAM at 0x%lx damaged\n",
		       (ulong)fs_image_get_cfg_addr());
		return -EINVAL;
	}

	/*
	 * Set the current board_id name and the compare_id that is used in
	 * fs_image_find_board_cfg().
	 */
	fs_image_set_board_id_from_cfg();

	found_cfg = fs_image_get_cfg_addr();
	expected_cfg = fs_image_get_regular_cfg_addr();
	if (found_cfg != expected_cfg) {
		printf("\n"
		       "*** Warning!\n"
		       "*** BOARD-CFG found at 0x%lx, expected at 0x%lx\n"
		       "*** Installed NBoot and U-Boot are not compatible!\n"
		       "\n", (ulong)found_cfg, (ulong)expected_cfg);
	}
	return 0;
}

int do_save_nboot_uboot(int argc, char* const argv[]){
	int boot_hwpart = -1;
	ulong addr;
	bool force = false;
	int ret;

	early_support_index = 0;
	while ((argc > 1) && (argv[1][0] == '-')) {

		if (!strcmp(argv[1], "-e")) {
			if (argc <= 2) {
				puts("Missing argument for option -e\n");
				return -EINVAL;
			}
			early_support_index = simple_strtoul(argv[2], NULL, 0);
			argv += 2;
			argc -= 2;
		} else if (!strcmp(argv[1], "-b")) {
			if (argc <= 2) {
				puts("Missing argument for option -b\n");
				return -EINVAL;
			}
			boot_hwpart = simple_strtol(argv[2], NULL, 0);
			if ((boot_hwpart < 0) || (boot_hwpart > 2)) {
				printf("Invalid argument %s for option -b\n",
				       argv[2]);
				return -EINVAL;
			}
			argv += 2;
			argc -= 2;
		} else if (!strcmp(argv[1], "-f")) {
			force = true;
			argv++;
			argc--;
		} else
			return -EINVAL;
	}

	if (argc > 1)
		addr = parse_loadaddr(argv[1], NULL);
	else
		addr = get_loadaddr();

#ifdef __UBOOT__
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	ret = fsimage_cntr_save(addr, boot_hwpart, force);
#else
	ret = fsimage_imx8_save(addr, boot_hwpart, force);
#endif
#else
	ret = fsimage_cntr_save(addr, boot_hwpart, force);
#endif

	return ret;

	return 0;
}