// SPDX-License-Identifier:	GPL-2.0+
/*
 * (C) Copyright 2026 F&S Elektronik Systeme GmbH
 * Hartmut Keller <keller@fs-net.de>
 *
 * Handle storage of F&S nboot.fs and uboot.fs images in NAND flash.
 */

#ifdef __UBOOT__
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <linux/err.h>
#if CONFIG_IS_ENABLED(MTD_RAW_NAND)
#include <nand.h>
#include <mxs_nand.h>			/* mxs_nand_mode_fcb_62bit(), ... */
#include <asm/mach-imx/imx-nandbcb.h>
#include <jffs2/jffs2.h>		/* struct mtd_device + part_info */
#endif
#include "fs_board_common.h"		/* fs_board_*() */
//#include "fs_bootrom.h"

#else /* !__UBOOT__ */

//####TODO
#include <linux/kconfig.h>		/* Get kconfig macros only */
#include <linux/libfdt.h>
#include <errno.h>
#include <stdio.h>
#include "linux_helpers.h"		/* fit_get_size(), confirm_yesno() ... */

/* ### the following signatures are most probably not required in linux */
//###int mtdparts_init(void);
//###int mtd_id_parse(const char *id, const char **ret_id, u8 *dev_type, u8 *dev_num);
//###int find_dev_and_part(const char *id, struct mtd_device **dev,
//###                      u8 *part_num, struct part_info **part);

#endif

#include "fs_image_common.h"		/* fs_image_*() */


/* ------------- NAND handling --------------------------------------------- */

#ifdef __UBOOT__

#ifdef CONFIG_FS_UPDATE_SUPPORT
	const char *uboot_mtd_names[] = {"UBoot_A", "UBoot_B"};
#elif defined CONFIG_SYS_NAND_U_BOOT_OFFS_REDUND
	const char *uboot_mtd_names[] = {"UBoot", "UBootRed"};
#else
	const char *uboot_mtd_names[] = {"UBoot"};
#endif

/* Check if any prerequisites for saving U-Boot are not met */
static bool fs_image_check_for_uboot_nand(struct storage_info *si, bool force)
{
	struct mtd_device *dev;
	u8 part_num;
	struct part_info *part;
	const char *name;
	int i;
	u64 start;
	bool warning = false;

	/* Issue warning if U-Boot MTD partitions do not match nboot-info */
	mtdparts_init();

	for (i = 0; i < ARRAY_SIZE(uboot_mtd_names); i++) {
		name = uboot_mtd_names[i];
		if (find_dev_and_part(name, &dev, &part_num, &part)) {
			printf("WARNING: MTD %s not found\n", name);
			warning = true;
		} else {
			start = si->start[i];
			if (part->offset != start) {
				printf("WARNING: MTD %s starts on 0x%llx but"
				       " should start on 0x%llx.\n",
				       name, part->offset, start);
				warning = true;
			}
			if (part->size != (u64)(si->size)) {
				printf("WARNING: MTD %s has size 0x%llx but"
				       " should have size 0x%llx.\n",
				       name, part->offset, start);
				warning = true;
			}
		}
	}

	return warning && !force && !fs_image_confirm();
}

/* Check if any prerequisites for storing NBoot are not met */
static bool fs_image_check_for_nboot_nand(struct flash_info *fi,
					  struct storage_info *si, bool force)
{
	/* Nothing to be done in case of NAND */

	return false;
}

static int fs_image_set_hwpart_nand(struct flash_info *fi, int copy,
				    const struct storage_info *si)
{
	return 0;
}

