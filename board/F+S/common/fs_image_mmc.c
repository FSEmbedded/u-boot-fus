// SPDX-License-Identifier:	GPL-2.0+
/*
 * (C) Copyright 2026 F&S Elektronik Systeme GmbH
 * Hartmut Keller <keller@fs-net.de>
 *
 * Handle storage of F&S nboot.fs and uboot.fs images in eMMC and SD-Card.
 */

#ifdef __UBOOT__
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <dm/device.h>
#include <linux/err.h>
#include "fs_board_common.h"		/* fs_board_*() */
#endif

#include "fs_image_common.h"		/* fs_image_*() */


/* ------------- MMC handling ---------------------------------------------- */

#ifdef CONFIG_IMX8MM
/* Info table for secondary SPL (MMC) */
struct info_table {
	u32 chip_num;			/* unused, set to 0 */
	u32 drive_type;			/* unused, set to 0 */
	u32 tag;			/* 0x00112233 */
	u32 first_sector;		/* Start sector of secondary SPL */
	u32 sector_count;		/* unused, set to 0 */
};
#endif

/* Check if any prerequisites for storing U-Boot are not met */
static bool fs_image_check_for_uboot_mmc(struct storage_info *si, bool force)
{
	/* Nothing to be done in case of MMC */
	return false;
}

/* Check if any prerequisites for storing NBoot are not met */
static bool fs_image_check_for_nboot_mmc(struct flash_info *fi,
					 struct storage_info *si, bool force)
{
#ifdef __UBOOT__
#ifndef CONFIG_IMX8MM
	u32 offset_fuses = fs_board_get_secondary_offset();
	u32 offset_nboot = si->start[1];

	/*
	 * If booting from User space of eMMC, check the fused secondary image
	 * matches the required offset
	 */
	if (!fi->boot_hwpart && (offset_fuses != offset_nboot)) {
		printf("Secondary Image Offset in fuses is 0x%08x but new NBOOT"
		       " wants 0x%08x.\nSecond copy (backup) will not boot"
		       " in case of errors!\n", offset_fuses, offset_nboot);
		if (!force && !fs_image_confirm())
			return true;
	}
#endif
#else
	/* ### TODO: Can we read fuses in Linux? */
#endif /* __UBOOT__ */

	return false;
}

