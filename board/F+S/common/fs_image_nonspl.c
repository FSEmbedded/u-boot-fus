// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright 2025 F&S Elektronik Systeme GmbH
 * Hartmut Keller <keller@fs-net.de>
 *
 * Handle F&S nboot.fs and uboot.fs images.
 *
 * See board/F+S/common/fs_image_common.c for a description of the NBoot file
 * format.
 *
 * When saving NBoot, different parts of the nboot.fs image, so-called
 * sub-images, are stored at different places in flash memory. In addition,
 * some administrative information needs to be stored. In NAND flash we need
 * the Boot Control Block (BCB), consisting of several smaller parts called
 * FCB, DBBT and DBBT-DATA. And in eMMC, some CPUs expect a Secondary Image
 * Table. Both are needed by the ROM loader to locate and load the primary and
 * secondary copy of the SPL bootloader.
 *
 * In case of NAND, this looks like this:
 *
 *       auto-generated          NAND
 *       +------------+          +------------------+
 *       | FCB        |          | FCB Copy 0       | \
 *       | DBBT       |----+---->| DBBT Copy 0      |  |
 *       | DBBT-DATA  |    |     | DBBT-DATA Copy 0 |  |
 *       +------------+    |     +------------------+  | BCB region
 *                         |     | FCB Copy 1       |  |
 *                         +---->| DBBT Copy 1      |  |
 *                               | DBBT-DATA Copy 1 | /
 *                               +------------------+
 *                               | ...              |
 *                               +------------------+
 *                    +--------->| SPL Copy 0       | \
 *                    |          +------------------+  | SPL region
 *                    +--------->| SPL Copy 1       | /
 *                    |          +------------------+
 *   RAM (nboot.fs)   |          | ...              |
 *   +------------+   |          +------------------+
 *   | SPL        |---+  +------>| BOARD-CFG Copy 0 | \
 *   |------------|      |  +--->| FIRMWARE Copy 0  |  |
 *   | BOARD-CFG  |------+  |    +------------------+  | NBOOT region
 *   |------------|      +--|--->| BOARD-CFG Copy 1 |  |
 *   | FIRMWARE   |---------+--->| FIRMWARE Copy 1  | /
 *   +------------+              +------------------+
 *
 * So we have to deal with different flash regions, one for BCB, one for SPL
 * and one for NBOOT. We store two copies of each sub-image in each region to
 * be failsafe. The location, where each region is stored in flash, is given
 * by the nboot-info which is part of the BOARD-CFG. Please note that the
 * FIRMWARE part actually consists of even more sub-images when saving, we
 * just want to keep this example as simple as possible.
 *
 * We use several structs to define the relationship between the sub-images in
 * RAM and the regions in flash memory.
 *
 * A region is built by a struct region_info (ri). It consists of two parts:
 *
 *  1. The place in flash, given by struct storage_info (si). It defines the
 *     two start addresses in flash (start[0..1], one for each copy) and the
 *     common size. In case of eMMC, it also tells the hardware partition
 *     (0: user area, 1: boot1, 2: boot2). This depends on which partition the
 *     eMMC is configured to boot from.
 *  2. A group of sub-images given by struct sub_info (sub). Each sub defines
 *     the source address of the corresponding sub-image in RAM (img),
 *     the size of the sub-image and the target in the flash region, relative
 *     to its start (offset).
 *
 * To deal with the above NAND setup, we need three regions:
 *
 *                       1. ri for BCB
 *                       +------------------+
 *                       | si:              |        NAND
 *                       | start[0]         |------->+------------------+Offset
 *                       | start[1]         |----+  {| FCB Copy 0       |0x0000
 *                       | size             |--+-|--{| DBBT Copy 0      |0x2000
 *                       |------------------|  | |  {| DBBT-DATA Copy 0 |0x4000
 *                       | sub[0]: FCB      |  | +-->|------------------|
 *                       | offset = 0x0000  |  |    {| FCB Copy 1       |0x0000
 *                  +----| img              |  +----{| DBBT Copy 1      |0x2000
 *                  | +--| size             |       {| DBBT-DATA Copy 1 |0x4000
 * auto-generated   | |  |------------------|        |------------------|
 * +------------+<--+ |  | sub[1]: DBBT     |        | ...              |
 * | FCB        |}----+  | offset = 0x2000  |        |                  |
 * |------------|<-------| img              |        |                  |
 * | DBBT       |}-------| size             |        |                  |
 * |------------|<----+  |------------------|        |                  |
 * | DBBT-DATA  |}--+ |  | sub[2]: DBBT-DATA|        |                  |
 * +------------+   | |  | offset = 0x4000  |        |                  |
 *                  | +--| img              |        |                  |
 *                  +----| size             |        |                  |
 *                       +------------------+        |                  |
 *                                                   |                  |
 *                       2. ri for SPL               |                  |
 *                       +------------------+        |                  |
 *                       | si:              |  +---->|------------------|
 *                       | start[0]         |--+ +--{| SPL Copy 0       |0x0000
 *                       | start[1]         |----|-->|------------------|
 *                       | size             |----+--{| SPL Copy 1       |0x0000
 *                       |------------------|        |------------------|
 *                       | sub[0]: SPL      |        | ...              |
 *                       | offset = 0x0000  |        |                  |
 *                  +----| img              |        |                  |
 *                  | +--| size             |        |                  |
 *                  | |  +------------------+        |                  |
 *                  | |                              |                  |
 *                  | |  3. ri for NBOOT             |                  |
 *                  | |  +------------------+        |                  |
 *                  | |  | si:              |        |                  |
 *                  | |  | start[0]         |------->|------------------|
 *                  | |  | start[1]         |----+  {| BOARD-CFG Copy 0 |0x0000
 *                  | |  | size             |--+-|--{| FIRMWARE Copy 0  |0x2000
 * RAM (nboot.fs)   | |  |------------------|  | +-->|------------------|
 * +------------+<--+ |  | sub[0]: BOARD-CFG|  |    {| BOARD-CFG Copy 1 |0x0000
 * | SPL        |}----+  | offset = 0x0000  |  +----{| FIRMWARE Copy 1  |0x2000
 * |------------|<-------| img              |        +------------------+
 * | BOARD-CFG  |}-------| size             |
 * |------------|<----+  |------------------|
 * | FIRMWARE   |}--+ |  | sub[1]: FIRMWARE |
 * +------------+   | |  | offset = 0x2000  |
 *                  + +--| img              |
 *                  +----| size             |
 *                       +------------------+
 *
 * Please note that the two copies of a region are not necessarily stored
 * consecutively. There are typically gaps between them to have space for
 * bigger images in the future. And they may even interleave with other
 * regions, e.g. SPL copy 0 followed by NBOOT copy 0 followed by SPL copy 1
 * followed by NBOOT copy 1. In eMMC, the two copies are typically on the same
 * offset, but in different boot partitions.
 *
 * To handle flash access (loading and saving) as generic as possible, there
 * is a struct flash_info (fi) that holds all flash specific settings.
 * Especially, there is the ops part, that defines a set of access functions
 * that differ between NAND and eMMC. When the code requires a flash specific
 * procedure, it calls such an ops function. As a result, the remaining code
 * can be kept common for both types of flash memory.
 *
 */

#ifdef __UBOOT__
#include <common.h>
#include <command.h>
#include <console.h>			/* confirm_yesno() */
#include <fdt_support.h>		/* fdt_getprop_u32_default_node() */
#include <fs.h>				/* fs_read(), fs_write(), ... */
#include <fuse.h>			/* fuse_read() */
#include <image.h>			/* parse_loadaddr() */
#include <stdlib.h>			/* malloc() */
#include <linux/err.h>
#include <asm/mach-imx/checkboot.h>	/* struct boot_data */
#include "fs_board_common.h"		/* fs_board_*() */
#include <asm/global_data.h>		/* DECLARE_GLOBAL_DATA_PTR */
#endif /* __UBOOT__ */

#include <u-boot/crc.h>			/* crc32() */
#include <asm/mach-imx/hab.h>		/* struct ivt, ... */
#include "fs_image_common.h"		/* Own interface */
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
#include <imx_container.h>
#endif
#if CONFIG_IS_ENABLED(IMX_HAB)
#include <asm/mach-imx/hab.h>
#endif

#define FIT_IMAGES_PATH		"/images"

/* Enable this if index images may have sub-images in the future */
//#define INDEX_WITH_SUB

/* Argument of option -e in fsimage save */
static uint early_support_index;

#ifdef __UBOOT__
#define IMAGE_SPEC "<addr>  [<size> | <file>]"
#else
#define IMAGE_SPEC "<filename>"
#define puts printf
#endif

enum parse_type {
	PARSE_CONTENT,
	PARSE_CHECKSUM,
};

/* Define the usage here just once, it is very similar for U-Boot and Linux */
const char fsimage_usage[] =
#ifndef __UBOOT__
	"Usage:\n"
	"fsimage "
#endif
	"arch\n"
	"    - Show F&S architecture\n"
	"fsimage board-id\n"
	"    - Show current BOARD-ID\n"
#ifdef CONFIG_CMD_FDT
	"fsimage board-cfg [stored | " IMAGE_SPEC "]\n"
	"    - Show contents of current BOARD-CFG or the one in the F&S image\n"
#endif
	"fsimage boot\n"
	"    - Show the current boot settings\n"
	"fsimage list [-c] [-t <type> [-d <descr>]] [" IMAGE_SPEC "]\n"
	"    - List all subimages of an F&S image, or of the one subimage\n"
        "      specified by <type>/<descr>; in case of -c, show checksums\n"
	"fsimage load [uboot | nboot] [" IMAGE_SPEC "]\n"
	"    - Load and verify the current NBoot or U-Boot\n"
	"fsimage save [-f] [-b <n>]"
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	" [-e <n>] [-s]"
#endif
	" [" IMAGE_SPEC "]\n"
	"    - Save the F&S image at the right place (NBoot, U-Boot)\n"
#ifdef __UBOOT__
	"fsimage fuse [-f] [stored | " IMAGE_SPEC "]\n"
	"    - Program fuses according to the current BOARD-CFG.\n"
	"      WARNING: This is a one time option and cannot be undone.\n"
#endif
#ifdef __UBOOT__
	"\n"
	"If no addr is given, use loadaddr. If a file is given, the image\n"
	"is loaded from the file first. Otherwise, the image should already\n"
	"be present at <addr>. A <file> is specified by three items:\n"
	"\n"
	"      <intf> <dev[:part]> <filename>\n"
	"\n"
	"  <intf>       Interface to use, e.g. mmc, usb, ubifs\n"
	"  <dev[:part]> Device number and optionally partition number\n"
	"  <filename>   Filename (incl. path)\n"
#endif
	"\n"
	"Options:\n"
	"-f      Suppress confirmation prompts; useful in scripts.\n"
	"-b <n>  Set eMMC boot hwpart: 0: user, 1: boot1, 2: boot2.\n"
	"        This is ignored on NAND.\n"
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	"-e <n>  Helps in updating from pre-2023.08 NBoot versions. First\n"
	"        update U-Boot, restart the board, then update NBoot. If no\n"
	"        environment is found when saving the new NBoot, try\n"
	"        increasing values 0, 1, 2, 3, ... for <n> until the\n"
	"        environment is found again.\n"
	"-s      Prefer System-ATF over User-ATF in fsimage save. This is\n"
	"        only relevant if the System-ATF was replaced with a\n"
	"        User-ATF by installing a special U-Boot image variant.\n"
	"        If opTee is present, it is handled together with ATF.\n"
	"\n"
	"                                        without -s   with -s\n"
	"          ATF/opTEE in NBoot image      ignored      saved\n"
	"          ATF/opTEE in U-Boot image     saved        ignored\n"
#endif
	"";


/* Forward declarations */

static int fs_image_validate_signed(const struct fs_header_v1_0 *fsh);

static const struct fs_header_v1_0 *fs_image_find(
	const struct fs_header_v1_0 *fsh, const char *type, const char *descr,
	struct index_info *idx_info);

/* ------------- Functions only in U-Boot, not SPL ------------------------- */

#ifdef __UBOOT__
/*
 * Return if currently running from Primary or Secondary copy. This function
 * is called early in boot_f phase of U-Boot and must not access any variables.
 */
u8 fs_image_get_secondary_boot_info(void)
{
	struct fs_header_v1_0 *fsh = fs_image_get_cfg_addr();
	u8 *size = (u8 *)&fsh->info.file_size_low;
	u8 boot_copy = 0;

	/*
	 * We know that a BOARD-CFG is smaller than 64KiB. So only the first
	 * two bytes of the file_size are actually used. Especially the 8th
	 * byte is definitely 0. SPL uses this byte to indicate if it was
	 * running from Primary (0) or Secondary SPL (<>0). Take this info and
	 * reset the byte to 0 before validating the BOARD-CFG.
	 */
	if (size[7]) {
		size[7] = 0;
		boot_copy |= BOOT_COPY_SECONDARY_NBOOT;
	}
	/*
	 * Similar to primary/secondary SPL, SPL uses the 7th byte to indicate
	 * if U-Boot is running from Primary (0) or Secondary (<>0) copy.
	 */
	if (size[6]) {
		size[6] = 0;
		boot_copy |= BOOT_COPY_SECONDARY_UBOOT;
	}

	return boot_copy;
}

/*
 * Return address of board configuration in OCRAM; search for it if not
 * available at expected place. This function is called early in boot_f phase
 * of U-Boot and must not access any variables. Use global data instead.
 */
bool fs_image_find_cfg_in_ocram(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	struct fs_header_v1_0 *fsh;
	const char *type = "BOARD-CFG";

	/* Try expected location first */
	fsh = fs_image_get_regular_cfg_addr();
	if (fs_image_match(fsh, type, NULL))
		return true;

	/*
	 * Search it from beginning of OCRAM.
	 *
	 * To avoid having to search this location over and over again, save a
	 * pointer to it in global data.
	 */
	fsh = (struct fs_header_v1_0 *)CFG_SYS_OCRAM_BASE;
	do {
		if (fs_image_match(fsh, type, NULL)) {
			gd->board_cfg = (ulong)fsh;
			return true;
		}
		fsh++;
	} while ((ulong)fsh < (CFG_SYS_OCRAM_BASE + CFG_SYS_OCRAM_SIZE));

	return false;
}

/* Return the copy that SPL and U-Boot were booted from */
unsigned int fs_image_get_boot_copy(void)
{
	return fs_board_get_boot_copy();
}
#endif /* __UBOOT__ */

static int fs_image_fdt_err(const char *name, const char *reason, int err)
{
	printf("Entry %s in BOARD-CFG %s\n", name, reason);

	return err;
}

/* Get count values from given device tree property and check alignment */
int fs_image_get_fdt_val(const void *fdt, int offs, const char *name, uint align,
			 int count, uint *val)
{
	int len;
	const fdt32_t *start;
	int i;

	start = fdt_getprop(fdt, offs, name, &len);
	if (!start)
		return fs_image_fdt_err(name, "missing", -ENOENT);

	/* Be pedantic, if nboot-info values are wrong, nothing will work */
	if (!len || (len % sizeof(fdt32_t)))
		return fs_image_fdt_err(name, "invalid", -EINVAL);
	len /= sizeof(fdt32_t);
	if (len > count)
		return fs_image_fdt_err(name, "has too many values", -EINVAL);

	/* Fetch all available values */
	for (i = 0; i < len; i++) {
		val[i] = fdt32_to_cpu(start[i]);
		if (align && (val[i] % align))
			return fs_image_fdt_err(name, "not aligned", -EINVAL);
	}

	/* If we have less than count values, repeat the last value */
	for (i = len; i < count; i++)
		val[i] = val[len - 1];

	return 0;
}


/* ------------- Common helper function ------------------------------------ */

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
int fs_image_get_si(const void *fdt, int offs, uint align, const char *type,
		    struct storage_info *si)
{
	int err;
	char name[20];

	si->type = type;

	/* Create the start property name for nboot-info and get value */
	fs_image_build_nboot_info_name(name, type, "-start");

	err = fs_image_get_fdt_val(fdt, offs, name, align, 2, si->start);
	if (err)
		return err;

	/* Create the size property name for nboot-info and get value */
	fs_image_build_nboot_info_name(name, type, "-size");

	return fs_image_get_fdt_val(fdt, offs, name, align, 1, &si->size);
}

static int fs_image_get_nboot_info(struct flash_info *fi, const void *fdt,
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
	if (fdt_getprop(fdt, offs, "support-u-atf", NULL))
		ni->flags |= NI_SUPPORT_U_ATF;
#ifdef CONFIG_IMX8MM
	/* Have a flag that not everything is in one boot partition */
	if (fdt_getprop(fdt, offs, "emmc-both-bootparts", NULL))
		ni->flags |= NI_EMMC_BOTH_BOOTPARTS;
#else
	/* This has always been the default on all other architectures */
	ni->flags |= NI_EMMC_BOTH_BOOTPARTS;
#endif

	/* Parse flash specific settings individually */
	return fi->ops->get_nboot_info(fi, fdt, offs, ni, hwpart, show,
				       early_support_index);
}

static void fs_image_print_crc32_status(const struct fs_header_v1_0 *fsh,
					int err)
{
	char fsh_type[MAX_TYPE_LEN];
	memcpy(&fsh_type, fsh->type, MAX_TYPE_LEN);

	fsh_type[12] = 0;

	switch (err) {
	case 0:
		debug("%s: (no CRC32)\n", fsh_type);
		break;
	case 1:
		debug("%s: (CRC32 header only ok)\n", fsh_type);
		break;
	case 2:
		debug("%s: (CRC32 image only ok)\n", fsh_type);
		break;
	case 3:
		debug("%s: (CRC32 header+image ok)\n", fsh_type);
		break;
	default:
		printf("%s: BAD CRC32\n", fsh_type);
	}
}

/* Return whitespace string with len blanks */
static const char *ws(int len)
{
	const char *whitespace = "                              ";

	if (len < strlen(whitespace))
		whitespace += strlen(whitespace) - len;

	return whitespace;
}

/* Show info line for one F&S image */
static void fs_image_print_line(enum parse_type ptype,
				const struct fs_header_v1_0 *fsh,
				const void *fsi, uint offs, int level)
{
	char info[MAX_DESCR_LEN + 1];
	u32 *pcs;
	bool crc_valid;
	u16 flags;

	if (ptype == PARSE_CONTENT) {
		flags = fsh->info.flags;
		printf("%08x %08x %c%c%c%c%c", offs,
		       fs_image_get_size(fsh, false),
		       (flags & FSH_FLAGS_DESCR) ? 'D' : '-',
		       (flags & FSH_FLAGS_CRC32) ? 'C' : '-',
		       (flags & FSH_FLAGS_SECURE) ? 'S' : '-',
		       (flags & FSH_FLAGS_INDEX) ? 'I' : '-',
		       (flags & FSH_FLAGS_EXTRA) ? 'E' : '-');
	} else if (ptype == PARSE_CHECKSUM) {
		pcs = (u32 *)&fsh->type[12];
		crc_valid = (fs_image_check_crc32_split(fsh, fsi) >= 0);
		printf("%08x %08x %s", offs, *pcs, crc_valid ? "(OK)" : "BAD!");
	}

	puts(ws(level));

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

	puts("\n");
}

/* Parse extra data and show any IMX headers found */
static void fs_image_parse_extra(const void *start, uint offs, uint remaining,
				 int level)
{
#ifdef CONFIG_FS_CNTR_COMMON
	/* Handle any IMX headers at beginning of extra data */
	while (remaining >= 0x400) {
		const struct container_hdr *imx_hdr;
		uint size;
		uint sig_offset;

		imx_hdr = start + offs;
		if (!valid_container_hdr(imx_hdr))
			break;
		size = (imx_hdr->length_msb << 8) | imx_hdr->length_lsb;
	        size = (size + 0x3ff) & ~0x3ff; /* Align to 1KB */
		sig_offset = imx_hdr->sig_blk_offset + offs;

		printf("%08x %08x ----- %sIMX Container Header (",
		       offs, size, ws(level));
		if (imx_hdr->num_images == 1)
			puts("1 image, ");
		else
			printf("%d images, ", imx_hdr->num_images);

		switch (imx_hdr->flags & 3) {
		case 1:
			printf("NXP signed @ 0x%x", sig_offset);
			break;
		case 2:
			printf("OEM signed @ 0x%x", sig_offset);
			break;
		default:
			puts("unsigned");
			break;
		}
		puts(")\n");

		offs += size;
		remaining -= size;
	}
#endif

	if (remaining > 0) {
		printf("%08x %08x ----- %s[extra data]\n", offs, remaining,
		       ws(level));
	}
}

/* Parse an F&S image and print info, including all sub-images */

const char *separator =
	"-------------------------------------------------"
	"------------------------------\n";

static void fs_image_parse_image(enum parse_type ptype, const void *start,
				 uint offs, uint remaining, int level)
{
	const struct fs_header_v1_0 *fsh = start + offs;
	uint size;

	if (!fs_image_is_fs_image(fsh))
		return;

	do {
		if (!level)
			puts(separator);

		fs_image_print_line(ptype, fsh, fsh + 1, offs, level);

		size = fs_image_get_size(fsh, false);
		offs += FSH_SIZE;

#ifdef CONFIG_FS_CNTR_COMMON
		if (fs_image_is_index(fsh)) {
			uint count;
			uint fsi_offs;
			uint fsi_size;

			/*
			 * Index image: The index contains all the F&S headers
			 * of the following images, followed by the image data
			 * of the images themselves.
			 */
			for (count = size / FSH_SIZE; count; count--) {
				fsh++;
				if (!fs_image_is_fs_image(fsh))
					break;
				fsi_offs = offs + size;
				fs_image_print_line(ptype, fsh, start + fsi_offs,
						    fsi_offs, level);

				fsi_size = fs_image_get_size(fsh, false);
#ifdef INDEX_WITH_SUB
				/* Recursively handle sub-images */
				fs_image_parse_image(ptype, start, fsi_offs,
						     fsi_size, level + 1);
#endif
				size += fsi_size;
			}
		} else
#endif /* FS_CNTR_COMMON */
		if (size) {
			uint extra_offs = 0;
			uint extra_size;

			/*
			 * Regular image: The image data follows immediately
			 * behind the F&S header (maybe offset by extra_size)
			 * and may be a set of sub-images.
			 */
#if CONFIG_IS_ENABLED(IMX_HAB)
			if (fs_image_is_signed(fsh))
				extra_offs = HAB_HEADER;
#endif
			extra_size = fs_image_get_extra_size(fsh);
			if (extra_size && (ptype == PARSE_CONTENT)) {
				fs_image_parse_extra(start, offs + extra_offs,
						     extra_size, level + 1);
			}
			extra_size += extra_offs;

			/* Recursively handle sub-images */
			fs_image_parse_image(ptype, start, offs + extra_size,
					     size - extra_size, level + 1);
		}

		/* Go to next image */
		offs += size;
		if (remaining) {
			remaining -= size + FSH_SIZE;
			if (!remaining)
				break;
		}
		fsh = start + offs;
	} while (fs_image_is_fs_image(fsh));

	if (remaining && (ptype == PARSE_CONTENT)) {
		printf("%08x %08x ----- %s[padding/unknown data]\n", offs,
		       remaining, ws(level));
	}

	return;
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
	strncpy(fsh->type, type, sizeof(fsh->type));
	strncpy(fsh->param.descr, descr, sizeof(fsh->param.descr));

	/* Set size, flags and padsize, calculate CRC32 if requested */
	fs_image_update_header(fsh, size, fsh_flags);
}

/**
 * fs_image_find_index() - Search image within INDEX
 * @fsh_idx: INDEX header to search for
 * @type: type to search for
 * @descr: descr to search for or NULL if any allowed
 * @idx_info: struct holds additional infos if fsh is index. NULL is allowed.
 *
 * Context: This is part of the fsimage command.
 * Return: Pointer to F&S header, NULL if not found
 *
 * Search given INDEX for an F&S header of given type @type and (optional)
 * description @descr. Return a pointer to it if found, NULL otherwise.
 */
static const struct fs_header_v1_0 *fs_image_find_index(
	const struct fs_header_v1_0 *fsh_idx, const char *type,
	const char *descr, struct index_info *idx_info)
{
	uint num_images = fs_image_index_get_n(fsh_idx);
	int i;
	void *fsi;
	const struct fs_header_v1_0 *fsh;

	if (fs_image_match(fsh_idx, type, descr))
		return fsh_idx;

	fsi = (void *)fsh_idx + fs_image_get_size(fsh_idx, true);
	fsh = fsh_idx + 1;
	for (i = 0; i < num_images; i++) {
		if (fs_image_match(fsh, type, descr)) {
			if (idx_info) {
				idx_info->fsh_idx = fsh_idx;
				idx_info->fsh = fsh;
				idx_info->fsi = fsi;
			}

			return fsh;
		}
#ifdef INDEX_WITH_SUB
		/* Check all subimages */
		if (fs_image_is_fs_image(fsh)) {
			void *sub;

			sub = fs_image_find(fsi, type, descr, idx_info);
			if (sub)
				return sub;
		}
#endif
		fsi += fs_image_get_size(fsh, false);
		fsh++;
	}

	return NULL;
}

/**
 * Return pointer to the header of the given sub-image or NULL if not found
 * @param *fsh: fs header to search for.
 * @param *type: type to search for
 * @param *decr: descr to search for or NULL
 * @param *idx_info: struct holds additional infos if fsh is found. NULL is allowed.
 * @return ptr to fsh or NULL if not found
 */
static const struct fs_header_v1_0 *fs_image_find(
	const struct fs_header_v1_0 *fsh, const char *type, const char *descr,
	struct index_info *idx_info)
{
	const struct fs_header_v1_0 *fsh_found;
	uint size;
	uint extra_size;
	uint remaining;

	if (idx_info) {
		idx_info->fsh_idx = NULL;
		idx_info->fsh = NULL;
		idx_info->fsi = NULL;
	}

	if (!fs_image_is_fs_image(fsh))
		return NULL;

	if (fs_image_match(fsh, type, descr)) {
		if (idx_info) {
			idx_info->fsh = fsh;
			idx_info->fsi = fsh + 1;
		}

		return fsh;
	}

	extra_size = fs_image_get_extra_size(fsh);
	remaining = fs_image_get_size(fsh, false);
	remaining -= extra_size;

	/* Get first subimg */
#if CONFIG_IS_ENABLED(IMX_HAB)
	if (fs_image_is_signed(fsh))
		fsh = (void *)fsh + HAB_HEADER;
#endif
	fsh++;
	fsh = (void *)((ulong)fsh + extra_size);
	while (remaining > 0) {
#if CONFIG_IS_ENABLED(FS_SECURE_BOOT)
		//in case of a signed Image
		if (!fs_image_is_fs_image(fsh))
			fsh++;
#endif
		if (!fs_image_is_fs_image(fsh))
			return NULL;

		if (fs_image_match(fsh, type, descr)) {
			if (idx_info) {
				idx_info->fsh = fsh;
				idx_info->fsi = fsh + 1;
			}

			return fsh;
		}

		if (fs_image_is_index(fsh)) {
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

/**
 * Like fs_image_find(), but here, we check if next F&S Image is concatinated.
 * @param *fsh: fs header to search for.
 * @param *type: type to search for
 * @param *decr: descr to search for or NULL
 * @param *idx_info: struct holds additional infos if fsh is index. NULL is allowed.
 * @return ptr to fsh or NULL if not found
 */
static const struct fs_header_v1_0 *fs_image_find_concat(
	const struct fs_header_v1_0 *fsh, const char *type, const char *descr,
	struct index_info *idx_info)
{
	const struct fs_header_v1_0 *fsh_sub;

	if (!strcmp(type, "BOARD-CFG"))
		fs_image_set_compare_id(descr);

	while (fs_image_is_fs_image(fsh)) {
		fsh_sub = fs_image_find(fsh, type, descr, idx_info);
		if (fsh_sub)
			return fsh_sub;

		fsh = (void *)fsh + fs_image_get_size(fsh, true);
	}

	return NULL;
}

void fs_image_region_create(struct region_info *ri, struct storage_info *si,
			    struct sub_info *sub)
{
	ri->si = si;
	ri->sub = sub;
	ri->count = 0;
}


/*
 * Add a subimage with any format to the region. Return offset for next
 * subimage or 0 in case of error.
 */
void fs_image_region_add_raw(struct region_info *ri, void *img,
			     const char *type, const char *descr, uint woffset,
			     uint flags, uint size)
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
static uint fs_image_region_add(struct region_info *ri,
				struct fs_header_v1_0 *fsh,
				const char *type, const char *descr,
				uint woffset, uint flags)
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
void fs_image_show_sub_status(int err)
{
	switch (err) {
	case 0:
		puts(" OK\n");
		break;
	case 1:
		puts(" BAD BLOCKS\n");
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
		return CMD_RET_SUCCESS;
	}

	if (failed != 3) {
		puts("incomplete!\n\n"
		     "*** WARNING! One copy failed, the system is unstable!\n");
		return CMD_RET_SUCCESS;
	}

	printf("\nFAILED!\n\n"
	       "*** ATTENTION!\n"
	       "*** Do not switch off or restart the board before you have\n"
	       "*** installed a working %s version. Otherwise the board will\n"
	       "*** most probably fail to boot.\n", type);

	return CMD_RET_FAILURE;
}

int fs_image_confirm(void)
{
	int yes;

	puts("Are you sure? [y/N] ");
	yes = confirm_yesno();
	if (!yes)
		puts("Aborted by user, nothing was changed\n");

	return yes;
}

/* Determine start copy depending on the copy NBoot or U-Boot were booted from */
int fs_image_get_start_copy(bool uboot, bool opposite)
{
	int start_copy;
	const char *loader_name;
	const char *copy_name;
	unsigned int boot_copy = fs_image_get_boot_copy();

	if (uboot) {
		start_copy = !!(boot_copy & BOOT_COPY_SECONDARY_UBOOT);
		loader_name = "U-Boot";
	} else {
		start_copy = !!(boot_copy & BOOT_COPY_SECONDARY_NBOOT);
		loader_name = "NBoot";
	}
	copy_name = start_copy ? "Secondary" : "Primary";
	if (opposite)
		start_copy = 1 - start_copy;

	printf("Booted from %s %s, so starting with copy %d\n",
	       copy_name, loader_name, start_copy);

	return start_copy;
}

static int fs_image_get_boot_dev(const void *fdt, enum boot_device *boot_dev,
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
	*boot_dev = fs_image_get_boot_dev_from_name(boot_dev_prop);
	if (*boot_dev == UNKNOWN_BOOT) {
		printf("Unknown boot device %s in BOARD-CFG\n", boot_dev_prop);
		return -EINVAL;
	}

	*boot_dev_name = fs_image_get_name_from_boot_dev(*boot_dev);

	return 0;
}

/* Check boot device; Return 0: OK, 1: Not fused yet, <0: Error */
int fs_image_check_boot_dev_fuses(enum boot_device boot_dev, const char *action)
{
#ifdef __UBOOT__
	enum boot_device boot_dev_fuses;

	boot_dev_fuses = fs_board_get_boot_dev_from_fuses();
	if (boot_dev_fuses == boot_dev)
		return 0;		/* Match, no change */

	if ((boot_dev_fuses == USB_BOOT) || (boot_dev_fuses == USB2_BOOT))
		return 1;		/* Not fused yet */

	printf("Error: New BOARD-CFG wants to boot from %s but board is\n"
	       "already fused for %s. Refusing to %s this configuration.\n",
	       fs_image_get_name_from_boot_dev(boot_dev),
	       fs_image_get_name_from_boot_dev(boot_dev_fuses), action);

	return -EINVAL;
#else
	/* Do not check boot fuses in Linux, this is only done in U-Boot */
	return 0;
#endif
}

/* Check CRC32 from indexed Images */
static int fs_image_check_index_crc32(struct fs_header_v1_0 *fsh_idx)
{
	uint num_images = fs_image_index_get_n(fsh_idx);
	int i;
	int err = 0;
	void *fsi = fsh_idx + fs_image_get_size(fsh_idx, true);
	const struct fs_header_v1_0 *fsh = fsh_idx + 1;

	for (i = 0; i < num_images; i++) {
		if (!fs_image_is_fs_image(fsh))
			continue;

		err = fs_image_check_crc32_split(fsh, fsi);
		fs_image_print_crc32_status(fsh, err);
		if (err)
			return err;

#ifdef INDEX_WITH_SUB
		/* Check CRC32 of subimages */
		if (fs_image_is_fs_image(fsi)) {
			err = fs_image_check_all_crc32(fsi);
			if (err)
				return err;
		}
#endif
		fsi += fs_image_get_size(fsh, false);
		fsh++;
	}

	return err;
}

/* Check CRC32 from image and all sub-images */
int fs_image_check_all_crc32(struct fs_header_v1_0 *fsh)
{
	uint size;
	uint remaining;
	uint extra_size;
	int err;

	debug("  - %s", fsh->type);
	err = fs_image_check_crc32(fsh);
	fs_image_print_crc32_status(fsh, err);
	if (err)
		return err;

	extra_size = fs_image_get_extra_size(fsh);
	remaining = fs_image_get_size(fsh++, false);
	remaining -= extra_size;

	while (remaining > 0) {
		if (!fs_image_is_fs_image(fsh))
			break;

		/* check indexed image or recursivly */
		if (fs_image_is_index(fsh))
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

/* Validate an image, either check signature or CRC32; 0: OK, <0: Error */
static int fs_image_validate(const struct fs_header_v1_0 *fsh, const char *type,
			     const char *descr, ulong offs)
{
	int err;

	if (!fs_image_match(fsh, type, descr)) {
		printf("Error: No %s image for %s found at offset 0x%lx\n",
		       type, descr, offs);
		return -EINVAL;
	}

	if (fs_image_is_signed(fsh)) {
		printf("Found signed %s image at offset 0x%08lx\n", type, offs);

		return fs_image_validate_signed(fsh);
	}

	printf("Found unsigned %s image at offset 0x%08lx\n", type, offs);

	if (fs_board_is_closed()) {
		puts("\nError: Board is closed, refusing to save unsigned"
		     " image\n");
		return -EINVAL;
	}

	err = fs_image_check_crc32(fsh);
	fs_image_print_crc32_status(fsh, err);

	if (err >= 0)
		return 0;

	return err;
}

/* Validate all image parts of the loaded image, e.g. NBoot and U-Boot */
static int fs_image_validate_full(ulong addr)
{
	const struct fs_header_v1_0 *fsh = (void *)addr;
	ulong offs = 0;
	int err;
	uint size;

	while (fs_image_is_fs_image(fsh)) {
		if (strcmp(fsh->type, "BOARD-ID")) {
			err = fs_image_validate(fsh, fsh->type,
						fsh->param.descr, offs);
			if (err)
				return err;
		}
		size = fs_image_get_size(fsh, true);
		offs += size;
		fsh = (void *)fsh + size;
	}

	return 0;
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
		 * If image data is external, given by data_position or
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
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
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
	}

	return 0;
}

/* Struct that is big enough for all headers that we may want to load */
union any_header {
	struct fs_header_v1_0 fsh;
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	struct ivt ivt;
#endif
	struct fdt_header fdt;
};

/* Get image length any header (F&S header, IVT or FIT header) */
int fs_image_get_size_from_header(struct flash_info *fi, uint offs, uint lim,
				  struct sub_info *sub, uint *size)
{
	int err;
	uint tmp_size;

	/* Load header (IVT, FIT or FS_HEADER) and get size from it */
	tmp_size = sizeof(union any_header);
	err = fs_image_load_sub(fi, offs, tmp_size, lim, 0, sub->img);
	if (err)
		return err;

	if (fdt_magic(sub->img) == FDT_MAGIC) {
		/* Read the FDT part of the FIT image to get size */
		tmp_size = fdt_totalsize(sub->img);
		err = fs_image_load_sub(fi, offs, tmp_size, lim, 0, sub->img);
		if (!err)
			err = fs_image_get_size_from_fit(sub, size);
	} else {
		err = fs_image_get_size_from_fsh_or_ivt(sub, size);
#ifdef CONFIG_IMX8MM
		/* Remove virtual offset */
		if (sub->flags & SUB_IS_SPL)
			*size -= 0x400;
#endif
	}

	return err;
}

/*
 * Get pointer to BOARD-CFG image that is to be used and to NBOOT part
 * Returns: <0: error; 0: aborted by user; 1: same ID; 2: new ID
 */
static int fs_image_find_board_cfg(ulong addr, bool force, const char *action,
				   struct index_info *cfg_info,
				   struct fs_header_v1_0 **nboot)
{
	struct fs_header_v1_0 *fsh = (struct fs_header_v1_0 *)addr;
	const struct fs_header_v1_0 *cfg = NULL;
	const char *board_id = fs_image_get_board_id();
	char new_id[MAX_DESCR_LEN + 1];
	const char *nboot_version;
	const void *fdt;
	int ret = 1;

	/* In case of an NBoot image with prepended BOARD-ID, use this ID */
	if (fs_image_match(fsh, "BOARD-ID", NULL)) {
		memcpy(new_id, fsh->param.descr, MAX_DESCR_LEN);
		new_id[MAX_DESCR_LEN] = '\0';
		if (strncmp(new_id, board_id, MAX_DESCR_LEN)) {
#ifndef __UBOOT__
			/* Changing BOARD-ID is not allowed in Linux */
			printf("Error: Current board is %s, refusing to %s"
			       " for %s\n", board_id, action, new_id);
			return -EINVAL;
#else
#if CONFIG_IS_ENABLED(FS_SECURE_BOOT) && CONFIG_IS_ENABLED(IMX_HAB)
			if (fs_board_is_closed()) {
				printf("Error: Current board is %s and board"
				       " is closed\nRefusing to %s for %s\n",
				       board_id, action, new_id);
				return -EINVAL;
			}
#endif
			printf("Warning! Current board is %s but you will\n"
			       "%s for %s\n", board_id, action, new_id);
			if (!force && !fs_image_confirm()) {
				return 0; /* used_cfg == NULL in this case */
			}

			/* Set this BOARD-ID as compare_id */
			board_id = new_id;
#endif /* !__UBOOT__ */
		}
		fsh++;
	}

	cfg = fs_image_find_concat(fsh, "BOARD-CFG", board_id, cfg_info);
	if (!cfg)
		return -ENOENT;

	/* Get and show NBoot version as noted in BOARD-CFG */
	fdt = fs_image_find_cfg_fdt_idx(cfg_info);
	if (!fdt)
		return -ENOENT;

	nboot_version = fs_image_get_nboot_version(fdt);
	if (!nboot_version) {
		printf("Unknown NBOOT version, refusing to %s\n", action);
		return -EINVAL;
	}
	printf("Found NBOOT version %s\n", nboot_version);

	if (nboot)
		*nboot = fsh;

	return ret;
}

/* Find the default description to given type by looking at BOARD-CFG */
const char *fs_image_get_default_descr(ulong addr, const char *type)
{
	struct index_info cfg_info = {0};
	const void *fdt;
	int cfg_offs;
	int rev_offs;
	const char *prop_name;

	/* BOARD-CFG and BOARD-ID need board_id */
	if (!strcmp(type, "BOARD-CFG") || !strcmp(type, "BOARD-ID"))
		return fs_image_get_board_id();

	/* All others but DRAM settings need arch */
	if (strcmp(type, "DRAM-FW")
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	    && strcmp(type, "DRAM-INFO")
#else
	    && strcmp(type, "DRAM-TYPE")
#endif
	    && strcmp(type, "DRAM-TIMING"))
		return fs_image_get_arch();

	/* DRAM settings either need dram-type or dram-timing from BOARD-CFG */
	if (fs_image_find_board_cfg(addr, false, "list", &cfg_info, NULL) <= 0)
		return NULL;

	fdt = fs_image_find_cfg_fdt_idx(&cfg_info);
	cfg_offs = fs_image_get_board_cfg_offs(fdt);
	rev_offs = fs_image_get_board_rev_subnode(fdt, cfg_offs);
	if (!strcmp(type, "DRAM-TIMING"))
		prop_name = "dram-timing";
	else
		prop_name = "dram-type";

	return fs_image_getprop(fdt, cfg_offs, rev_offs, prop_name, NULL);
}

#ifdef __UBOOT__
/**
 * fs_image_get_image_params() - Get parameters of image to process
 * @argc:   Number of available command line arguments
 * @argv:   Command line arguments
 * @ip:     Pointer to fs_image_params for data to be filled in
 * @def_fname: Filename to use if no name is given on command line
 *
 * Return: true: Success, false: Failure
 *
 * Parse a command line of the form:
 *
 *   [<addr> [<intf> <dev:[part]> [<file>] | <size>]]
 *
 * The first part is the address in RAM where the image is loaded to. If
 * omitted, $loadaddr is used. The second part is either a filename spec
 * (similar to the load command), or a size. If a filename spec is given, the
 * file is loaded to <addr> first before processing starts. Otherwise it is
 * assumend that the image is already present in RAM at <addr>.
 *
 * If <file> is missing in the filename spec, @def_fname is used. In
 * case of fsimage load, the caller should set this according to the image type
 * to be loaded, e.g. uboot.fs in case of U-Boot. Otherwise it can be NULL,
 * which means "nboot.fs" is used.
 *
 * If <size> is given, it is used to limit the RAM region that is processed.
 * This makes sure that no other F&S image that happens to be present in RAM
 * exactly at the end of the image to accidently be included in processing. A
 * typical usecase would be $filesize to limit the command to the size of the
 * previously loaded file. When a filename spec is given, a <size> value is
 * not needed because it is automatically set to the file's size.
 *
 * Examples (assuming that . can be used for $loadaddr):
 *   fsimage save                     - Save the image at $loadaddr
 *   fsimage save . $filesize         - Save the image at $loadaddr, limit size
 *   fsimage save . mmc 0:2           - Load file nboot.fs from mmc and save
 *   fsimage save . mmc 0:2 myfile.fs - Load file myfile.fs from mmc and save
 */
static bool fs_image_get_image_params(int argc, char *const argv[],
				      struct fs_image_params *ip,
				      const char *def_fname)
{
	ip->addr = 0;
	ip->size = 0;
	ip->fname = NULL;

	if (argc > 4)
		return false;

	/*
	 * Get address. If a user forgets the <addr> argument when loading
	 * from a filename spec, then <intf> is taken for <addr>, which
	 * typically results in value zero, causing the board to hang
	 * afterwards when trying to access this address. Avoid this error by
	 * making sure that <addr> is not zero.
	 */
	if (argc > 0)
		ip->addr = parse_loadaddr(argv[0], NULL);
	else
		ip->addr = get_loadaddr();
	if (!ip->addr)
		return false;

	/* Get size */
	if (argc == 2) {
		ip->size = hextoul(argv[1], NULL);
		return true;
	}

	/* Get filename spec */
	if (argc > 2) {
		ip->interface = argv[1];
		ip->devpart = argv[2];

		if (!def_fname)
			def_fname = "nboot.fs";
		ip->fname = (argc > 3) ? argv[3] : def_fname;
	}

	return true;
}

/* Load image from file and fill in ip->size */
static bool fs_image_provide_file(struct fs_image_params *ip)
{
	loff_t size;

	set_fileaddr(ip->addr);

	if (fs_set_blk_dev(ip->interface, ip->devpart, FS_TYPE_ANY))
		return false;

	/*
	 * len=0 loads the whole file and returns the actually read bytes
	 * This call outputs errors and info what is loaded.
	 */
	if (fs_read(ip->fname, ip->addr, 0, 0, &size))
		return false;

	/*
	 * Remark: Each call to a filesystem function implicitly calls
	 * fs_close() at the end. Thus if more than one function should be
	 * used in a sequence, every single call has to be preceeded by a call
	 * to fs_set_blk_dev().
	 */

	ip->size = size;
	env_set_fileinfo(size);

	return true;
}

static int fs_image_store_file(struct fs_image_params *ip)
{
	loff_t len;

	if (fs_set_blk_dev(ip->interface, ip->devpart, FS_TYPE_ANY))
		return -EACCES;

	if ((fs_write(ip->fname, ip->addr, 0, ip->size, &len) < 0)
	    || (len < ip->size))
		return -EIO;

	return 0;
}
#endif /* __UBOOT__ */

static int fs_image_locate(int argc, char *const argv[], ulong *addr)
{
	struct fs_image_params ip;

	if (!fs_image_get_image_params(argc, argv, &ip, NULL))
		return CMD_RET_USAGE;

	if (ip.fname) {
		printf("Reading image from %s\n  ", ip.fname);
		if (!fs_image_provide_file(&ip))
			return CMD_RET_FAILURE;
		puts("\n");
#ifdef __UBOOT__
	} else {
		printf("Using image at addr 0x%lx\n", ip.addr);
#endif
	}
	if (!fs_image_is_fs_image((void *)ip.addr)) {
		puts("Error: This is not an F&S image\n");
		return CMD_RET_FAILURE;
	}

	/*
	 * Clear a word at the end of the image to make sure that any data
	 * behind it is not misinterpreted as a concatenated further F&S
	 * image.
	 */
	ip.size = (ip.size + 3) & ~3;
	if (ip.size)
		*(u32 *)(ip.addr + ip.size) = 0;

	*addr = ip.addr;

	return CMD_RET_SUCCESS;
}

static int fs_image_locate_nboot(int argc, char *const argv[], ulong *addr)
{
	const char *arch;
	int ret;

	ret = fs_image_locate(argc, argv, addr);
	if (ret)
		return ret;

	arch = fs_image_get_arch();
	if (!fs_image_match((void *)*addr, "NBOOT", arch)
	    && !fs_image_match((void *)*addr, "BOOT-INFO", arch)) {
		puts("Error: This is not an F&S NBoot image, use 'stored'"
		       " to refer to stored NBoot\n");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

/* Invalidate the temp buffer read cache */
void fs_image_drop_temp(struct flash_info *fi)
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

int fs_image_load_sub(struct flash_info *fi, uint offs, uint size, uint lim,
		      uint flags, u8 *buf)
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

int fs_image_load_image(struct flash_info *fi, const struct storage_info *si,
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
		printf("  Warning! One copy corrupted! Saving image again may"
		       " fix this.\n");
	}

#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	/* In case of SPL, set the secondary bit back to 0 before comparing */
	if (sub->flags & SUB_IS_SPL) {
		fs_image_set_spl_secondary_bit(copy0, 0);
		if (copy0 != copy1)
			fs_image_set_spl_secondary_bit(copy1, 0);
	}
#endif

	if (!err) {
		if (copy0 == copy1)
			size0 = sub->size;
		else if ((size0 != sub->size) || memcmp(copy0, copy1, size0))
			printf("  Warning! Images differ, taking copy 0\n");
		else
			printf("  Both copies are identical\n");
	}

	/* Align the size to 16 Bytes, pad with 0 and fill FS header */
	sub->size = ALIGN(size0, 16);
	sub->img = copy0 + sub->size;
	memset(copy0 + size0, 0, sub->size - size0);

	if (!(sub->flags & SUB_HAS_FS_HEADER))
		fs_image_set_header(fsh, sub->type, sub->descr, size0, 0);

	return 0;
}

/* Load the F&S header of ATF in the ATF region, return 0 if ATF, 1 if U-ATF */
static bool fs_image_is_u_atf(struct flash_info *fi,
			      const struct storage_info *atf_si)
{
	struct fs_header_v1_0 fsh;
	const char *arch = fs_image_get_arch();
	int start_copy = 0;
	int copy;
	uint size = atf_si->size;
	uint start;
	uint lim;

	/* Clear the temp buffer (read cache) */
	fs_image_drop_temp(fi);

	/* Find a valid copy of the ATF header */
	copy = start_copy;
	do {
		fi->ops->set_hwpart(fi, copy, atf_si);
		start = atf_si->start[copy];
		lim = start + size;
		if (!fs_image_load_sub(fi, start, FSH_SIZE, lim, 0, (u8 *)&fsh))
		{
			if (fs_image_match(&fsh, "ATF", arch))
				return false;
			if (fs_image_match(&fsh, "U-ATF", arch))
				return true;
		}
		copy = 1 - copy;
	} while (copy != start_copy);

	return false;
}

/* Check CRC32 for an environment of given size */
int fs_image_check_env_crc32(void *env, uint size)
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

/*
 * Load U-Boot to given address. If SUB_HAS_FS_HEADER is not set as sub_flags,
 * then fs_image_load_image() will create a new one. If the image actually has
 * a header in this case (new U-BOOT versions are stored with header), it is
 * used for CRC32 checking, then removed, and the own new header is used
 * instead.
 */
static int fs_image_load_uboot(struct flash_info *fi, struct nboot_info *ni,
			       void *addr, uint sub_flags, ulong *size)
{
	struct sub_info sub;
	struct fs_header_v1_0 *uboot_fsh = addr;
	struct fs_header_v1_0 *uboot_atf_fsh = addr;
	uint fsh_flags = 0;
	int err;
	bool have_atf = false;
	const char *arch = fs_image_get_arch();

	if (ni->flags & NI_SUPPORT_U_ATF)
		have_atf = fs_image_is_u_atf(fi, &ni->atf);

	if (have_atf) {
		/* Load U-ATF behind U-BOOT-ATF header that is filled in later */
		sub.type = "U-ATF";
		sub.descr = arch;
		sub.img = uboot_atf_fsh + 1;
		sub.offset = 0;
		sub.flags = SUB_HAS_FS_HEADER;
		err = fs_image_load_image(fi, &ni->atf, &sub);
		if (err)
			return err;

#ifdef CONFIG_OPTEE
		/* Load U-TEE */
		sub.type = "U-TEE";
		sub.offset += sub.size;
		err = fs_image_load_image(fi, &ni->atf, &sub);
		if (err)
			return err;
#endif
		uboot_fsh = sub.img;
	}

#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	sub.type = "U-BOOT-INFO";
	fsh_flags |= (FSH_FLAGS_INDEX | FSH_FLAGS_EXTRA);
#else
	sub.type = "U-BOOT";
#endif
	sub.descr = arch;
	sub.img = uboot_fsh;
	sub.offset = 0;
	sub.flags = sub_flags;
	err = fs_image_load_image(fi, &ni->uboot, &sub);
	if (err)
		return err;

	/* Compute CRC32 if it is missing */
	if (!(uboot_fsh->info.flags & (FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE)))
		fs_image_update_header((void *)uboot_fsh, sub.size,
			       FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE | fsh_flags);

	if (have_atf) {
		/* Fill in U-BOOT-ATF header */
		fs_image_set_header(uboot_atf_fsh, "U-BOOT-ATF", arch,
				    sub.img - (void *)(uboot_atf_fsh + 1),
				    FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE);
	}

	/*
	 * Clear a word to invalidate any subsequent F&S images that we may
	 * have loaded there before, e.g. a second copy of an image.
	 */
	*(u32 *)sub.img = 0;

	if (size)
		*size = (ulong)(sub.img - addr);

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

/* Write one sub-image to flash */
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
		if (!fi->write_pos || (base_offs != fi->base_offs)) {
			err = fs_image_fill_temp(fi, base_offs, lim, flags);
			if (err)
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

		if (!fi->write_pos || (base_offs != fi->base_offs)) {
			/* Fill TEMP with DATA from FLASH */
			err = fs_image_fill_temp(fi, base_offs, lim, flags);
			if (err)
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
int fs_image_save_region(struct flash_info *fi, int copy,
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

static int fs_image_save_uboot(struct flash_info *fi,
			       struct region_info *atf_ri,
			       struct region_info *uboot_ri)
{
	int failed;
	int copy, start_copy;

	failed = 0;
	start_copy = fs_image_get_start_copy(true, true);
	copy = start_copy;
	do {
		printf("\nSaving copy %d to %s:\n", copy, fi->devname);
		if (atf_ri && fs_image_save_region(fi, copy, atf_ri))
			failed |= BIT(copy);
		if (fs_image_save_region(fi, copy, uboot_ri))
			failed |= BIT(copy);
		copy = 1 - copy;
	} while (copy != start_copy);

	return failed;
}

/* ------------- Generic Flash Handling ------------------------------------ */

/* Get flash information for given boot device (ro=true: open read-only) */
static int fs_image_get_flash_info(struct flash_info *fi, const void *fdt,
				   bool ro)
{
	int err;

	memset(fi, 0, sizeof(struct flash_info));

	err = fs_image_get_boot_dev(fdt, &fi->boot_dev, &fi->boot_dev_name);
	if (err)
		return err;

	/* Prepare flash information from where to load */
	switch (fi->boot_dev) {
#ifdef CONFIG_NAND_MXS
	case NAND_BOOT:
		err = fs_image_get_flash_nand(fi, 0, ro);
		break;
#endif

#ifdef CONFIG_MMC
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
		err = fs_image_get_flash_mmc(fi, fi->boot_dev - MMC1_BOOT, ro);
		break;
#endif
	default:
		printf("Cannot handle %s boot device\n", fi->boot_dev_name);
		return -ENODEV;
	}
	if (err)
		return err;

	fi->temp = malloc(fi->temp_size);
	if (!fi->temp) {
		puts("Cannot allocate temp buffer\n");
		return -ENOMEM;
	}
	memset(fi->temp, fi->temp_fill, fi->temp_size);

	return 0;
}

/* Clean up flash info */
static void fs_image_put_flash_info(struct flash_info *fi)
{
	fi->ops->put_flash(fi);
	free(fi->temp);
}

/* ------------- IVT Image Format (i.MX8M) --------------------------------- */

#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)

void fs_image_set_spl_secondary_bit(void *img, int copy)
{
	uint32_t *ivt = img;

	uint32_t spl_csf = ivt[6];
	uint32_t spl_self = ivt[5];
	uint32_t offset_csf = spl_csf - spl_self;
	uint32_t *real_csf = img + offset_csf;
	uint32_t *copy_addr = real_csf - 1;

	*copy_addr = copy;
}

/* Validate a signed image; Return 0: OK, <0: Error */
static int fs_image_validate_signed(const struct fs_header_v1_0 *fsh)
{
	struct fs_header_v1_0 *validate_addr;
	u32 size;

	validate_addr = fs_image_get_ivt_info(fsh, &size);
	if (!validate_addr || !size) {
		puts("Error: Bad IVT, validation impossible\n");
		return -EINVAL;
	}

#ifdef __UBOOT__
	/* Copy to verification address and check signature */
	debug("Copy 0x%x bytes from 0x%08lx to validation address 0x%08lx\n",
	      size, (ulong)fsh, (ulong)validate_addr);
	memcpy(validate_addr, fsh, size + FSH_SIZE);
	if (!fs_image_is_valid_signature(validate_addr)) {
		puts("Error: Invalid signature, refusing to save\n");
		return -EILSEQ;
	}

	puts("Signature OK\n");
#else
	/* ### TODO: actually check signature in Linux, too */
	{
		int err;

		printf("Skipping signature validation, just checking CRC32\n");
		err = fs_image_check_crc32(fsh);
		fs_image_print_crc32_status(fsh, err);
		if (err < 0)
			return err;
	}
#endif

	return 0;
}

/*
 * Search the subimage with given type/descr and add it to the region. Return
 * offset for next image or 0 in case of error.
 */
static uint fs_image_region_find_add(struct region_info *ri,
				     struct fs_header_v1_0 *fsh,
				     const char *type, const char *descr,
				     uint woffset, uint flags)
{
	fsh = (void *)fs_image_find(fsh, type, descr, NULL);
	if (!fsh) {
		printf("No %s found for %s\n", type, descr);
		return 0;
	}

	return fs_image_region_add(ri, fsh, type, descr, woffset, flags);
}

/*
 * Add a single F&S header with given data to the region. Return offset for
 * next subimage or 0 in case of error.
 */
static uint fs_image_region_add_fsh(struct region_info *ri,
				    struct fs_header_v1_0 *fsh,
				    const char *type,
				    const char *descr, uint woffset)
{
	fs_image_set_header(fsh, type, descr, 0, 0);

	return fs_image_region_add(ri, fsh, type, descr, woffset,
				   SUB_HAS_FS_HEADER);
}

static int fs_image_imx8m_load(ulong addr, bool load_uboot, ulong *im_size)
{
	struct sub_info sub;
	struct fs_header_v1_0 *nboot_fsh, *board_info_fsh, *board_cfg_fsh;
	struct fs_header_v1_0 *dram_info_fsh;
	struct flash_info fi;
	struct nboot_info ni;
	const void *fdt;
	const char *arch;
	const char *target = load_uboot ? "U-Boot" : "NBoot";

	nboot_fsh = (void *)addr;

	fdt = fs_image_get_cfg_fdt();
	if (fs_image_get_flash_info(&fi, fdt, true))
		return CMD_RET_FAILURE;

	if (fs_image_get_nboot_info(&fi, fdt, &ni, -1, false))
		goto fail;

	if (load_uboot) {
		if (fs_image_load_uboot(&fi, &ni, (void *)addr, 0, im_size))
			goto fail;

		goto success;
	}

	/* Load flash specific stuff (NAND: BCB, MMC: Secondary Image Table) */
	if (fi.ops->load_extra(&fi, &ni.spl, nboot_fsh + 1))
		goto fail;

	arch = fs_image_get_arch();

	/* Load SPL behind the NBOOT F&S header that is filled later */
	sub.type = "SPL";
	sub.descr = arch;
	sub.img = nboot_fsh + 1;
	sub.offset = 0;
	sub.flags = SUB_IS_SPL;
	if (fs_image_load_image(&fi, &ni.spl, &sub))
		goto fail;

	/* Load BOARD_CFG */
	board_info_fsh = sub.img;
	sub.type = "BOARD-CFG";
	sub.descr = NULL;
	board_cfg_fsh = board_info_fsh + 1;
	sub.img = board_cfg_fsh;
	sub.flags = SUB_HAS_FS_HEADER;
	if (fs_image_load_image(&fi, &ni.nboot, &sub))
		goto fail;

	/* If set, remove BOARD-ID rev (in file_size_high) and update CRC32 */
	if (board_cfg_fsh->info.file_size_high) {
		board_cfg_fsh->info.file_size_high = 0;
		debug("  ");
		fs_image_update_header(board_cfg_fsh,
				       fs_image_get_size(board_cfg_fsh, false),
				       board_cfg_fsh->info.flags);
	}

	/* Create BOARD-INFO header */
	sub.descr = arch;
	if (ni.flags & NI_SUPPORT_CRC32)
		sub.type = "BOARD-INFO";
	else
		sub.type = "BOARD-CONFIGS";
	fs_image_set_header(board_info_fsh, sub.type, sub.descr, sub.size, 0);

	if (ni.flags & NI_SUPPORT_U_ATF) {
		/* Load DRAM-FW behind DRAM-INFO/DRAM-TYPE (filled in later) */
		dram_info_fsh = sub.img;
		sub.type = "DRAM-FW";
		sub.descr = NULL;
		sub.img = dram_info_fsh + 2;
		sub.flags = SUB_HAS_FS_HEADER;
		sub.offset += sub.size;
		if (fs_image_load_image(&fi, &ni.nboot, &sub))
			goto fail;

		/* Load DRAM-TIMING */
		sub.type = "DRAM-TIMING";
		sub.descr = NULL;
		sub.flags = SUB_HAS_FS_HEADER;
		sub.offset += sub.size;
		if (fs_image_load_image(&fi, &ni.nboot, &sub))
			goto fail;

		/* Create DRAM-TYPE header, use descr from DRAM-FW */
		sub.type = "DRAM-TYPE";
		sub.descr = (dram_info_fsh + 2)->param.descr;
		fs_image_set_header(dram_info_fsh + 1, sub.type, sub.descr,
				    sub.img - (void *)(dram_info_fsh + 2), 0);

		/* Create DRAM-INFO header */
		sub.type = "DRAM-INFO";
		sub.descr = arch;
		fs_image_set_header(dram_info_fsh, sub.type, sub.descr,
				    sub.img - (void *)(dram_info_fsh + 1), 0);

		if (fs_image_is_u_atf(&fi, &ni.atf)) {
			puts("Skipping U-ATF/U-TEE\n");
		} else {
			/* Load ATF */
			sub.type = "ATF";
			sub.offset = 0;
			if (fs_image_load_image(&fi, &ni.atf, &sub))
				goto fail;

#ifdef CONFIG_OPTEE
			/* Load TEE */
			sub.type = "TEE";
			sub.offset += sub.size;
			if (fs_image_load_image(&fi, &ni.atf, &sub))
				goto fail;
#endif
		}
	} else {
		/* Simply load the whole FIRMWARE sub-image */
		sub.type = "FIRMWARE";
		sub.offset = ni.board_cfg_size ? ni.board_cfg_size : sub.size;
		if (fs_image_load_image(&fi, &ni.nboot, &sub))
			goto fail;
	}

	/* Fill overall NBOOT header */
	debug("  ");
	fs_image_set_header(nboot_fsh, "NBOOT", sub.descr,
			    sub.img - (void *)(nboot_fsh + 1),
			    FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE);

	/*
	 * Clear a word to invalidate any subsequent F&S images that we may
	 * have loaded there before, e.g. a second copy of an image.
	 */
	*(u32 *)sub.img = 0;

	if (im_size)
		*im_size = (ulong)sub.img - addr;

success:
	fs_image_put_flash_info(&fi);
	printf("%s successfully loaded to RAM\n", target);

	return CMD_RET_SUCCESS;

fail:
	fs_image_put_flash_info(&fi);
	printf("Failed to load %s\n", target);

	return CMD_RET_FAILURE;

}

/* Handle fsimage save if loaded image is a U-Boot image */
static int fs_image_save_imx8m_uboot(ulong addr, bool force,
				     bool system_atf, bool have_atf)
{
	const void *fdt;
	struct sub_info uboot_sub, atf_sub[2];
	struct region_info uboot_ri, atf_ri, *patf_ri = NULL;
	struct flash_info fi;
	struct nboot_info ni;
	int failed;
	uint flags;
	struct fs_header_v1_0 *fsh = (struct fs_header_v1_0 *)addr;
	const char *arch;
	const char *type;
	uint woffset = 0;

	fdt = fs_image_get_cfg_fdt();
	if (fs_image_get_flash_info(&fi, fdt, false))
		return CMD_RET_FAILURE;

	if (fs_image_get_nboot_info(&fi, fdt, &ni, -1, false))
		goto fail;

	arch = fs_image_get_arch();
	if (have_atf) {
		if (!(ni.flags & NI_SUPPORT_U_ATF)) {
			puts("U-Boot with ATF/TEE not supported."
			     " Maybe you need to update NBoot first.\n");
			goto fail;
		}
		if (system_atf) {
			puts("Skipping U-ATF/U-TEE on user request\n");
		} else {
			/* Create ATF region */
			patf_ri = &atf_ri;
			fs_image_region_create(patf_ri, &ni.atf, atf_sub);

			/* Add ATF image */
			type = "U-ATF";
			flags = SUB_HAS_FS_HEADER;
#ifdef CONFIG_OPTEE
			woffset = fs_image_region_find_add(patf_ri, fsh, type,
							   arch, woffset, flags);
			if (!woffset)
				goto fail;

			/* Add TEE image */
			type = "U-TEE";
#endif
			/* Last image, set SUB_SYNC */
			flags |= SUB_SYNC;
			woffset = fs_image_region_find_add(patf_ri, fsh, type,
							   arch, woffset, flags);
			if (!woffset)
				goto fail;
		}
	}

	fs_image_region_create(&uboot_ri, &ni.uboot, &uboot_sub);
	type = "U-BOOT";
	flags = SUB_SYNC;
	if (ni.flags & NI_UBOOT_WITH_FSH)
		flags |= SUB_HAS_FS_HEADER; /* Save with F&S header */
	if (!fs_image_region_find_add(&uboot_ri, fsh, type, arch, 0, flags))
		goto fail;

	/* Check if all prerequisites for U-Boot are valid */
	if (fi.ops->check_for_uboot(&ni.uboot, force))
		goto fail;

	/* ### TODO: set copy depending on Set A or B (or redundant copy) */
	failed = fs_image_save_uboot(&fi, patf_ri, &uboot_ri);
	fs_image_put_flash_info(&fi);

	return fs_image_show_save_status(failed, "U-Boot");

fail:
	fs_image_put_flash_info(&fi);
	puts("Failed to save U-Boot\n");

	return CMD_RET_FAILURE;
}

static int fs_image_imx8m_save(ulong addr, int boot_hwpart, bool force,
			       bool system_atf)
{
	struct index_info cfg_info = {0};
	struct fs_header_v1_0 *cfg_fsh;
	struct fs_header_v1_0 *nboot_fsh;
	struct fs_header_v1_0 firmware_fsh, dram_info_fsh, dram_type_fsh;
	struct fs_header_v1_0 cfg_fsh_bak;
	uint firmware_start = 0;
	uint dram_info_start = 0;
	uint dram_type_start = 0;
	struct region_info nboot_ri, spl_ri, atf_ri;
	struct region_info *patf_ri = NULL, *puatf_ri = NULL;
	struct sub_info nboot_sub[MAX_SUB_IMGS], spl_sub, atf_sub[2];
	void *uboot_addr, *env_addr, *envred_addr;
	struct region_info uboot_ri, env_ri, envred_ri;
	struct sub_info uboot_sub, env_sub, envred_sub;
	const char *arch = fs_image_get_arch();
	const char *type;
	uint flags;
	const void *fdt;
	int board_cfg_offs;
	int rev_offs;
	const char *dram_type;
	const char *dram_timing;
	struct nboot_info ni;
	struct flash_info fi;
	bool need_uboot = false, need_env = false;
	bool ignore_old;
	struct nboot_info ni_old;
	uint woffset;
	int ret = 0;
	int failed;

	/* If this is an U-Boot image, handle separately */
	if (fs_image_match((void *)addr, "U-BOOT", NULL))
		return fs_image_save_imx8m_uboot(addr, force, system_atf, false);
	if (fs_image_match((void *)addr, "U-BOOT-ATF", NULL))
		return fs_image_save_imx8m_uboot(addr, force, system_atf, true);

	/* Handle NBoot image */
	ret = fs_image_find_board_cfg(addr, force, "save", &cfg_info,
				      &nboot_fsh);
	if (ret <= 0)
		return CMD_RET_FAILURE;

	/*
	 * TODO: For non-Container Images,
	 * it is not expected to handle index structures.
	 */
	cfg_fsh = (struct fs_header_v1_0 *)cfg_info.fsh;
	ignore_old = (ret == 2);	/* Ignore old BOARD-CFG if ID changed */

	fdt = fs_image_find_cfg_fdt(cfg_fsh);
	board_cfg_offs = fs_image_get_board_cfg_offs(fdt);
	rev_offs = fs_image_get_board_rev_subnode(fdt, board_cfg_offs);

	dram_type = fs_image_getprop(fdt, board_cfg_offs, rev_offs,
				     "dram-type", NULL);
	dram_timing = fs_image_getprop(fdt, board_cfg_offs, rev_offs,
				       "dram-timing", NULL);
	if (!dram_type || !dram_timing) {
		puts("Error: No dram-type and/or dram-timing in BOARD-CFG\n");
		return CMD_RET_FAILURE;
	}

	if (fs_image_get_flash_info(&fi, fdt, false))
		return CMD_RET_FAILURE;

	if (fs_image_get_nboot_info(&fi, fdt, &ni, boot_hwpart, false))
		goto fail;

	ret = fs_image_check_boot_dev_fuses(fi.boot_dev, "save");
	if (ret < 0)
		goto fail;
	if (ret > 0) {
		printf("Warning! Boot fuses not yet set, remember to burn"
		       " them for %s\n", fi.boot_dev_name);
	}

	if (!ignore_old) {
		const void *fdt_old = fs_image_get_cfg_fdt();

		/* Check if U-Boot and/or environment need to be relocated */
		if (fs_image_get_nboot_info(&fi, fdt_old, &ni_old, -1, false))
			ignore_old = true;

		/* Check if there are changes */
		need_uboot = fi.ops->si_differs(&ni.uboot, &ni_old.uboot);
		need_env = fi.ops->si_differs(&ni.env, &ni_old.env);
	}

	/* Load U-Boot behind NBoot, if necessary */
	if (need_uboot) {
		puts("Need to move U-Boot\n");

		uboot_addr = (void *)nboot_fsh;
		uboot_addr += fs_image_get_size(uboot_addr, true);
		if (fs_image_load_uboot(&fi, &ni_old, uboot_addr, 0, NULL))
			goto fail;

		/* Create ATF region for U-ATF/U-TEE if present */
		if ((ni.flags & NI_SUPPORT_U_ATF)
		    && fs_image_match(uboot_addr, "U-BOOT-ATF", arch)) {
			puatf_ri = &atf_ri;
			fs_image_region_create(puatf_ri, &ni.atf, atf_sub);

			/* Add ATF image */
			type = "U-ATF";
			flags = SUB_HAS_FS_HEADER;
			woffset = 0;
#ifdef CONFIG_OPTEE
			woffset = fs_image_region_find_add(puatf_ri, uboot_addr,
							   type, arch, woffset,
							   flags);
			if (!woffset)
				goto fail;

			/* Add TEE image */
			type = "U-TEE";
#endif
			/* Last image, set SUB_SYNC */
			flags |= SUB_SYNC;
			woffset = fs_image_region_find_add(puatf_ri, uboot_addr,
							   type, arch, woffset,
							   flags);
			if (!woffset)
				goto fail;
			uboot_addr += woffset;
		}

		/* Prepare U-BOOT region with one sub-image */
		fs_image_region_create(&uboot_ri, &ni.uboot, &uboot_sub);

		flags = SUB_SYNC;
		if (ni.flags & NI_UBOOT_WITH_FSH)
			flags |= SUB_HAS_FS_HEADER; /* Save with F&S header */
		if (!fs_image_region_add(&uboot_ri, uboot_addr, "U-BOOT",
					 arch, 0, flags))
			goto fail;

		/* Check if all prerequisites for U-Boot are valid */
		if (fi.ops->check_for_uboot(&ni.uboot, force))
			goto fail;
	}

	/* Load Environment behind NBoot (or U-Boot), if necessary */
	if (need_env) {
		puts("Need to move U-Boot Environment\n");
		printf("Loading ENV from %s\n", fi.devname);

		env_addr = need_uboot ? uboot_addr : nboot_fsh;
		env_addr += fs_image_get_size(env_addr, true);
		ret = fs_image_load_env(&fi, &ni_old.env, env_addr, 0);
		if (ret)
			goto fail;

		envred_addr = env_addr + fs_image_get_size(env_addr, true);
		ret = fs_image_load_env(&fi, &ni_old.env, envred_addr, 1);
		if (ret)
			goto fail;

		/* Prepare ENV region with one sub-image */
		fs_image_region_create(&env_ri, &ni.env, &env_sub);
		if (!fs_image_region_add(&env_ri, env_addr, "ENV",
					 arch, 0, SUB_SYNC))
			goto fail;

		/* Prepare ENV-RED region with one sub-image */
		fs_image_region_create(&envred_ri, &ni.env, &envred_sub);
		if (!fs_image_region_add(&envred_ri, envred_addr, "ENV-RED",
					 arch, 0, SUB_SYNC))
			goto fail;
	}

	/*
	 * NBoot Layout in flash memory
	 * ----------------------------
	 *
	 * without U-ATF support:                         with U-ATF support:
	 * SPL region:                     nboot-info     SPL region:
	 * +----------------------------+ <-spl_start---> +--------------------+
	 * | 0: SPL                     |                 | 0: SPL             |
	 * +----------------------------+                 +--------------------+
	 *
	 * NBOOT region:                                  NBOOT region:
	 * +----------------------------+ <-nboot_start-> +--------------------+
	 * | 0: BOARD-CFG (padded)|     |                 | 0: BOARD-CFG       |
	 * +----------------------------+                 +--------------------+
	 * | 1: FIRMWARE                |                 | 1: DRAM-FW         |
	 * |   +------------------------+                 +--------------------+
	 * |   | 2: DRAM-INFO           |                 | 2. DRAM-TIMING     |
	 * |   |   +--------------------+                 +--------------------+
	 * |   |   | 3: DRAM-TYPE       |
	 * |   |   |   +----------------+
	 * |   |   |   | 4: DRAM-FW     |
	 * |   |   |   +----------------+
	 * |   |   |   | 5: DRAM-TIMING |                 ATF region:
	 * |   +---+---+----------------+     atf_start-> +--------------------+
	 * |   | 6: ATF                 |                 | 0: ATF/U-ATF       |
	 * |   +------------------------+                 +--------------------+
	 * |   | 7: TEE (opt.)          |                 | 1: TEE/U-TEE (opt.)|
         * +---+------------------------+                 +--------------------+
	 *
	 * The numbers indicate the index of the sub-image within the region.
	 */

	/* Prepare subimages for NBOOT region */
	fs_image_region_create(&nboot_ri, &ni.nboot, nboot_sub);

	/* Start with BOARD-CFG */
	flags = SUB_HAS_FS_HEADER;
	if (ni.board_cfg_size)
		flags |= SUB_SYNC;
	woffset = fs_image_region_add(&nboot_ri, cfg_fsh, "BOARD-CFG",
				      arch, 0, flags);
	if (!woffset)
		goto fail;

	/* Very old NBoots need BOARD-CFG padded to 8KB (board_cfg_size) */
	if (ni.board_cfg_size)
		woffset = ni.board_cfg_size;

	/* FIRMWARE/DRAM-INFO/DRAM-TYPE only needed for old NBoots */
	if (!(ni.flags & NI_SUPPORT_U_ATF)) {
		/* Add a FIRMWARE header */
		woffset = fs_image_region_add_fsh(&nboot_ri, &firmware_fsh,
						  "FIRMWARE", arch, woffset);
		if (!woffset)
			goto fail;
		firmware_start = woffset;

		/* Add a DRAM-INFO/SETTINGS header */
		if (ni.flags & NI_SUPPORT_CRC32)
			type = "DRAM-INFO";
		else
			type = "DRAM-SETTINGS";
		woffset = fs_image_region_add_fsh(&nboot_ri, &dram_info_fsh,
						  type, arch, woffset);
		if (!woffset)
			goto fail;
		dram_info_start = woffset;

		/* Add a DRAM-TYPE header */
		woffset = fs_image_region_add_fsh(&nboot_ri, &dram_type_fsh,
						  "DRAM-TYPE", dram_type,
						  woffset);
		if (!woffset)
			goto fail;
		dram_type_start = woffset;
	}

	/* Add the DRAM-FW image needed on this board */
	flags = SUB_HAS_FS_HEADER;
	woffset = fs_image_region_find_add(&nboot_ri, nboot_fsh, "DRAM-FW",
					   dram_type, woffset, flags);
	if (!woffset)
		goto fail;

	/*
	 * Add the DRAM-TIMING image needed on this board; in case of U-ATF
	 * support, this is the last image of the NBOOT region and ATF/TEE go
	 * to the separate ATF region.
	 */
	if (ni.flags & NI_SUPPORT_U_ATF)
		flags |= SUB_SYNC;
	woffset = fs_image_region_find_add(&nboot_ri, nboot_fsh, "DRAM-TIMING",
					   dram_timing, woffset, flags);
	if (!woffset)
		goto fail;

	if (ni.flags & NI_SUPPORT_U_ATF) {
		if (!system_atf && fs_image_is_u_atf(&fi, &ni.atf)) {
			printf("Skipping ATF/TEE because U-ATF is present\n");
		} else {
			system_atf = true;

			/* Start ATF region */
			fs_image_region_create(&atf_ri, &ni.atf, atf_sub);
			patf_ri = &atf_ri;
			woffset = 0;
		}
	} else {
		/* Always store ATF/TEE */
		system_atf = true;

		/* Update size and CRC32 (header only) for DRAM-TYPE header */
		fs_image_update_header(&dram_type_fsh, woffset - dram_type_start,
				       FSH_FLAGS_SECURE);

		/*
		 * Update size and CRC32 (header only) for DRAM-INFO header;
		 * in case of "DRAM-SETTINGS", the type string is too long and
		 * there is no room for the CRC32.
		 */
		fs_image_update_header(&dram_info_fsh, woffset - dram_info_start,
			   (ni.flags & NI_SUPPORT_CRC32) ? FSH_FLAGS_SECURE : 0);
	}

	if (system_atf) {
		/* Add ATF image */
		type = "ATF";
		flags = SUB_HAS_FS_HEADER;
#ifdef CONFIG_OPTEE
		woffset = fs_image_region_find_add(patf_ri, nboot_fsh, type,
						   arch, woffset, flags);
		if (!woffset)
			goto fail;

		/* Add TEE image */
		type = "TEE";
#endif
		/* Last image, set SUB_SYNC */
		flags |= SUB_SYNC;
		woffset = fs_image_region_find_add(patf_ri, nboot_fsh, type,
						   arch, woffset, flags);
		if (!woffset)
			goto fail;
	}

	if (!(ni.flags & NI_SUPPORT_U_ATF)) {
		/* Update size and CRC32 (header only) for FIRMWARE */
		fs_image_update_header(&firmware_fsh, woffset - firmware_start,
				       FSH_FLAGS_SECURE);
	}

	/* Prepare SPL region: SPL */
	fs_image_region_create(&spl_ri, &ni.spl, &spl_sub);
	woffset = fs_image_region_find_add(&spl_ri, nboot_fsh, "SPL",
					   arch, 0, SUB_IS_SPL | SUB_SYNC);
	if (!woffset)
		goto fail;

	if (fi.ops->check_for_nboot(&fi, &ni.spl, force))
		goto fail;

	/* Temporarily set BOARD-ID board revision and update CRC32 */
	if (ni.flags & NI_SAVE_BOARD_ID) {
		cfg_fsh_bak = *cfg_fsh;
		fs_image_board_cfg_set_board_rev(cfg_fsh);
	}

	/* Set up final boot hwpart */
	if (fi.ops->set_boot_hwpart(&fi, boot_hwpart))
		goto fail;

	/* --- Found all sub-images, everything is prepared, go and save --- */

	/* Save U-Boot if needed */
	failed = 0;
	if (need_uboot) {
		int uboot_failed = fs_image_save_uboot(&fi, puatf_ri, &uboot_ri);

		if (!failed || (uboot_failed == 3))
			failed = uboot_failed;
	}

	/* Save ENV if needed */
	if ((failed != 3) && need_env) {
		int env_failed = 0;

		printf("\nSaving copy 0 to %s:\n", fi.devname);
		if (fs_image_save_region(&fi, 0, &env_ri))
			env_failed |= BIT(0);

		printf("\nSaving copy 1 to %s:\n", fi.devname);
		if (fs_image_save_region(&fi, 1, &envred_ri))
			env_failed |= BIT(1);

		if (failed || (env_failed == 3))
			failed = env_failed;
	}

	/* Finally save NBOOT */
	if (failed != 3) {
		int nboot_failed;

		nboot_failed = fi.ops->save_nboot(&fi, &nboot_ri, patf_ri,
						  &spl_ri);
		if (!failed || (nboot_failed == 3))
			failed = nboot_failed;
	}

	fs_image_put_flash_info(&fi);

	ret = fs_image_show_save_status(failed, "NBoot");

	if (ret == CMD_RET_SUCCESS) {
		/* Success: Activate new BOARD-CFG by copying it to OCRAM */
		memcpy(fs_image_get_cfg_addr(), cfg_fsh,
		       fs_image_get_size(cfg_fsh, true));
		puts("New BOARD-CFG is now active\n");
	}

	/* Restore BOARD-CFG header to previous content */
	if (ni.flags & NI_SAVE_BOARD_ID)
		*cfg_fsh = cfg_fsh_bak;

	return ret;

fail:
	/* Free flash_info and return error */
	fs_image_put_flash_info(&fi);
	puts("Failed to save NBoot\n");

	return CMD_RET_FAILURE;
}

#endif /* !CONFIG_IS_ENABLED(FS_CNTR_COMMON) */

/* ------------- Container Image Format (i.MX8ULP/i.MX9) ------------------- */

#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
struct _image_list{
	struct fs_header_v1_0 *fsh;
	struct _image_list *next;
};

static int fs_image_validate_signed(const struct fs_header_v1_0 *fsh)
{
	if (!fs_image_is_valid_signature(fsh)) {
		puts("Error: Invalid signature, refusing to save\n");
		return -EILSEQ;
	}

	puts("Signature OK\n");
	return 0;
}

/* append fsh at end of img list */
static int append_image_list(struct _image_list *img_list,
			     struct fs_header_v1_0 *fsh)
{
	struct _image_list *ptr = img_list;
	struct _image_list *next_entry;

	next_entry = malloc(sizeof(struct _image_list));
	if (!next_entry)
		return -ENOMEM;

	next_entry->fsh = fsh;
	next_entry->next = NULL;

	while (ptr->next)
		ptr = ptr->next;

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

static void free_image_list(struct _image_list *img_list)
{
	struct _image_list *tmp;

	while (img_list) {
		tmp = img_list;
		img_list = img_list->next;
		free(tmp);
	}
}

/* Create list of padded Images */
static int create_image_list(struct _image_list **img_list, ulong addr,
			     uint *file_size)
{
	struct fs_header_v1_0 *fsh = (void *)addr;
	uint size = 0;
	int ret = 0;

	*img_list = malloc(sizeof(struct _image_list));
	if (!(*img_list))
		return -ENOMEM;

	/* set first entry */
	if (!fs_image_is_fs_image(fsh)) {
		free(*img_list);
		return -EINVAL;
	}

	(*img_list)->fsh = fsh;
	(*img_list)->next = NULL;

	addr += fs_image_get_size(fsh, true);
	size += fs_image_get_size(fsh, true);
	fsh = (void *)addr;

	/* set entries for padded images */
	while (fs_image_is_fs_image(fsh)) {
		ret = append_image_list(*img_list, fsh);
		if (ret) {
			free_image_list(*img_list);
			return ret;
		}
		addr += fs_image_get_size(fsh, true);
		size += fs_image_get_size(fsh, true);
		fsh = (void *)addr;
	}

	if (file_size)
		*file_size = size;

	return 0;
}

static bool is_img_list_valid(struct _image_list *img_list, ulong addr)
{
	struct _image_list *ptr;
	bool ret = true;

	if (!img_list)
		return false;

	ptr = img_list;

	while (ptr) {
		if (fs_image_match(ptr->fsh, "BOARD-ID", NULL)) {
			ptr = ptr->next;
			continue;
		}

		if (fs_image_validate(ptr->fsh, ptr->fsh->type, NULL,
				      (ulong)ptr->fsh - addr))
			ret = false;

		ptr = ptr->next;
	}

	return ret;
}

static struct fs_header_v1_0 *get_fsh_from_list(struct _image_list *img_list,
						const char* type,
						const char* descr)
{
	struct _image_list *ptr = img_list;

	while (ptr) {
		if (fs_image_match(ptr->fsh, type, descr))
			return ptr->fsh;

		ptr = ptr->next;
	}

	return NULL;
}

static uint get_nboot_cntr_size(struct _image_list *img_list)
{
	struct _image_list *ptr = img_list;
	ulong size = 0;

	if (!img_list)
		return 0;

	/* skip BOOT-INFO */
	ptr = ptr->next;
	while (ptr) {
		if (fs_image_match(ptr->fsh, "U-BOOT-INFO", NULL)
		    || fs_image_match(ptr->fsh, "ENV", NULL))
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
static int fsimage_cntr_load(ulong addr, bool load_uboot, int boot_hwpart,
			     ulong *im_size)
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
	const void *fdt;
	int i;
	int ret;
	const char *target = load_uboot ? "U-Boot" : "NBoot";

	fdt = fs_image_get_cfg_fdt();
	if (fs_image_get_flash_info(&fi, fdt, true))
		return CMD_RET_FAILURE;

	if (fs_image_get_nboot_info(&fi, fdt, &ni, -1, false))
		goto fail;

	if (load_uboot) {
		if (!ni.uboot.start[0] || !ni.uboot.start[1]) {
			puts("Missing U.Boot start address,"
			     " try to load complete Firmware.\n");
			goto fail;
		}

		if (fs_image_load_uboot(&fi, &ni, (void *)addr, 0, im_size))
			goto fail;

		goto success;
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

	/*
	 * BOOT-INFO provides two Container.
	 * search directly for the second container, which is 1KiB aligned
	 */
	for (i = 1; i < 8; i++)	{
		cntr = (void *)(fsh + 1);
		cntr = (void *)((ulong)cntr + (i * CONTAINER_HDR_ALIGNMENT));
		debug("search imx_cntr at 0x%lx\n", (ulong)cntr);
		if (valid_container_hdr(cntr))
			break;
	}

	if (i >= 8) {
		puts("Failed to find BOOT-INFO Container\n");
		goto fail;
	}
	debug("found cntr at 0x%lx\n", (ulong)cntr);
	filesize = i * CONTAINER_HDR_ALIGNMENT;
	filesize += get_container_size((ulong)cntr, NULL);
	filesize += 0x380; // PADDING TO NEXT FSH
	img_entry = (struct boot_img_t *)
		((ulong)cntr + sizeof(struct container_hdr));

	/* Update FSH and set Extra Data offset */
	fs_image_update_header(fsh, filesize,
			FSH_FLAGS_CRC32 | FSH_FLAGS_INDEX |FSH_FLAGS_EXTRA);
	fsh->param.p32[7] = i * CONTAINER_HDR_ALIGNMENT;
	fsh->param.p32[7] += img_entry->offset;

	/*
	 * Load all Images including U-Boot
	 * Search until 3MiB is loaded.
	 * U-Boot must be placed within 3MiB!.
	 */
	for (i = 0, fsh = NULL; i < 5; i++) {
		ret = create_image_list(&img_list, addr, &filesize);
		if (ret)
			break;

		fsh = get_fsh_from_list(img_list, "U-BOOT-INFO", NULL);
		if (fsh)
			break;

		/* Load next 512KiB and search again */
		free_image_list(img_list);
		img_list = NULL;
		lim = flash_offset + size;
		debug("load 0x%x at 0x%x into 0x%lx", size, flash_offset,
		      ram_offset);
		fi.ops->read(&fi, flash_offset, size, lim, 0,
			     (void *)(ram_offset));
		flash_offset += size;
		ram_offset += size;
	}

	if (!fsh)
		goto fail;

	debug("found U-BOOT at 0x%lx", (ulong)fsh);
	/* load rest of U-BOOT */
	if (filesize > flash_offset) {
		lim = filesize;
		debug("load 0x%x at 0x%x into %0lx", size, flash_offset,
		      ram_offset);
		fi.ops->read(&fi, flash_offset, filesize - flash_offset,
			     lim, 0, (void *)(ram_offset));
		flash_offset += filesize - flash_offset;
		ram_offset += filesize - flash_offset;
	}

	//### Is this necessary? Stored files should be valid, toggled bits are
	//### found when loading. If not, we could drop	is_img_list_valid().
	if (!is_img_list_valid(img_list, addr)) {
		puts("WARNING: Firmware is invalid\n");
		goto fail;
	}

	if (im_size)
		*im_size = ram_offset - addr;

success:
	fs_image_put_flash_info(&fi);
	free_image_list(img_list);
	printf("%s loaded successfully to RAM\n", target);

	return CMD_RET_SUCCESS;

fail:
	fs_image_put_flash_info(&fi);
	free_image_list(img_list);
	printf("Failed to load %s\n", target);

	return CMD_RET_FAILURE;

}

static int prepare_nboot_cntr_images(ulong addr, const void *fdt_new,
				     struct flash_info *fi,
				     struct region_info *spl_ri,
				     struct region_info *nboot_ri,
				     struct region_info *uboot_ri,
				     struct region_info *env_ri,
				     struct nboot_info *ni_new)
{
	struct _image_list *img_list = NULL;
	struct _image_list *ptr;
	struct fs_header_v1_0 *tmp_fsh;
	struct nboot_info ni_old;
	const char *arch = fs_image_get_arch();
	const char board_id[MAX_DESCR_LEN + 1] = {0};
	const char *dram_type;
	const void *fdt_old = fs_image_get_cfg_fdt();
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

	if (!fs_image_match(ptr->fsh, "BOOT-INFO", arch)) {
		free_image_list(img_list);
 		return -EINVAL;
	}

	ptr = ptr->next;

	if (ptr && !fs_image_match(ptr->fsh, "BOARD-ID", NULL)) {
		free_image_list(img_list);
		return -EINVAL;
	}

	/* Update Board-ID Description */
	memset(ptr->fsh->param.descr, 0, MAX_DESCR_LEN);
	strncpy(ptr->fsh->param.descr, board_id, MAX_DESCR_LEN);

	/* Update CRC32 if available*/
	fs_image_update_header(ptr->fsh, fs_image_get_size(ptr->fsh, false),
			       ptr->fsh->info.flags);

	/* get current DRAM-INFO descrp*/
 	dram_type = fs_image_getprop(fdt_new, offs, rev_offs, "dram-type", NULL);
 	if (!dram_type) {
		free_image_list(img_list);
		return -EINVAL;
	}

	/* remove unneeded Container or FS-Images */
	while (ptr->next) {
		/* Remove unneeded DRAM-INFO */
		if (fs_image_match(ptr->next->fsh, "DRAM-INFO", NULL)
		    &&!fs_image_match(ptr->next->fsh, "DRAM-INFO", dram_type)) {
			remove_next_image(ptr);
			continue;
		}

		/* Remove EXTRA */
		if (fs_image_match(ptr->next->fsh, "EXTRA", NULL)) {
			remove_next_image(ptr);
			continue;
		}

		ptr = ptr->next;
	}

	/* Check for DRAM-CNTR */
	if (!get_fsh_from_list(img_list, "DRAM-INFO", dram_type)) {
		free_image_list(img_list);
		return -EINVAL;
	}

	/* --- get nboot-info --- */
	ret = fs_image_get_nboot_info(fi, fdt_new, ni_new, -1, false);
	if (ret) {
		free_image_list(img_list);
		return ret;
	}

	/*
	 * Update new nboot info
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
	if (ret) {
		free_image_list(img_list);
		return ret;
	}

	/* --- Get missing Images --- */
	tmp_fsh = get_fsh_from_list(img_list, "U-BOOT-INFO", arch);
	if (!tmp_fsh) {
		/*
		 * Check if U-Boot and/or environment need to be relocated.
		 * Old NBOOT-Info is only available, if device boots from flash.
		 * These Informations can not be set during compile-time.
		 * Therefore SPL will provide missing NBOOT-INFOs during
		 * fs_handle_uboot(). When Infos are missing, than load complete
		 * firmware into $loadaddr.
		 */
		if (ni_old.uboot.start[0] && ni_old.uboot.size) {
			/*
			 * Since new U-Boot is not provided, we need to know
			 * the old size
			 */
			ni_new->uboot.size = ni_old.uboot.size;

			/* Check if there are changes */
			need_uboot = fi->ops->si_differs(&ni_new->uboot,
							 &ni_old.uboot);
			need_env = fi->ops->si_differs(&ni_new->env,
						       &ni_old.env);
		} else {
			puts("WARNING: U-Boot missing. System will not BOOT!\n"
			     "WARNING: Load U-Boot and re-run fsimage save!\n");
		}
	} else {
		ni_new->uboot.size = fs_image_get_size(tmp_fsh, true);
	}

	/* load U-Boot from Flash, if needed */
	if (need_uboot) {
		puts("Need to move U-BOOT-INFO\n");
		uboot_addr = addr + (ulong)file_size;
		ret = fs_image_load_uboot(fi, &ni_old, (void *)uboot_addr,
					  SUB_HAS_FS_HEADER, NULL);
		if (ret) {
			free_image_list(img_list);
			return -EIO;
		}

		if (!fs_image_match((void *)uboot_addr, "U-BOOT-INFO", arch)) {
			free_image_list(img_list);
			return -EINVAL;
		}

		ni_new->uboot.size = fs_image_get_size((void *)uboot_addr, true);
		file_size += ni_new->uboot.size;

		ret = append_image_list(img_list, (void *)uboot_addr);
		if (ret) {
			free_image_list(img_list);
			return ret;
		}
	}

	/* Load Env from Flash, if Needed */
	if (need_env) {
		puts("Need to move U-Boot Environment\n");
		printf("Loading ENV from %s\n", fi->devname);

		env_addr = addr + (ulong)file_size;

		ret = fs_image_load_env(fi, &ni_old.env, (void *)env_addr, 0);
		if (ret) {
			free_image_list(img_list);
			return -EIO;
		}

		file_size += fs_image_get_size((void *)env_addr, true);

		ret = append_image_list(img_list, (void *)env_addr);
		if (ret) {
			free_image_list(img_list);
			return ret;
		}
	}

	ptr = img_list;

	/* --- Prepare SPL region --- */
	woffset = fs_image_region_add(spl_ri, ptr->fsh, ptr->fsh->type,
					   arch, 0, SUB_IS_SPL | SUB_SYNC);
	if (!woffset) {
		free_image_list(img_list);
		return -ENOMEM;
	}

	ptr = ptr->next;
	woffset = 0;

	/* --- Prepare NBOOT region --- */
	while (ptr) {
		if (fs_image_match(ptr->fsh, "U-BOOT-INFO", arch)
		    || fs_image_match(ptr->fsh, "ENV", NULL))
			break;

		woffset = fs_image_region_add(nboot_ri, ptr->fsh, ptr->fsh->type,
					      ptr->fsh->param.descr, woffset,
					      SUB_HAS_FS_HEADER | SUB_SYNC);

		if (!woffset) {
			free_image_list(img_list);
			return -ENOMEM;
		}

		ptr = ptr->next;
	}

	/* --- Prepare UBOOT Region --- */
	tmp_fsh = get_fsh_from_list(img_list, "U-BOOT-INFO", arch);
	if (tmp_fsh) {
		struct fs_header_v1_0 *uboot_fsh = tmp_fsh;

		woffset = fs_image_region_add(uboot_ri, uboot_fsh,
					      uboot_fsh->type,
					      uboot_fsh->param.descr,
					      0, SUB_HAS_FS_HEADER | SUB_SYNC);

		if (!woffset) {
			free_image_list(img_list);
			return -ENOMEM;
		}
	}

	tmp_fsh = get_fsh_from_list(img_list, "ENV", arch);
	if (tmp_fsh) {
		struct fs_header_v1_0 *env_fsh = tmp_fsh;

		woffset = fs_image_region_add(env_ri, env_fsh, env_fsh->type,
					      env_fsh->param.descr, 0, SUB_SYNC);

		if (!woffset) {
			free_image_list(img_list);
			return -ENOMEM;
		}
	}

	free_image_list(img_list);

	return 0;
}

/* Update nboot-info of BOARD-CFG in OCRAM (U-Boot only) */
static void update_board_cfg(struct nboot_info *ni)
{
#ifdef __UBOOT__
	uint uboot_size, nboot_size, uboot_offset;
	void *fdt = (void *)fs_image_get_cfg_fdt();
	int offs;

	nboot_size = cpu_to_fdt32(ni->nboot.size);
	uboot_offset = cpu_to_fdt32(ni->uboot.start[0]);
	uboot_size = cpu_to_fdt32(ni->uboot.size);

	offs = fdt_path_offset(fdt, "/nboot-info/emmc-boot");
	if (offs < 0)
		return;

	fdt_setprop(fdt, offs, "nboot-size", &nboot_size, sizeof(uint));
	fdt_setprop(fdt, offs, "uboot-start", &uboot_offset, sizeof(uint));
	fdt_setprop(fdt, offs, "uboot-size", &uboot_size, sizeof(uint));
#endif /* __UBOOT__ */
}

static int fsimage_cntr_save_uboot(ulong addr, uint boot_hwpart, bool force)
{
	struct fs_header_v1_0 *uboot_fsh = (void *)addr;
	struct flash_info fi;
	struct nboot_info ni;
	struct region_info uboot_ri;
	struct sub_info uboot_sub;
	struct fs_header_v1_0 *cfg_fsh = fs_image_get_cfg_addr();
	uint cfg_size = fs_image_get_size(cfg_fsh, false);
	const void *fdt = fs_image_get_cfg_fdt();
	int ret = CMD_RET_SUCCESS;

	fs_image_region_create(&uboot_ri, &ni.uboot, &uboot_sub);

	if (fs_image_get_flash_info(&fi, fdt, false))
		return CMD_RET_FAILURE;

	if (fs_image_get_nboot_info(&fi, fdt, &ni, -1, false))
		goto fail;

	if (!ni.uboot.start[0]) {
		puts("Unknown U-Boot start address.\n"
		     "Boot from MMC or provide complete Firmware"
		     " (NBOOT + UBOOT) in RAM\n");
		goto fail;
	}

	ni.uboot.size = fs_image_get_size(uboot_fsh, true);

	/* --- Prepare UBOOT Region --- */
	if (!fs_image_region_add(&uboot_ri, uboot_fsh, uboot_fsh->type,
				 uboot_fsh->param.descr, 0,
				 SUB_HAS_FS_HEADER | SUB_SYNC))
		goto fail;

	ret = fs_image_save_uboot(&fi, NULL, &uboot_ri);
	ret = fs_image_show_save_status(ret, "U-Boot");

	fs_image_put_flash_info(&fi);
	if (ret == CMD_RET_SUCCESS) {
		update_board_cfg(&ni);

		/* calc new crc32 */
		fs_image_update_header(cfg_fsh, cfg_size, cfg_fsh->info.flags);
	}

	return ret;

fail:
	printf("Failed to save U-Boot\n");
	fs_image_put_flash_info(&fi);

	return CMD_RET_FAILURE;
}

static int fsimage_cntr_save(ulong addr, int boot_hwpart, bool force)
{
	const char *arch = fs_image_get_arch();
	struct index_info cfg_info = {0};
	struct flash_info fi;
	struct nboot_info ni_new;
	struct region_info nboot_ri, spl_ri;
	struct region_info uboot_ri, env_ri;
	struct sub_info spl_sub, nboot_sub[MAX_SUB_IMGS];
	struct sub_info uboot_sub, env_sub;
	const void *fdt_new;
	int failed = 0;

	int ret = CMD_RET_SUCCESS;

	/* If this is an U-Boot image, skip nboot handling */
	if (fs_image_match((void *)addr, "U-BOOT-INFO", NULL))
		return fsimage_cntr_save_uboot(addr, boot_hwpart, force);

	if (!fs_image_match((void *)addr, "BOOT-INFO", arch)
	    && !fs_image_match((void *)addr, "BOARD-ID", NULL))
		return CMD_RET_FAILURE;

	/* This call will set new board-id if available */
	if (fs_image_find_board_cfg(addr, force, "save", &cfg_info, NULL) <= 0)
		return CMD_RET_FAILURE;

	fdt_new = fs_image_find_cfg_fdt_idx(&cfg_info);
	if (!fdt_new)
		return CMD_RET_FAILURE;

	/* When new ID is provided, skip to BOOT-INFO */
	if (fs_image_match((void *)addr, "BOARD-ID", NULL))
		addr += fs_image_get_size((void *)addr, true);

	/* Get Flash-Info */
	if (fs_image_get_flash_info(&fi, fdt_new, false))
		return CMD_RET_FAILURE;

	ret = fs_image_check_boot_dev_fuses(fi.boot_dev, "save");
	if (ret < 0)
		goto fail;
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

	if (prepare_nboot_cntr_images(addr, fdt_new, &fi, &spl_ri, &nboot_ri,
				      &uboot_ri, &env_ri, &ni_new))
		goto fail;

	/* --- Found all sub-images, everything is prepared, go and save --- */

	/*
	 *  Save is done in two stages.
	 *  In the first stage all new Images are stored as Secondary.
	 *  Then the Images are stored as Primary
	 */

	/* Save BOOT-INFO + NBOOT */
	failed = fi.ops->save_nboot(&fi, &nboot_ri, NULL, &spl_ri);

	/* Save U-Boot if needed */
	if ((failed != 3) && uboot_ri.count) {
		int uboot_failed = fs_image_save_uboot(&fi, NULL, &uboot_ri);

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
			env_failed |= BIT(1);

		if (failed || (env_failed == 3))
			failed = env_failed;
	}

	fs_image_flush_temp(&fi, 0, 0);


	if (fs_image_show_save_status(failed, "NBoot"))
		goto fail;

	/* Success: Activate new BOARD-CFG by copying it to OCRAM */
	memcpy(fs_image_get_cfg_addr(), cfg_info.fsh, FSH_SIZE);
	memcpy(fs_image_get_cfg_addr() + FSH_SIZE, cfg_info.fsi,
	       fs_image_get_size(cfg_info.fsh, false));

	update_board_cfg(&ni_new);
	fs_image_board_cfg_set_board_rev(fs_image_get_cfg_addr());
	puts("New BOARD-CFG is now active\n");

	fs_image_put_flash_info(&fi);

	return CMD_RET_SUCCESS;

fail:
	puts("Failed to save NBoot\n");
	fs_image_put_flash_info(&fi);

	return CMD_RET_FAILURE;
}
#endif /* CONFIG_IS_ENABLED(FS_CNTR_COMMON) */

/* ------------- Generic Command Implementation ---------------------------- */

/* Show the F&S architecture */
int fs_image_do_arch(int argc, char * const argv[])
{
	if (argc > 1)
		return CMD_RET_USAGE;

	printf("%s\n", fs_image_get_arch());

	return CMD_RET_SUCCESS;
}

/* Show the current BOARD-ID */
int fs_image_do_boardid(int argc, char * const argv[])
{
	if (argc > 1)
		return CMD_RET_USAGE;

	printf("%s\n", fs_image_get_board_id());

	return CMD_RET_SUCCESS;
}

#ifdef CONFIG_CMD_FDT
/* Print FDT content of current BOARD-CFG */
int fs_image_do_boardcfg(int argc, char * const argv[])
{
	ulong addr;
	int ret;
	const void *fdt = fs_image_get_cfg_fdt();
	struct index_info cfg_info = {0};

	argv++;
	argc--;

	if ((argc == 1) && !strncmp(argv[0], "stored", strlen(argv[0]))) {
		cfg_info.fsh = fs_image_get_cfg_addr();
	} else {
		ret = fs_image_locate_nboot(argc, argv, &addr);
		if (ret)
			return ret;

		ret = fs_image_find_board_cfg(addr, true, "show", &cfg_info,
					      NULL);
		if (ret <= 0)
			return CMD_RET_FAILURE;
	}

	fdt = fs_image_find_cfg_fdt_idx(&cfg_info);
	if (!fdt)
		return CMD_RET_FAILURE;

	printf("FDT part of BOARD-CFG located at 0x%lx\n", (ulong)fdt);

	return fdt_print((void *)fdt, "/", NULL, 5, ULONG_MAX);
}
#endif

/* Show current boot settings */
int fs_image_do_boot(int argc, char * const argv[])
{
	const void *fdt;
	struct flash_info fi;
	struct nboot_info ni;
	int ret;

	if (argc > 1)
		return CMD_RET_USAGE;

	early_support_index = 0;

	/* Output is actually done in fs_image_get_nboot_info() */
	fdt = fs_image_get_cfg_fdt();
	if (fs_image_get_flash_info(&fi, fdt, true))
		return CMD_RET_FAILURE;

	ret = fs_image_get_nboot_info(&fi, fdt, &ni, -1, true);
	fs_image_put_flash_info(&fi);

	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

/* List contents of an F&S image */
int fs_image_do_list(int argc, char * const argv[])
{
	struct index_info idx_info = {0};
	enum parse_type ptype = PARSE_CONTENT;
	ulong addr;
	const char *type = NULL;
	const char *descr = NULL;
	uint offs;
	int ret;
	const char *headline = "\nContent:\n\n"
	     "offset   size     flags type (description)\n";

	early_support_index = 0;

	argv++;
	argc--;

	while ((argc > 0) && (argv[0][0] == '-')) {
		if (!strcmp(argv[0], "-c")) {
			ptype = PARSE_CHECKSUM;
			argv++;
			argc--;
		} else if (!strcmp(argv[0], "-t")) {
			if (argc < 2) {
				puts("Missing argument for option -t\n");
				return CMD_RET_USAGE;
			}
			type = argv[1];
			argv += 2;
			argc -= 2;
		} else if (!strcmp(argv[0], "-d")) {
			if (argc < 2) {
				puts("Missing argument for option -d\n");
				return CMD_RET_USAGE;
			}
			descr = argv[1];
			argv += 2;
			argc -= 2;
		} else
			return CMD_RET_USAGE;
	}

	if (descr && !type)
		return CMD_RET_USAGE;	/* A description also needs a type */

	ret = fs_image_locate(argc, argv, &addr);
	if (ret)
		return ret;

	/* If no type is given, show a list of all files */
	if (!type) {
		puts(headline);
		fs_image_parse_image(ptype, (void *)addr, 0, 0, 0);

		return CMD_RET_SUCCESS;
	}

	/* If no description is given, use BOARD-CFG to find a default value */
	if (!descr) {
		descr = fs_image_get_default_descr(addr, type);
		if (!descr) {
			printf("Cannot find description for type %s\n", type);
			return CMD_RET_FAILURE;
		}
	}

	/* Search for the image */
	if (!fs_image_find_concat((void *)addr, type, descr, &idx_info))
	{
		printf("No entry of type %s (%s) found\n", type, descr);
		return CMD_RET_FAILURE;
	}

	/* Print result */
	if (idx_info.fsi != idx_info.fsh + 1)
		offs = (ulong)idx_info.fsi - addr;
	else
		offs = (ulong)idx_info.fsh - addr;

	puts(headline);
	puts(separator);
	fs_image_print_line(ptype, idx_info.fsh, idx_info.fsi, offs, 0);

	return CMD_RET_SUCCESS;
}

/* Load NBOOT and SPL regions from the boot device (NAND or MMC) to DRAM,
   create minimal NBoot image that could be saved again */
int fs_image_do_load(int argc, char * const argv[])
{
	struct fs_header_v1_0 *fsh;
	struct fs_image_params ip;
	bool load_uboot = false;
	const char *def_fname = "nboot.fs";
	int ret;

	early_support_index = 0;

	argv++;
	argc--;

	if (argc > 0) {
		size_t len = strlen(argv[0]);

		if (!strncmp(argv[0], "uboot", len)) {
			load_uboot = true;
			def_fname = "uboot.fs";
			argv++;
			argc--;
		} else if (!strncmp(argv[0], "nboot", len)) {
			/* Accept "nboot", too, but it is the default anyway */
			argv++;
			argc--;
		}
	}

	if (!fs_image_get_image_params(argc, argv, &ip, def_fname))
		return CMD_RET_USAGE;

	/* Invalidate any old image */
	fsh = (struct fs_header_v1_0 *)ip.addr;
	memset(fsh->info.magic, 0, 4);

#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	ret = fsimage_cntr_load(ip.addr, load_uboot, 1, &ip.size);
	if (ret)
		ret = fsimage_cntr_load(ip.addr, load_uboot, 2, &ip.size);
#else
	ret = fs_image_imx8m_load(ip.addr, load_uboot, &ip.size);
#endif
	if (ret)
		return ret;

#ifdef __UBOOT__
	set_fileaddr(ip.addr);
	env_set_fileinfo(ip.size);
#endif

	/* If a filename was given, write loaded image to the file */
	if (ip.fname) {
		printf("\nWriting final image to target file\n  ");
		if (!ip.size) {
			puts("Unknown image size; cannot write file\n");
			return CMD_RET_FAILURE;
		}

		if (!fs_image_store_file(&ip))
			return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

/* Save the F&S NBoot image to the boot device (NAND or MMC) */
int fs_image_do_save(int argc, char * const argv[])
{
	int boot_hwpart = -1;
	ulong addr;
	bool force = false;
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	bool system_atf = false;	/* If set, prefer ATF/TEE from NBoot */
#endif
	int ret;

	early_support_index = 0;

	argv++;
	argc--;

	while ((argc > 0) && (argv[0][0] == '-')) {
		if (!strcmp(argv[0], "-e")) {
			if (argc < 2) {
				puts("Missing argument for option -e\n");
				return CMD_RET_USAGE;
			}
			early_support_index = simple_strtoul(argv[1], NULL, 0);
			argv += 2;
			argc -= 2;
		} else if (!strcmp(argv[0], "-b")) {
			if (argc < 2) {
				puts("Missing argument for option -b\n");
				return CMD_RET_USAGE;
			}
			boot_hwpart = simple_strtol(argv[1], NULL, 0);
			if ((boot_hwpart < 0) || (boot_hwpart > 2)) {
				printf("Invalid argument %s for option -b\n",
				       argv[1]);
				return CMD_RET_USAGE;
			}
			argv += 2;
			argc -= 2;
		} else if (!strcmp(argv[0], "-f")) {
			force = true;
			argv++;
			argc--;
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
		} else if (!strcmp(argv[0], "-s")) {
			system_atf = true;
			argv++;
			argc--;
#endif
		} else
			return CMD_RET_USAGE;
	}

	ret = fs_image_locate(argc, argv, &addr);
	if (ret)
		return ret;

	/* Check if loaded image is valid, only valid images may be saved */
	if (fs_image_validate_full(addr) < 0)
		return CMD_RET_FAILURE;

#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	ret = fsimage_cntr_save(addr, boot_hwpart, force);
#else
	ret = fs_image_imx8m_save(addr, boot_hwpart, force, system_atf);
#endif

	return ret;
}

#ifdef __UBOOT__
/* Burn the fuses according to the NBoot in DRAM */
int fs_image_do_fuse(int argc, char * const argv[])
{
	struct index_info cfg_info = {0};
	const void *fdt;
	int offs;
	int rev_offs;
	int ret;
	enum boot_device boot_dev;
	const char *boot_dev_name;
	uint cur_val, fuse_val, fuse_mask, fuse_bw;
	const fdt32_t *fvals, *fmasks, *fbws;
	int i, len, len2, len3;
	ulong addr;
	bool force = false;

	argv++;				/* Skip command keyword */
	argc--;
	if ((argc > 0) && (argv[0][0] == '-')) {
		if (strcmp(argv[0], "-f"))
			return CMD_RET_USAGE;
		force = true;
		argv++;
		argc--;
	}

	if ((argc == 1) && !strncmp(argv[0], "stored", strlen(argv[0]))) {
		cfg_info.fsh = fs_image_get_cfg_addr();
	} else {
		ret = fs_image_locate_nboot(argc, argv, &addr);
		if (ret)
			return ret;

		ret = fs_image_find_board_cfg(addr, force, "fuse", &cfg_info,
					      NULL);
		if (ret <= 0)
			return CMD_RET_FAILURE;
	}

	fdt = fs_image_find_cfg_fdt_idx(&cfg_info);
	if (fs_image_get_boot_dev(fdt, &boot_dev, &boot_dev_name)
	    || fs_image_check_boot_dev_fuses(boot_dev, "fuse") < 0)
		return CMD_RET_FAILURE;

	/* No contradictions, do an in-depth check */
	offs = fs_image_get_board_cfg_offs(fdt);
	if (offs < 0) {
		puts("Cannot find BOARD-CFG\n");
		return CMD_RET_FAILURE;
	}

	rev_offs = fs_image_get_board_rev_subnode(fdt, offs);

	fbws = fs_image_getprop(fdt, offs, rev_offs, "fuse-bankword", &len);
	fmasks = fs_image_getprop(fdt, offs, rev_offs, "fuse-mask", &len2);
	fvals = fs_image_getprop(fdt, offs, rev_offs, "fuse-value", &len3);
	if (!fbws || !fmasks || !fvals || (len != len2) || (len2 != len3)
	    || !len || (len % sizeof(fdt32_t) != 0)) {
		printf("Invalid or missing fuse value settings for boot"
		       " device %s\n", boot_dev_name);
		return CMD_RET_FAILURE;
	}
	len /= sizeof(fdt32_t);

	puts("\n"
	     "Fuse settings (with respect to booting):\n"
	     "\n"
	     "  Bank Word Value      -> Target\n"
	     "  ----------------------------------------------\n");
	ret = 0;
	for (i = 0; i < len; i++) {
		fuse_bw = fdt32_to_cpu(fbws[i]);
		fuse_mask = fdt32_to_cpu(fmasks[i]);
		fuse_val = fdt32_to_cpu(fvals[i]);
		fuse_read(fuse_bw >> 16, fuse_bw & 0xffff, &cur_val);
		cur_val &= fuse_mask;

		printf("  0x%02x 0x%02x 0x%08x -> 0x%08x", fuse_bw >> 16,
		       fuse_bw & 0xffff, cur_val, fuse_val);
		if (cur_val == fuse_val)
			puts(" (unchanged)");
		else if (cur_val & ~fuse_val) {
			ret |= 1;
			puts(" (impossible)");
		} else
			ret |= 2;	/* Need change */
		puts("\n");
	}
	puts("\n");
	if (!ret) {
		printf("Fuses already set correctly to boot from %s\n",
		       boot_dev_name);
		return CMD_RET_SUCCESS;
	}
	if (ret & 1) {
		printf("Error: New settings for boot device %s would need to"
		       " clear fuse bits which\nis impossible. Refusing to"
		       " save this configuration.\n", boot_dev_name);
		return CMD_RET_FAILURE;
	}
	if (!force) {
		puts("The fuses will be changed to the above settings. This is"
		     " a write once option\nand cannot be undone. ");
		if (!fs_image_confirm())
			return CMD_RET_FAILURE;
	}

	/* Now there is no way back... actually burn the fuses */
	for (i = 0; i < len; i++) {
		fuse_bw = fdt32_to_cpu(fbws[i]);
		fuse_val = fdt32_to_cpu(fvals[i]);
		fuse_mask = fdt32_to_cpu(fmasks[i]);
		fuse_read(fuse_bw >> 16, fuse_bw & 0xffff, &cur_val);
		cur_val &= fuse_mask;
		if (cur_val == fuse_val)
			continue;	/* Skip unchanged values */
		ret = fuse_prog(fuse_bw >> 16, fuse_bw & 0xffff, fuse_val);
		if (ret) {
			printf("Error: Fuse programming failed for bank 0x%x,"
			       " word 0x%x, value 0x%08x (%d)\n",
			       fuse_bw >> 16, fuse_bw & 0xffff, fuse_val, ret);
			return CMD_RET_FAILURE;
		}
	}

	printf("Fuses programmed for boot device %s\n", boot_dev_name);

	return CMD_RET_SUCCESS;
}
#endif /* __UBOOT__ */