/* Parse nboot-info for NAND settings and fill struct */
static int fs_image_get_nboot_info_nand(struct flash_info *fi, void *fdt,
					int offs, struct nboot_info *ni,
					int boot_hwpart, bool show, uint index)
{
	int layout;
	const char *layout_name;
	int err;
	uint align = fi->mtd->erasesize;

	/* Go to layout node if present */
	layout_name = "nand";
	layout = fdt_subnode_offset(fdt, offs, layout_name);
	if (layout < 0) {
		layout_name = "old";
		layout = offs;
	}

	err = fs_image_get_si(fdt, layout, align, "SPL", &ni->spl);
	if (err)
		return err;

	err = fs_image_get_si(fdt, layout, align, "NBOOT", &ni->nboot);
	if (err)
		return err;

	if (ni->flags & NI_SUPPORT_U_ATF) {
		err = fs_image_get_si(fdt, layout, align, "ATF", &ni->atf);
		if (err)
			return err;
	}

	err = fs_image_get_si(fdt, layout, align, "U-BOOT", &ni->uboot);
	if (err)
		return err;

	/*
	 * The size of the environment region is actually given by env-range
	 * entry, from which env-size bytes are then used.
	 */
	ni->env.type = "ENV";
	err = fs_image_get_fdt_val(fdt, layout, "env-start", align,
				   2, ni->env.start);
	if (!err) {
		err = fs_image_get_fdt_val(fdt, layout, "env-range", align,
					   1, &ni->env.size);
		if (err)
			return err;
		/* The size only has to be aligned to pages */
		align = fi->mtd->writesize;
		err = fs_image_get_fdt_val(fdt, layout, "env-size", align,
					   1, &fi->env_used);
	} else if (err == -ENOENT) {
		/*
		 * No env data found in nboot-info, fall back to some known
		 * values. Use option -e to select index.
		 */
		err = fs_image_get_known_env_nand(index,
						  ni->env.start, &fi->env_used);
		ni->env.size = CONFIG_ENV_NAND_RANGE;;
	}
	if (err)
		return err;

#ifndef DEBUG
	if (!show)
		return 0;
#endif

	printf("nboot-info@0x%lx (%s layout): Booting from %s\n",
	       (ulong)fdt, layout_name, fi->devname);
	if (ni->board_cfg_size)
		printf("- board-cfg-size=0x%08x\n", ni->board_cfg_size);
	printf("- spl:   start=0x%08x/0x%08x size=0x%08x\n",
	       ni->spl.start[0], ni->spl.start[1], ni->spl.size);
	printf("- nboot: start=0x%08x/0x%08x size=0x%08x\n",
	       ni->nboot.start[0], ni->nboot.start[1], ni->nboot.size);
	if (ni->flags & NI_SUPPORT_U_ATF) {
		printf("- atf:   start=0x%08x/0x%08x size=0x%08x\n",
		       ni->atf.start[0], ni->atf.start[1], ni->atf.size);
	}
	printf("- uboot: start=0x%08x/0x%08x size=0x%08x\n",
	       ni->uboot.start[0], ni->uboot.start[1], ni->uboot.size);
	printf("- env:   start=0x%08x/0x%08x size=0x%08x env_used=0x%08x\n",
	       ni->env.start[0], ni->env.start[1], ni->env.size, fi->env_used);

	return 0;
}

/* Check if start address or size differs */
static bool fs_image_si_differs_nand(const struct storage_info *si1,
				     const struct storage_info *si2)
{
	return ((si1->size != si2->size)
		|| (si1->start[0] != si2->start[0])
		|| (si1->start[1] != si2->start[1]));
}

/* Compute checksum for FCB or DBBT block */
static u32 fs_image_bcb_checksum(void *data, size_t size)
{
	u32 checksum = 0;
	u8 *p = data;

	while (size--)
		checksum += *p++;

	return ~checksum;
}

/* Check if checksum for FCB or DBBT block is correct */
static int fs_image_check_bcb_checksum(void *data, size_t size,
					u32 expected, bool accept_zero)
{
	u32 checksum;

	debug("  -");
	if (accept_zero && (expected == 0)) {
		debug(" Checksum 0 OK\n");
		return 0;
	}

	checksum = fs_image_bcb_checksum(data, size);
	if (checksum == expected) {
		debug(" Checksum OK\n");
		return 0;
	}

	puts(" BAD CHECKSUM");
	debug(" (got 0x%08x, expected 0x%08x)\n", checksum, expected);

	return -EILSEQ;
}