/* Parse nboot-info for MMC settings and fill struct */
static int fs_image_get_nboot_info_mmc(struct flash_info *fi, const void *fdt,
				       int offs, struct nboot_info *ni,
				       int boot_hwpart, bool show, uint index)
{
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

	if (ni->flags & NI_SUPPORT_U_ATF) {
		err = fs_image_get_si(fdt, layout, align, "ATF", &ni->atf);
		if (err)
			return err;
		ni->atf.hwpart[0] = first;
		ni->atf.hwpart[1] = second;
	}

	/* Get U-Boot storage info */
	err = fs_image_get_si(fdt, layout, align, "U-BOOT", &ni->uboot);
	if (err)
		return err;
	if (ni->flags & NI_UBOOT_EMMC_BOOTPART) {
		ni->uboot.hwpart[0] = first;
		ni->uboot.hwpart[1] = second;
		/* Limit U-Boot size to boot part size */
		if (ni->uboot.size > fi->boot_part_size)
			ni->uboot.size = fi->boot_part_size;
	} else {
		ni->uboot.hwpart[0] = 0;
		ni->uboot.hwpart[1] = 0;
	}

#ifndef __UBOOT__
#if CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	/* Fill in values found when loading the BOARD-CFG from flash */
	if (!ni->nboot.start[0]) {
		ni->nboot.start[0] = nboot_info_fixup.nboot_start;
		ni->nboot.start[1] = nboot_info_fixup.nboot_start;
	}
	if (!ni->nboot.size)
		ni->nboot.size = nboot_info_fixup.nboot_size;
	if (!ni->uboot.start[0]) {
		ni->uboot.start[0] = nboot_info_fixup.uboot_start;
		ni->uboot.start[1] = nboot_info_fixup.uboot_start;
	}
	if (!ni->uboot.size)
		ni->uboot.size = nboot_info_fixup.uboot_size;
#endif
#endif

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
		err = fs_image_get_known_env_mmc(index,
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

	printf("nboot-info (%s layout): Booting from %s hwpart %d\n",
	       layout_name, fi->devname, first);
	if (ni->board_cfg_size)
		printf("- board-cfg-size=0x%08x\n", ni->board_cfg_size);
	printf("  spl:   start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->spl.hwpart[0], ni->spl.start[0],
	       ni->spl.hwpart[1], ni->spl.start[1], ni->spl.size);
	printf("  nboot: start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->nboot.hwpart[0], ni->nboot.start[0],
	       ni->nboot.hwpart[1], ni->nboot.start[1], ni->nboot.size);
	if (ni->flags & NI_SUPPORT_U_ATF) {
		printf("  atf:   start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
		       ni->atf.hwpart[0], ni->atf.start[0],
		       ni->atf.hwpart[1], ni->atf.start[1], ni->atf.size);
	}
	printf("  uboot: start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->uboot.hwpart[0], ni->uboot.start[0],
	       ni->uboot.hwpart[1], ni->uboot.start[1], ni->uboot.size);
	printf("  env:   start=%d:0x%08x/%d:0x%08x size=0x%08x\n",
	       ni->env.hwpart[0], ni->env.start[0],
	       ni->env.hwpart[1], ni->env.start[1], ni->env.size);

	return 0;
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

/* Load the image of given type/descr from eMMC at given offset */
static int fs_image_load_image_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si,
				   struct sub_info *sub)
{
	uint size = sub->size;
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

	err = fi->ops->set_hwpart(fi, copy, si);
	if (err)
		return err;

	if (sub->flags & SUB_IS_ENV) {
		size = si->size;
	} else if (sub->flags & SUB_IS_CNTR) {
		size = size;
	} else {
		err = fs_image_get_size_from_header(fi, offs, lim, sub, &size);
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
	if (err < 0)
		return err;

	sub->size = size;

	return 0;
}

/* Temporarily load Secondary Image Table */
static int fs_image_load_extra_mmc(struct flash_info *fi,
				   struct storage_info *spl, void *tempaddr)
{
#ifdef CONFIG_IMX8MM
	ulong blksz = fi->temp_size;
	uint offs;
	uint lim;
	int err;
	struct info_table *secondary;

	/* Do nothing if not running from User area of eMMC */
	if (spl->hwpart[1] != 0)
		return 0;

	/* Read Secondary Image Table, which is one block before primary SPL */
	lim = spl->start[0];
	offs = lim - blksz;

	printf("Loading SECONDARY-SPL-INFO from NAND\n"
	       "  Loading only copy from offset 0x%08x size 0x%lx...",
	       offs, blksz);

	err = fi->ops->read(fi, offs, blksz, lim, 0, fi->temp);
	secondary = (struct info_table *)(fi->temp);
	if (!err && (secondary->tag != 0x00112233))
		err = -ENOENT;

	fs_image_show_sub_status(err);
	if (err)
		return 0;		/* Don't fail, rely on nboot-info */

	/*
	 * The first_sector entry is counted relative to 0x8000 (0x40 blocks)
	 * and also the two skipped blocks for MBR and Secondary Image Table
	 * must be included, even if empty in the secondary case, which
	 * results in 0x42 blocks that have to be added.
	 */
	offs = (secondary->first_sector + 0x42) * blksz;
	if (offs != spl->start[1]) {
		printf("  Warning! SPL copy 1 is on offset 0x%08x, should be"
		       " on 0x%08x\n", offs, spl->start[1]);
		spl->start[1] = offs;
	}
	memset(fi->temp, fi->temp_fill, fi->temp_size);
#endif

	return 0;
}

/* Invalidate an image by overwriting the first block with zeroes */
static int fs_image_invalidate_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si)
{
	uint offs = si->start[copy];
	uint size = si->size;
	uint lim = offs + size;
	uint chunk_mask = fi->temp_size - 1;
	int err;

	/*
	 * In container versions, offs may not be on an MMC block boundary
	 * (e.g. when writing U-Boot). However then offs points to an F&S
	 * header (fsh) where the main image (fsi) is 1 KiB aligned and the
	 * previous image ends on the 1 KiB boundary before. Which means there
	 * is an empty slot of 1 KiB that just contains the F&S header at the
	 * end. This is why the whole MMC block containing the F&S header can
	 * be cleared here when invalidating the region and can later be
	 * written back when re-activating the region without ever touching
	 * the image before.
	 *
	 * Pleas note that this is not true for the BOARD-INFO. Here the
	 * BOARD-ID preceeds the BOARD-INFO F&S header. This means a
	 * BOARD-INFO must not be written alone, just as part of a longer
	 * sub sequence of a region.
	 */
	size += offs & chunk_mask;
	offs &= ~chunk_mask;

	printf("  Invalidating %s at offset 0x%08x size 0x%x...",
	       si->type, offs, size);
	debug("\n");

	fs_image_drop_temp(fi);
	err = fi->ops->write(fi, offs, fi->temp_size, lim, 0, fi->temp);

	if (!err)
		err = fi->ops->sync(fi);

	fs_image_show_sub_status(err);

	return err;
}

/* Switch to partition where reion is located and show region info */
static int fs_image_prepare_region_mmc(struct flash_info *fi, int copy,
				       struct storage_info *si)
{
	int err;

	err = fi->ops->set_hwpart(fi, copy, si);
	if (err)
		return err;

	printf("  -- %s (hwpart %d) --\n", si->type, si->hwpart[copy]);

	return 0;
}

#ifdef CONFIG_IMX8MM
/* Write Secondary Image table for redundant SPL */
static int fs_image_write_secondary_table(struct flash_info *fi, int copy,
					  struct storage_info *si)
{
	ulong blksz = fi->temp_size;
	uint offs;
	uint lim;
	int err;
	struct info_table *secondary;

	/* Do nothing if not second copy */
	if (copy != 1)
		return 0;

	/*
	 * Write Secondary Image Table for redundant SPL; the first_sector
	 * entry is counted relative to 0x8000 (0x40 blocks) and also the two
	 * skipped blocks for MBR and Secondary Image Table must be included,
	 * even if empty in the secondary case, which results in subtracting
	 * 0x42 blocks.
	 *
	 * The table itself is located one block before primary SPL.
	 */
	secondary = (struct info_table *)(fi->temp);
	secondary->tag = 0x00112233;
	secondary->first_sector = si->start[1] / blksz - 0x42;

	lim = si->start[0];
	offs = lim - blksz;

	printf("  Writing SECONDARY-SPL-INFO at offset 0x%08x size 0x200...",
	       offs);
	debug("\n");

	err = fi->ops->write(fi, offs, blksz, lim, 0, fi->temp);
	memset(fi->temp, fi->temp_fill, fi->temp_size);

	fs_image_show_sub_status(err);

	return err;
}

#endif

/* Save NBOOT and SPL region to MMC */
static int fs_image_save_nboot_mmc(struct flash_info *fi,
				   struct region_info *nboot_ri,
				   struct region_info *atf_ri,
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
	start_copy = fs_image_get_start_copy(false, true);
	copy = start_copy;
	do {
		printf("\nSaving copy %d to %s:\n", copy, fi->devname);
		if (fs_image_save_region(fi, copy, nboot_ri))
			failed |= BIT(copy);

		if (atf_ri && fs_image_save_region(fi, copy, atf_ri))
			failed |= BIT(copy);

#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
		fs_image_set_spl_secondary_bit(spl_ri->sub->img, copy);
#endif

		if (fs_image_save_region(fi, copy, spl_ri))
			failed |= BIT(copy);

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
		copy = 1 - copy;
	} while (copy != start_copy);

	return failed;
}


/* ------------- MMC low-level access in U-Boot backend --------------------- */

#ifdef __UBOOT__

static struct udevice *bdev;	       /* blkdev driver instance */
static struct mmc *mmc;		       /* mmc instance */
static u8 old_hwpart;		       /* Previous partition before command */

/* Switch to a new hardware partition */
static int fs_image_set_hwpart_mmc(struct flash_info *fi, int copy,
				   const struct storage_info *si)
{
	int err;
	uint hwpart = si->hwpart[copy];

	err = blk_select_hwpart(bdev, hwpart);
	if (err)
		printf("  Cannot switch to hwpart %d on %s for %s (%d)\n",
		       hwpart, fi->devname, si->type, err);

	return err;
}

/* Set the hardware partition to boot from in the future */
static int fs_image_set_boot_hwpart_mmc(struct flash_info *fi, int boot_hwpart)
{
	u8 ack;
	u8 access;
	int err;

	if ((boot_hwpart < 0) || (boot_hwpart == fi->boot_hwpart))
		return 0;

	printf("\nSwitching %s to boot hwpart %d...", fi->devname, boot_hwpart);

	if (!boot_hwpart)
		boot_hwpart = 7;

	ack = EXT_CSD_EXTRACT_BOOT_ACK(mmc->part_config);
	access = EXT_CSD_EXTRACT_PARTITION_ACCESS(mmc->part_config);
 	err = mmc_set_part_conf(mmc, ack, boot_hwpart, access);

	if (!err)
		fi->boot_hwpart = boot_hwpart;

	return err;
}

/* Read image at offset with given size */
static int fs_image_read_mmc(struct flash_info *fi, uint offs, uint size,
			     uint lim, uint flags, void *buf)
{
	ulong count;
	ulong blksz = fi->temp_size;
	lbaint_t blk = offs / blksz;
	lbaint_t blk_count = (size + blksz - 1) / blksz;

	debug("  -> mmc_read from offs 0x%x (block 0x" LBAF ") size 0x%x\n",
	      offs, blk, size);

	count = blk_read(bdev, blk, blk_count, buf);
	if (count < blk_count)
		return -EIO;
	else if (IS_ERR_VALUE(count))
		return (int)count;

	return 0;
}

/* Sync data with flash */
static int fs_image_sync_mmc(struct flash_info *fi)
{
	return 0;			/* U-Boot always writes synchronously */
}

/* Save some data (only full blocks) to eMMC */
static int fs_image_write_mmc(struct flash_info *fi, uint offs, uint size,
			      uint lim, uint flags, void *buf)
{
	ulong count;
	ulong blksz = fi->temp_size;
	lbaint_t blk = offs / blksz;
	lbaint_t blk_count = (size + blksz - 1) / blksz;;

	/* Bad block handling is done by eMMC controller */
	debug("  -> mmc_write to offs 0x%x (block 0x" LBAF ") size 0x%x\n",
	      offs, blk, size);

	count = blk_write(bdev, blk, blk_count, buf);
	if (count < blk_count)
		return -EIO;
	else if (IS_ERR_VALUE(count))
		return (int)count;

	return 0;
}

static int fs_image_read_board_cfg_mmc(struct flash_info *fi,
				       const struct storage_info *si,
				       void *board_cfg)
{
	return -EINVAL;
}

static void fs_image_put_flash_mmc(struct flash_info *fi)
{
	if (blk_select_hwpart(bdev, old_hwpart))
		printf("Cannot switch back to original hwpart %d\n", old_hwpart);
}
#endif /* __UBOOT__ */

static struct flash_ops flash_ops_mmc = {
	/* Generic access functions */
	.check_for_uboot = fs_image_check_for_uboot_mmc,
	.check_for_nboot = fs_image_check_for_nboot_mmc,
	.get_nboot_info = fs_image_get_nboot_info_mmc,
	.si_differs = fs_image_si_differs_mmc,
	.load_image = fs_image_load_image_mmc,
	.load_extra = fs_image_load_extra_mmc,
	.invalidate = fs_image_invalidate_mmc,
	.prepare_region = fs_image_prepare_region_mmc,
	.save_nboot = fs_image_save_nboot_mmc,

	/* Backend specific low-level access functions */
	.set_hwpart = fs_image_set_hwpart_mmc,
	.set_boot_hwpart = fs_image_set_boot_hwpart_mmc,
	.read = fs_image_read_mmc,
	.write = fs_image_write_mmc,
	.sync = fs_image_sync_mmc,
	.read_board_cfg = fs_image_read_board_cfg_mmc,
	.put_flash = fs_image_put_flash_mmc,
};

/* ------------- Global access functions ----------------------------------- */

#ifdef __UBOOT__
int fs_image_get_flash_mmc(struct flash_info *fi, int devnum, bool rw)
{
	struct udevice *mmc_dev;
	struct blk_desc *bdesc;
	struct mmc_uclass_priv *upriv;
	int err;

	err = blk_get_device(UCLASS_MMC, devnum, &bdev);
	if (err) {
		printf("blkdev %d not found\n", fi->boot_dev);
		return -ENODEV;
	}
	fi->ops = &flash_ops_mmc;

	mmc_dev = dev_get_parent(bdev);
	bdesc = dev_get_uclass_plat(bdev);
	upriv = dev_get_uclass_priv(mmc_dev);
	mmc = upriv->mmc;

	/* Determine hwpart (when command starts) and boot hwpart */
	old_hwpart = bdesc->hwpart;
	fi->boot_hwpart = EXT_CSD_EXTRACT_BOOT_PART(mmc->part_config);
	if (fi->boot_hwpart > 2)
		fi->boot_hwpart = 0;

	/* Temporary buffer is for one block */
	fi->temp_size = bdesc->blksz;

	/* Size of a boot partition */
	fi->boot_part_size = (u32)(mmc->capacity_boot);

	/* Set device name */
	sprintf(fi->devname, "mmc%d", devnum);

	return 0;
}
#endif /* __UBOOT__ */

/*
 * List of known environment positions before it was moved to nboot-info.
 *
 * Remarks:
 * - Old versions of PicoCoreMX8MN, where no redundand environment was used
 *   and the environment was on 0x400000, are not supported.
 * - efusMX8X still uses no redundand environment, updating from such a version
 *   is not implemented yet. (### TODO ###)
 */
#ifdef CONFIG_IMX8MM
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x100000, 0x104000},
		.size = 0x4000
	},
	{
		.start = {0x100000, 0x104000},
		.size = 0x2000
	},
};
#elif defined CONFIG_IMX8MN
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x138000, 0x13c000},
		.size = 0x4000
	},
};
#elif defined CONFIG_IMX8MP
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x138000, 0x13c000},
		.size = 0x4000
	},
	{
		.start = {0x100000, 0x104000},
		.size = 0x4000
	},
};
#elif defined CONFIG_IMX8X
static const struct env_info fs_image_known_env_mmc[] = {
	{
		.start = {0x200000, 0x200000},
		.size = 0x2000
	},
};
#else
static const struct env_info fs_image_known_env_mmc[0];
#endif

int fs_image_get_known_env_mmc(uint index, uint start[2], uint *size)
{
	const struct env_info *env_info;

	printf("Using env location fallback #%d for old NBoot\n", index);
	if (index > ARRAY_SIZE(fs_image_known_env_mmc)) {
		puts("No such fallback for env location\n");
		return -EINVAL;
	}

	env_info = &fs_image_known_env_mmc[index];

	start[0] = env_info->start[0];
	start[1] = env_info->start[1];
	if (size)
		*size = env_info->size;

	return 0;
}

void fs_image_get_generic_si_mmc(struct flash_info *fi, struct storage_info *si)
{
	si->type = "NBOOT";
	if (fi->boot_hwpart) {
		si->start[0] = 0x00000000;
		si->start[1] = 0x00000000;
		si->size = 0x003f8000;	/* Leave room for env at end */
		si->hwpart[0] = fi->boot_hwpart;
		si->hwpart[1] = 3 - fi->boot_hwpart;
	} else {
		si->start[0] = 0x00008000; /* skip GPT in first 32KiB */
		si->start[1] = 0x00400000;
		si->size = 0x003f8000;	/* env is at end of 2nd copy */
		si->hwpart[0] = 0;
		si->hwpart[1] = 0;
	}
}