/* Load some data from offset with given size */
static int fs_image_read_nand(struct flash_info *fi, uint offs, uint size,
			      uint lim, uint flags, u8 *buf)
{
	int err;
	size_t rsize;
	size_t actual;
	loff_t roffs;
	loff_t maxsize;
	size_t bb_extra;

	/* FCB, DBBT and DBBT_DATA sub-images ignore any bad block offsets */
	if (flags & (SUB_IS_FCB | SUB_IS_DBBT | SUB_IS_DBBT_DATA)) {
		uint block_offs = offs & ~(fi->mtd->erasesize - 1);

		if (nand_block_isbad(fi->mtd, block_offs)) {
			puts(" BAD BLOCK!");
			return -EBADMSG;;
		}
		if (flags & SUB_IS_FCB) {
			debug("  - Switch to 62bit ECC\n");
			mxs_nand_mode_fcb_62bit(fi->mtd);
		}
	}

	rsize = size;
	roffs = offs + fi->bb_extra_offs;
	maxsize = lim - roffs;

	debug("  -> nand_read from offs 0x%llx size 0x%x maxsize 0x%llx\n",
	     roffs, size, maxsize);
	err = nand_read_skip_bad(fi->mtd, roffs, &rsize, &actual,
				  maxsize, buf);

	if (flags & SUB_IS_FCB) {
		debug("  - Switch back to normal ECC\n");
		mxs_nand_mode_normal(fi->mtd);
	}

	bb_extra = actual - rsize;
	if (bb_extra) {
		debug("  - Adding 0x%lx to bad block offset\n", bb_extra);
		fi->bb_extra_offs += bb_extra;
	}

	return err;
}

/* Load the image of given type/descr from NAND flash at given offset */
static int fs_image_load_image_nand(struct flash_info *fi, int copy,
				    const struct storage_info *si,
				    struct sub_info *sub)
{
	int err;
	uint size;
	uint offs = si->start[copy] + sub->offset;
	uint lim = si->start[copy] + si->size;
	size_t cs_size;

	sub->size = 0;

	printf("  Loading copy %d from offset 0x%08x", copy, offs);
	debug("\n");

#ifdef CONFIG_IMX8MM
	/* On i.MX8MM, SPL starts at offset 0x400 in the middle of the page */
	if (sub->flags & SUB_IS_SPL)
		offs += 0x400;
#endif

	/* Clear the temp buffer (read cache) */
	fs_image_drop_temp(fi);

	/* Determine sub-image size */
	if (sub->flags & SUB_IS_FCB) {
		size = sizeof(struct fcb_block);
	} else if ((sub->flags & SUB_IS_DBBT)
		   || (sub->flags & SUB_IS_DBBT_DATA)) {
		size = fi->mtd->writesize;
#ifdef CONFIG_NAND_MXS
	} else if (sub-> flags & SUB_IS_ENV) {
		size = fi->env_used;
#endif
	} else {
		err = fs_image_get_size_from_header(fi, offs, lim, sub, &size);
		if (err)
			return err;
	}

	printf(" size 0x%x...", size);
	debug("\n");

	/* Load image itself */
	err = fs_image_load_sub(fi, offs, size, lim, sub->flags, sub->img);
	if (err)
		return err;

	/* Check if image is corrupted */
	if (sub->flags & SUB_IS_FCB) {
		struct fcb_block *fcb = sub->img;

		if ((fcb->fingerprint != FCB_FINGERPRINT)
		    || (fcb->version != FCB_VERSION_1))
			return -ENOENT;

		cs_size = sizeof(struct fcb_block) - 4;
		err = fs_image_check_bcb_checksum(sub->img + 4, cs_size,
						  fcb->checksum, false);
	} else if (sub->flags & SUB_IS_DBBT) {
		struct dbbt_block *dbbt = sub->img;

		if ((dbbt->fingerprint != DBBT_FINGERPRINT)
		    || (dbbt->version != DBBT_VERSION_1))
			return -ENOENT;

		/* NXP's kobs writes checksum as 0, accept that */
		cs_size = sizeof(struct dbbt_block) - 4;
		err = fs_image_check_bcb_checksum(sub->img + 4, cs_size,
						  dbbt->checksum, true);
	} else if (sub->flags & SUB_IS_DBBT_DATA) {
		u32 *dbbt_data = sub->img;
		u32 count = dbbt_data[1];

		/* Detection is vague, but enough to not accept empty pages */
		if (count > 32)
			return -ENOENT;

		/* NXP's kobs writes checksum as 0, accept that */
		cs_size = (count + 1) * sizeof(u32);
		err = fs_image_check_bcb_checksum(sub->img + 4, cs_size,
						  dbbt_data[0], true);
	} else if (sub->flags & SUB_IS_ENV) {
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

/* Temporarily load Boot Control Block (BCB) with FCB and DBBT */
static int fs_image_load_extra_nand(struct flash_info *fi,
				    struct storage_info *spl, void *tempaddr)
{
	struct fcb_block *fcb;
	struct dbbt_block *dbbt;
	u32 *dbbt_data;
	struct sub_info sub;
	u32 start;
	struct storage_info bcb;
	uint pages_per_block;
	struct mtd_info *mtd = fi->mtd;
	int copy;
	int err;

	/* Load Firmware Configuration Block (FCB) */
	bcb.start[0] = 0;
	bcb.start[1] = mtd->erasesize;
	bcb.size = mtd->erasesize;

	sub.type = "FCB";
	sub.descr = fs_image_get_arch();
	sub.img = tempaddr;
	sub.offset = 0;
	sub.flags = SUB_IS_FCB;
	err = fs_image_load_image(fi, &bcb, &sub);
	if (err)
		return err;

	fcb = tempaddr + FSH_SIZE;
	for (copy = 0; copy < 2; copy++) {
		start = copy ? fcb->fw2_start : fcb->fw1_start;
		start *= mtd->writesize;
		if (start != spl->start[copy]) {
			printf("  Warning! SPL copy %d is on offset 0x%08x,"
			       " should be on 0x%08x\n",
			       copy, start, spl->start[copy]);
			spl->start[copy] = start;
		}
	}

	/* Load Discovered Bad Block Table (DBBT) */
	pages_per_block = mtd->erasesize / mtd->writesize;
	start = fcb->dbbt_start / pages_per_block * mtd->erasesize;
	bcb.start[0] = start;
	bcb.start[1] = start + mtd->erasesize;
	bcb.size = mtd->erasesize;

	/* Do not fail on DBBT/DBBT-DATA, output is for information only */
	tempaddr = sub.img;
	sub.type = "DBBT";
	sub.offset = fcb->dbbt_start % pages_per_block * mtd->writesize;
	sub.flags = SUB_IS_DBBT;
	err = fs_image_load_image(fi, &bcb, &sub);
	if (err) {
		printf("  Warning! No Discovered Bad Block Table (DBBT)!\n");
		return 0;		/* Don't fail, DBBT is not important */
	}

	dbbt = tempaddr + FSH_SIZE;
	if (!dbbt->dbbtpages) {
		printf("  No Bad Blocks in DBBT recorded\n");
		return 0;
	}

	tempaddr = sub.img;
	sub.type = "DBBT-DATA";
	sub.offset += 4 * mtd->writesize;
	sub.flags = SUB_IS_DBBT_DATA;
	err = fs_image_load_image(fi, &bcb, &sub);
	if (err) {
		printf("  Warning! No DBBT data found!\n");
		return 0;		/* Don't fail, DBBT is not important */
	}

	dbbt_data = tempaddr + FSH_SIZE;
	printf("  %d bad block(s) in DBBT recorded\n", dbbt_data[1]);

	return 0;
}

/* Erase given region */
static int fs_image_invalidate_nand(struct flash_info *fi, int copy,
			       const struct storage_info *si)
{
	loff_t offs = si->start[copy];
	size_t size = si->size;
	struct nand_erase_options opts = {0};
	int err;

	printf("  Erasing %s region at offset 0x%llx size 0x%zx...",
	       si->type, offs, size);
	debug("\n");

	opts.length = size;
	opts.lim = size;
	opts.quiet = 1;
	opts.offset = offs;

	/* This automatically marks blocks bad if they cannot be erased */
	err = nand_erase_opts(fi->mtd, &opts);

	fs_image_show_sub_status(err);

	return err;
}

/* Save some data (only full pages) to NAND; return 1 if new bad block */
static int fs_image_write_nand(struct flash_info *fi, uint offs, uint size,
			       uint lim, uint flags, u8 *buf)
{
	int err;
	size_t wsize;
	size_t actual;
	loff_t woffs;
	loff_t maxsize;
	size_t bb_extra;

	/* FCB, DBBT and DBBT_DATA sub-images ignore any bad block offsets */
	if (flags & (SUB_IS_FCB | SUB_IS_DBBT | SUB_IS_DBBT_DATA)) {
		uint block_offs = offs & ~(fi->mtd->erasesize - 1);

		if (nand_block_isbad(fi->mtd, block_offs)) {
			puts(" BAD BLOCK!");
			return -EBADMSG;;
		}
		if (flags & SUB_IS_FCB) {
			debug("  - Switch to 62bit ECC\n");
			mxs_nand_mode_fcb_62bit(fi->mtd);
			size = fi->mtd->writesize;
		}
	}

	wsize = size;
	woffs = offs + fi->bb_extra_offs;
	maxsize = lim - woffs;

	debug("  -> nand_write to offs 0x%llx size 0x%x maxsize 0x%llx\n",
	      woffs, size, maxsize);
	err = nand_write_skip_bad(fi->mtd, woffs, &wsize, &actual,
				  maxsize, buf, WITH_WR_VERIFY);
	if (flags & SUB_IS_FCB) {
		debug("  - Switch back to normal ECC\n");
		mxs_nand_mode_normal(fi->mtd);
	}
	if (err) {
		/*
		 * ### TODO:
		 * Handle new bad blocks and return 1. Actually written bytes
		 * are in wsize, i.e. this is kind of a pointer to the bad
		 * page, but nevertheless difficult to handle in case of bad
		 * blocks. Also difficult to say if the call failed because of
		 * a real write failure (where the block should be marked bad)
		 * or because of some other minor error. For example if wsize
		 * is 0 after return, this could be because the image does not
		 * fit because of bad blocks, or there was a real write error
		 * right in the first page. Maybe check for -EIO?
		 *
		 * Or should we implement our own writing loop that writes
		 * block by block? This seems like re-inventing the wheel.
		 */
		return err;
	}

	bb_extra = actual - wsize;
	if (bb_extra) {
		debug("  - Adding 0x%lx to bad block offset\n", bb_extra);
		fi->bb_extra_offs += bb_extra;
	}

	return 0;
}

/* Show region info */
static int fs_image_prepare_region_nand(struct flash_info *fi, int copy,
					struct storage_info *si)
{
	printf("  -- %s --\n", si->type);

	return 0;
}

/* Save NBOOT and SPL region to NAND */
#define DBBT_DATA_BLOCKS 32
#define DBBT_DATA_ENTRIES (DBBT_DATA_BLOCKS - 8)
static int fs_image_save_nboot_nand(struct flash_info *fi,
				    struct region_info *nboot_ri,
				    struct region_info *atf_ri,
				    struct region_info *spl_ri)
{
	int failed;
	int copy, start_copy;
	u32 i, bad_blocks;
	struct storage_info bcb_si;
	struct region_info bcb_ri;
	struct sub_info bcb_sub[3];
	struct fcb_block fcb;
	struct dbbt_block dbbt;
	u32 dbbt_data[DBBT_DATA_ENTRIES + 2];
	struct nand_chip *chip = mtd_to_nand(fi->mtd);
	struct mxs_nand_info *nand_info = nand_get_controller_data(chip);
	struct mxs_nand_layout mxs_layout;
	const char *arch = fs_image_get_arch();

	/*
	 * On NAND we have an additional region called BCB (Boot Control
	 * Block). This consists of:
	 *
	 * 1. FCB (Firmware Configuration Block)
	 * One FCB is stored in the first page of the first n blocks in NAND.
	 * It is a struct that tells the ROM Loader which NAND settings and
	 * ECC to use to access further images. It also says where the DBBT and
	 * the two Firmware copies (a.k.a. SPL) are located. The FCB itself is
	 * loaded by the ROM Loader with safe NAND timings and a very high
	 * ECC: 8 chunks with 128 Bytes with 62-Bit ECC each, which is 1024
	 * bytes main data in total, and additional 32 Bytes metadata in the
	 * first chunk. This allows for up to 496 bit errors within those 1056
	 * Bytes.
	 *
	 * 2. DBBT (Discovered Bad Block Table)
	 * The page number of the first DBBT is given by the FCB. The next
	 * DBBT is in the next block at the same page offset. NXPs kobs tool
	 * locates the DBBT copies in the n blocks after the FCB copies, but we
	 * are using the same blocks as for FCB, but in the 4th page. The DBBT
	 * is a struct that tells the ROM Loader how many DBBT-DATA pages
	 * there are. If there are no bad blocks in the boot area, no
	 * DBBT-DATA pages are required.
	 *
	 * 3. DBBT-DATA
	 * This entry starts 4 pages behind DBBT and is only necessary, if
	 * there are bad blocks in the boot area. It tells the ROM Loader how
	 * many of those blocks are bad, and then lists the numbers of those
	 * blocks. It helps the ROM Loader so that it does not need to read
	 * all the Bad Block Markers. We consider the NBoot MTD partition to
	 * be our boot area. It consists of 32 blocks, but at least 8 of them
	 * need to be intact to have at least one copy of BCB and SPL. So we
	 * only prepare an array for at most 24 bad block entries.
	 *
	 * The number n of FCB and DBBT copies is given by OTP fuses and can
	 * be read from BOOT_SEARCH_COUNT (0x470[6:5] on i.MX8MM, 0x1B0[7:8]
	 * on i.MX8X) or NAND_FCB_SEARCH_COUNT (0x4A0[14:13] on i.MX8MN/MP)
	 * respectively. n can be 2, 4 or 8. We do not change the fuses and
	 * therefore assume two copies like with all our other images. But our
	 * layout has room for up to four copies, so we can change this in the
	 * future if we find this useful. Please note that the ROM Loader does
	 * *not* skip bad blocks for FCB/DBBT, so if a block is bad there,
	 * this copy simply does not exist, which reduces the number of
	 * available copies.
	 */
	bcb_si.type = "BCB";
	bcb_si.start[0] = 0x0;
	bcb_si.start[1] = fi->mtd->erasesize;
	bcb_si.size = fi->mtd->erasesize;

	/* Fill FCB (Firmware Configuration Block) */
	memset(&fcb, 0, sizeof(struct fcb_block));
	mxs_nand_get_layout(fi->mtd, &mxs_layout);

	fcb.fingerprint = FCB_FINGERPRINT;
	fcb.version = FCB_VERSION_1;

	fcb.datasetup = 80;
	fcb.datahold = 60;
	fcb.addr_setup = 25;
	fcb.dsample_time = 6;

	fcb.pagesize = fi->mtd->writesize;
	fcb.oob_pagesize = fcb.pagesize + fi->mtd->oobsize;
	fcb.sectors = fi->mtd->erasesize / fcb.pagesize;

	fcb.meta_size = mxs_layout.meta_size;
	fcb.nr_blocks = mxs_layout.nblocks;
	fcb.ecc_nr = mxs_layout.data0_size;
	fcb.ecc_level = mxs_layout.ecc0;
	fcb.ecc_size = mxs_layout.datan_size;
	fcb.ecc_type = mxs_layout.eccn;
	fcb.bchtype = mxs_layout.gf_len;

	/* DBBT search area starts in first block at page 4 */
	fcb.dbbt_start = 4;

	fcb.bb_byte = nand_info->bch_geometry.block_mark_byte_offset;
	fcb.bb_start_bit = nand_info->bch_geometry.block_mark_bit_offset;

	fcb.phy_offset = fcb.pagesize;

	fcb.disbbm = 0;

	fcb.fw1_start = spl_ri->si->start[0] / fcb.pagesize;
	fcb.fw2_start = spl_ri->si->start[1] / fcb.pagesize;
	fcb.fw1_pages = (spl_ri->sub[0].size + fcb.pagesize - 1) / fcb.pagesize;
	fcb.fw2_pages = fcb.fw1_pages;

	fcb.checksum = fs_image_bcb_checksum((void *)&fcb.fingerprint,
					     sizeof(fcb) - 4);

	/* Fill DBBT-DATA */
	memset(&dbbt_data[2], 0xFF, DBBT_DATA_ENTRIES * 4);
	bad_blocks = 0;
	for (i = 0; i < DBBT_DATA_BLOCKS; i++) {
		uint offs = i * fi->mtd->erasesize;

		if (mtd_block_isbad(fi->mtd, offs)) {
			debug("- Found bad block 0x%x\n", offs);
			dbbt_data[2 + bad_blocks] = i;
			if (++bad_blocks >= DBBT_DATA_ENTRIES)
				break;
		}
	}
	debug("- Total %u bad block(s)\n", bad_blocks);
	dbbt_data[1] = bad_blocks;
	dbbt_data[0] = fs_image_bcb_checksum(&dbbt_data[1], bad_blocks * 4 + 4);

	/* Fill DBBT (Discovered Bad Block Table) */
	memset(&dbbt, 0, sizeof(struct dbbt_block));
	dbbt.fingerprint = DBBT_FINGERPRINT;
	dbbt.version = DBBT_VERSION_1;
	dbbt.dbbtpages = (bad_blocks > 0);
	dbbt.checksum = fs_image_bcb_checksum(&dbbt.fingerprint,
					      sizeof(dbbt) - 4);

#ifdef CONFIG_IMX8MM
	/* On i.MX8MM, SPL starts on offset 0x400 in the middle of the page */
	spl_ri->sub[0].offset += 0x400;
#endif

	fs_image_region_create(&bcb_ri, &bcb_si, bcb_sub);
	fs_image_region_add_raw(&bcb_ri, &fcb, "FCB", arch, 0,
				SUB_IS_FCB | SUB_SYNC, sizeof(fcb));
	fs_image_region_add_raw(&bcb_ri, &dbbt, "DBBT", arch,
				4 * fi->mtd->writesize,
				SUB_IS_DBBT | SUB_SYNC, sizeof(dbbt));
	if (bad_blocks > 0) {
		fs_image_region_add_raw(&bcb_ri, dbbt_data, "DBBT-DATA",
					arch, 8 * fi->mtd->writesize,
					SUB_IS_DBBT_DATA | SUB_SYNC,
					bad_blocks * 4 + 8);
	}

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
	 * damaged Primary copy is updated first and this succeeds, then the
	 * Primary copy is repaired and provides a working fallback when
	 * writing the Secondary copy afterwards.
	 *
	 * Start with the "other" copy:
	 *
	 *  1. Invalidate the "other" NBOOT region by erasing the region. This
	 *     immediately invalidates the F&S header of the BOARD-CFG so that
	 *     this copy will definitely not be loaded anymore.
	 *  2. Write all of the "other" NBOOT but the first block. If
	 *     interrupted, the BOARD-CFG ist still invalid and will not be
	 *     loaded.
	 *  3. Write first block of the "other" NBOOT. This adds the F&S
	 *     header and makes NBOOT valid.
	 *  4. Erase the "other" SPL region. This immediately invalidates SPL
	 *     (IVT) so that this copy will definitely not be loaded anymore.
	 *  5. Write all of the "other" SPL but the first block. If
	 *     interrupted, SPL is still invalid and will not be loaded.
	 *  6. Write the first block of the "other" SPL. This adds the IVT and
	 *     makes SPL valid.
	 *  7. Erase the "other" BCB region.
	 *  8. Write DBBT and DBBT-DATA of the "other" BCB.
	 *  9. Write FCB of the "other" BCB. This validates the BCB.
	 *
	 * If interrupted somewhere in steps 1 to 6, the "current" copy is
	 * still available and will continue to boot. After step 6, the
	 * "other" copy is fully functional. So if the "other" copy is the
	 * Primary copy, it will be booted after Step 6 again. (Unless the
	 * start address of new SPL has changed so that the old BCB entries do
	 * not point to it anymore. In that case, the Primary copy will be
	 * started after step 9 again.)
	 *
	 * 10. Update the "current" NBOOT in the same sequence.
	 * 11. Update the "current" SPL in the same sequence.
	 * 12. Update the "current" BCB in the same sequence.
	 *
	 * The worst case happens if interrupted in step 10 and if "current"
	 * is the Primary copy. Then the "current" (=Primary) but still old SPL
	 * will boot, but fails to load the "current" (=Primary) NBOOT,
	 * because it is invalid right now. So it will fall back to load the
	 * "other" (=Secondary) NBOOT, which is the new version already. This
	 * may or may not work, depending on how compatible the old and new
	 * versions are.
	 *
	 * If interrupted in step 11, the "other" (=Secondary) copy is loaded,
	 * which is the new version already. This is OK. If interrupted in
	 * step 12, the "current" (=Primary) copy is loaded, but then SPL and
	 * NBOOT are also both the new version, so this is OK, too.
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

		if (atf_ri && fs_image_save_region(fi, copy, atf_ri))
			failed |= BIT(copy);

		if (fs_image_save_region(fi, copy, spl_ri))
			failed |= BIT(copy);

		if (fs_image_save_region(fi, copy, &bcb_ri))
			failed |= BIT(copy);
		copy = 1 - copy;
	} while (copy != start_copy);

	return failed;
}

static int fs_image_set_boot_hwpart_nand(struct flash_info *fi, int boot_hwpart)
{
	/* Nothing to do on NAND */
	return 0;
}


static void fs_image_put_flash_nand(struct flash_info *fi)
{
	/* Nothing to be done in case of NAND */
}

struct flash_ops flash_ops_nand = {
	.check_for_uboot = fs_image_check_for_uboot_nand,
	.check_for_nboot = fs_image_check_for_nboot_nand,
	.get_nboot_info = fs_image_get_nboot_info_nand,
	.si_differs = fs_image_si_differs_nand,
	.read = fs_image_read_nand,
	.load_image = fs_image_load_image_nand,
	.load_extra = fs_image_load_extra_nand,
	.invalidate = fs_image_invalidate_nand,
	.write = fs_image_write_nand,
	.prepare_region = fs_image_prepare_region_nand,
	.save_nboot = fs_image_save_nboot_nand,
	.set_hwpart = fs_image_set_hwpart_nand,
	.set_boot_hwpart = fs_image_set_boot_hwpart_nand,
	.put_flash = fs_image_put_flash_nand,
};

/* ------------- Global access functions ----------------------------------- */

int fs_image_get_flash_nand(struct flash_info *fi, int devnum)
{
	fi->mtd = get_nand_dev_by_index(devnum);
	if (!fi->mtd) {
		puts("NAND not found\n");
		return -ENODEV;
	}
	fi->ops = &flash_ops_nand;

	/* Temporary buffer is for one page */
	fi->temp_size = fi->mtd->writesize;
	fi->temp_fill = 0xff;

	/* Set device name */
	strcpy(fi->devname, "NAND");

	return 0;
}

static const struct env_info fs_image_known_env_nand[] = {
	{
		.start = {0x480000, 0x4c0000},
		.size = 0x4000
	},
};

int fs_image_get_known_env_nand(uint index, uint start[2], uint *size)
{
	const struct env_info *env_info;

	printf("Using env location fallback #%d for old NBoot\n", index);
	if (index > ARRAY_SIZE(fs_image_known_env_nand)) {
		puts("No such fallback for env location\n");
		return -EINVAL;
	}

	env_info = &fs_image_known_env_nand[index];

	start[0] = env_info->start[0];
	start[1] = env_info->start[1];
	if (size)
		*size = env_info->size;

	return 0;
}
#else
//### NAND not supported yet
int fs_image_get_flash_nand(struct flash_info *fi, int devnum)
{
	return -ENODEV;
}

#endif /* __UBOOT__ */
