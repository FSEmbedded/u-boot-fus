// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright 2025 F&S Elektronik Systeme GmbH
 *
 * Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
 *
 * Common F&S image handling
 *
 */

#ifdef __UBOOT__
#include <common.h>
#include <fdt_support.h>		/* fdt_getprop_u32_default_node() */
//####include <spl.h>
//####include <mmc.h>
//####include <nand.h>
//####include <sdp.h>
//####include <asm/sections.h>
#include <asm/global_data.h>		/* DECLARE_GLOBAL_DATA_PTR */
#include <u-boot/crc.h>			/* crc32() */

#include <asm/mach-imx/checkboot.h>
#ifdef CONFIG_FS_SECURE_BOOT
//####include <asm/mach-imx/hab.h>
//####include <stdbool.h>
#endif

#else

#include <linux/kconfig.h>		/* Get kconfig macros only */
#include "linux_helpers.h"
//####include "../../../include/fdt_support.h"
#include <linux/libfdt.h>
#include <asm/mach-imx/hab.h>		/* struct ivt, ... */
//####include <asm/mach-imx/checkboot.h>	/* HAB_HEADER */
//####include "../../../include/linux/libfdt_env.h"
//####include <string.h>
#include <stdio.h>
#include <errno.h>
#include "crc32.h"
#endif /* __UBOOT__ */

#include "fs_board_common.h"		/* fs_board_is_closed() */
#include "fs_image_common.h"		/* Own interface */
#include "fs_cntr_common.h"		/* fs_cntr_*() */


/* Structure to handle board name and revision separately */
struct bnr {
	char name[MAX_DESCR_LEN];
	unsigned int rev;
};

static struct bnr compare_bnr;		/* Used for BOARD-ID comparisons */
static char board_id[MAX_DESCR_LEN + 1]; /* Current board-id */


/* ------------- Functions in SPL and U-Boot ------------------------------- */

#ifdef __UBOOT__
/* Return the F&S architecture */
const char *fs_image_get_arch(void)
{
	return CONFIG_SYS_BOARD;
}

/* Return the intended address of the board configuration in OCRAM */
void *fs_image_get_regular_cfg_addr(void)
{
	return (void*)CFG_FUS_BOARDCFG_ADDR;
}

/* Return the real address of the board configuration in OCRAM */
void *fs_image_get_cfg_addr(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	if (!gd->board_cfg)
		gd->board_cfg = (ulong)fs_image_get_regular_cfg_addr();

	return (void *)gd->board_cfg;
}
#endif /* __UBOOT__ */

/* Return the fdt part of the given board configuration */
void *fs_image_find_cfg_fdt(struct fs_header_v1_0 *fsh)
{
	void *fdt = fsh + 1;

#if defined(CONFIG_IMX_HAB)
	if (fs_image_is_signed(fsh))
		fdt += HAB_HEADER;
#endif
	if (fdt_check_header(fdt))
		return NULL;

	return fdt;
}

/* Check if this is an F&S image */
bool fs_image_is_fs_image(const struct fs_header_v1_0 *fsh)
{
	return !strncmp(fsh->info.magic, "FSLX", sizeof(fsh->info.magic));
}

/* Return the fdt part of the given board configuration with index header */
void *fs_image_find_cfg_fdt_idx(struct index_info *cfg_info)
{
	void *fdt;

	if(!cfg_info)
		return NULL;
	
	if(cfg_info->fsh_idx == NULL)
		return fs_image_find_cfg_fdt(cfg_info->fsh_idx_entry);
	
	fdt = (void *)cfg_info->fsh_idx_entry;
	fdt += sizeof(struct fs_header_v1_0) + cfg_info->offset;

	if(fdt_check_header(fdt)){
		return NULL;
	}
	
	return fdt;
}

/* Return the fdt part of the board configuration in OCRAM */
void *fs_image_get_cfg_fdt(void)
{
	return fs_image_find_cfg_fdt(fs_image_get_cfg_addr());
}

/* Return the address of the /nboot-info node */
int fs_image_get_nboot_info_offs(void *fdt)
{
	return fdt_path_offset(fdt, "/nboot-info");
}

/* Return the address of the /board-cfg node */
int fs_image_get_board_cfg_offs(void *fdt)
{
	return fdt_path_offset(fdt, "/board-cfg");
}

/* Return pointer to string with NBoot version */
const char *fs_image_get_nboot_version(void *fdt)
{
	int offs;

	if (!fdt)
		fdt = fs_image_get_cfg_fdt();

	offs = fs_image_get_nboot_info_offs(fdt);
	return fdt_getprop(fdt, offs, "version", NULL);
}

/* Read the image size (incl. padding) from an F&S header */
unsigned int fs_image_get_size(const struct fs_header_v1_0 *fsh,
			       bool with_fs_header)
{
	/* We ignore the high word, boot images are definitely < 4GB */
	return fsh->info.file_size_low + (with_fs_header ? FSH_SIZE : 0);
}

/* return the size of extra data after FS HEADER */
unsigned int fs_image_get_extra_size(const struct fs_header_v1_0 *fsh)
{
	if (!(fsh->info.flags & FSH_FLAGS_EXTRA))
		return 0;

	return fsh->param.p32[7];
}

bool fs_image_is_index(const struct fs_header_v1_0 *fsh)
{
	return !!(fsh->info.flags & FSH_FLAGS_INDEX);
}

/* return number of index entries */
unsigned int fs_image_index_get_n(const struct fs_header_v1_0 *fsh)
{
	unsigned int size;
	if(!fs_image_is_index(fsh))
		return 0;

	size = fs_image_get_size(fsh, false);
	return size / FSH_SIZE;
}

/* Check image magic, type and descr; return true on match */
bool fs_image_match(const struct fs_header_v1_0 *fsh,
		    const char *type, const char *descr)
{
	if (!type)
		return false;

	if (!fs_image_is_fs_image(fsh))
		return false;

	if (strncmp(fsh->type, type, MAX_TYPE_LEN))
		return false;

	if (descr && descr[0] != 0) {
		if (!(fsh->info.flags & FSH_FLAGS_DESCR))
			return false;
		if (strncmp(fsh->param.descr, descr, MAX_DESCR_LEN))
			return false;
	}

	return true;
}

static void fs_image_get_board_name_rev(const char id[MAX_DESCR_LEN],
					struct bnr *bnr)
{
	char c;
	int i;
	int rev = -1;

	/* Copy string and look for rightmost '.' */
	bnr->rev = 0;
	i = 0;
	do {
		c = id[i];
		bnr->name[i] = c;
		if (!c)
			break;
		if (c == '.')
			rev = i;
	} while (++i < sizeof(bnr->name));

	/* No revision found, assume 0 */
	if (rev < 0)
		return;

	bnr->name[rev] = '\0';
	while (++rev < i) {
		char c = bnr->name[rev];

		if ((c < '0') || (c > '9'))
			break;
		bnr->rev = bnr->rev * 10 + c - '0';
	}
}

/* Check if ID of the given BOARD-CFG matches the compare_id */
bool fs_image_match_board_id(struct fs_header_v1_0 *cfg_fsh)
{
	struct bnr bnr;

	/* Compare magic and type */
	if (!fs_image_match(cfg_fsh, "BOARD-CFG", NULL))
		return false;

	/* A config must include a description, this is the board ID */
	if (!(cfg_fsh->info.flags & FSH_FLAGS_DESCR))
		return false;

	/* Split board ID of the config we look at into name and rev */
	fs_image_get_board_name_rev(cfg_fsh->param.descr, &bnr);

	/*
	 * Compare with name and rev of the BOARD-ID we are looking for (in
	 * compare_bnr). In the new layout, the BOARD-CFG does not have a
	 * revision anymore, so here bnr.rev is 0 and is accepted for any
	 * BOARD-ID of this board type.
	 */
	if (strncmp(bnr.name, compare_bnr.name, sizeof(bnr.name)))
		return false;
	if (bnr.rev > compare_bnr.rev)
		return false;

	return true;
}

/* Read property from board-rev subnode or board-cfg main node */
const void *fs_image_getprop(const void *fdt, int cfg_offs, int rev_offs,
			     const char *name, int *lenp)
{
	const void *prop;

	if (rev_offs) {
		prop = fdt_getprop(fdt, rev_offs, name, lenp);
		if (prop)
			return prop;
	}

	return fdt_getprop(fdt, cfg_offs, name, lenp);
}

/* Read u32 property from board-rev subnode or board-cfg main node */
u32 fs_image_getprop_u32(const void *fdt, int cfg_offs, int rev_offs,
			 int cell, const char *name, const u32 dflt)
{
	const void *prop;
	int len;

	if (rev_offs) {
		prop = fdt_getprop(fdt, rev_offs, name, &len);
		if (prop)
			return fdt_getprop_u32_default_node(fdt, rev_offs,
							    cell, name, dflt);
	}

	return fdt_getprop_u32_default_node(fdt, cfg_offs, cell, name, dflt);
}

#if !defined(CONFIG_FS_CNTR_COMMON)
/* Check if the F&S image is signed (followed by an IVT) */
bool fs_image_is_signed(struct fs_header_v1_0 *fsh)
{
	struct ivt *ivt = (struct ivt *)(fsh + 1);

	return (ivt->hdr.magic == IVT_HEADER_MAGIC);
}

/* Check IVT integrity of F&S image and return size and validation address */
void *fs_image_get_ivt_info(struct fs_header_v1_0 *fsh, u32 *size)
{
	struct ivt *ivt = (struct ivt *)(fsh + 1);
	struct boot_data *boot;

	/*
	 * Layout of a signed F&S image (SPL differs, but does not matter here)
	 *
	 * Offset    Header-Entry    Size                     Content
	 * -----------------------------------------------------------------
	 * 0x00      boot->start     0x40 (FSH-SIZE)          F&S header
	 * 0x40      ivt->self       0x20 (IVT_TOTAL_LENGTH)  IVT
	 * 0x60      ivt->boot       0x20                     boot_data
	 * 0x80      ivt->entry      n                        Image data
	 * 0x80+n    ivt->csf        m                        CSF
	 * -----------------------------------------------------------------
	 *           boot->length <- total size: n+m+0x80
	 *
	 * Remark: The size of IVT and boot_data together is HAB_HEADER
	 */
	if ((ivt->boot != ivt->self + IVT_TOTAL_LENGTH)
	    || (ivt->entry != ivt->self + HAB_HEADER))
		return NULL;

	boot = (struct boot_data *)(ivt + 1);
	*size = boot->length;

	return (void *)(ulong)(boot->start);
}

/* Validate a signed image; it has to be at the validation address */
bool fs_image_is_valid_signature(struct fs_header_v1_0 *fsh)
{
	struct ivt *ivt = (struct ivt *)(fsh + 1);
	void *start;
	u32 size;

	/* Check IVT integrity */
	start = fs_image_get_ivt_info(fsh, &size);
	if (!start || (start != fsh) || ((ulong)(ivt->self) != (ulong)ivt))
		return false;

#ifdef CONFIG_FS_SECURE_BOOT
	{
		u16 flags;
		u32 file_size_high;
		u32 *pcs = (u32 *)&fsh->type[12];
		u32 crc32;
		int err;

		/*
		 * The signature was created without board revision in
		 * file_size_high and without CRC32, so clear these values
		 * temporarily.
		 */
		file_size_high = fsh->info.file_size_high;
		fsh->info.file_size_high = 0;
		flags = fsh->info.flags;
		fsh->info.flags &= ~(FSH_FLAGS_SECURE | FSH_FLAGS_CRC32);
		crc32 = *pcs;
		*pcs = 0;

		/* Check signature */
		err = imx_hab_authenticate_image((u32)(ulong)fsh,
				fs_image_get_size(fsh, true), FSH_SIZE);

		/* Bring back the saved values */
		fsh->info.file_size_high = file_size_high;
		fsh->info.flags = flags;
		*pcs = crc32;

		if (err)
			return false;
	}
#endif

	return true;
}
#else
bool fs_image_is_signed(struct fs_header_v1_0 *fsh)
{
	struct container_hdr *cntr_hdr = (struct container_hdr *)(fsh + 1);

	if(!valid_container_hdr(cntr_hdr)){
		char type[MAX_TYPE_LEN + 1];
		memcpy(type, fsh->type, MAX_TYPE_LEN);
		type[MAX_TYPE_LEN] = 0;
		printf("%s has no IMX-Container, skip validation ...\n", type);
		return false;
	}

	return fs_cntr_is_signed(cntr_hdr);
}

bool fs_image_is_valid_signature(struct fs_header_v1_0 *fsh)
{
	struct container_hdr *cntr_hdr = (struct container_hdr *)(fsh + 1);
	struct signature_block_hdr *sig_hdr;
	unsigned int offset;

	if(!valid_container_hdr(cntr_hdr)){
		char type[MAX_TYPE_LEN + 1];
		memcpy(type, fsh->type, MAX_TYPE_LEN);
		type[MAX_TYPE_LEN] = 0;
		printf("%s does not contain IMX-Container\n", type);
		return false;
	}

	if(!fs_image_match(fsh, "BOOT-INFO", NULL))
		return fs_cntr_is_valid_signature(cntr_hdr);

	/**
	 * BOOT-INFO image contains two bootcntr.
	 * One container contains the ELE-FW,
	 * the second contains the SPL and optionally an M33_Image
	 */
	sig_hdr = (void *)cntr_hdr + cntr_hdr->sig_blk_offset;
	offset = cntr_hdr->sig_blk_offset + sig_hdr->length_lsb + (sig_hdr->length_msb << 8);
	offset = ALIGN(offset, CONTAINER_HDR_ALIGNMENT);
	cntr_hdr = (void *)cntr_hdr + offset;

	if(!valid_container_hdr(cntr_hdr)){
		printf("No IMX-Container found!\n");
		return false;
	}

	/* if second cntr is not signed then it is OK when board is OEM open */
	if(!fs_cntr_is_signed(cntr_hdr) && !fs_board_is_closed()){
		printf("NOTE: Second BOOT-CNTR is unsigned\n");
		return true;
	}

	return fs_cntr_is_valid_signature(cntr_hdr);
}
#endif /* !CONFIG_IS_ENABLED(FS_CNTR_COMMON) */

/*
 * Check CRC32; return: 0: No CRC32, >0: CRC32 OK, <0: error (CRC32 failed)
 * If CRC32 is OK, the result also gives information about the type of CRC32:
 * 1: CRC32 was Header only (only FSH_FLAGS_SECURE set)
 * 2: CRC32 was Image only (only FSH_FLAGS_CRC32 set)
 * 3: CRC32 was Header+Image (both FSH_FLAGS_SECURE and FSH_FLAGS_CRC32 set)
 */
int fs_image_check_crc32_offset(const struct fs_header_v1_0 *fsh,
				unsigned int offset)
{
	u32 expected_cs;
	u32 computed_cs;
	u32 *pcs;
	unsigned int size;
	unsigned char *start;
	int ret = 0;

	if (!(fsh->info.flags & (FSH_FLAGS_SECURE | FSH_FLAGS_CRC32)))
		return 0;		/* No CRC32 */

	if (fsh->info.flags & FSH_FLAGS_SECURE) {
		start = (unsigned char *)fsh;
		size = FSH_SIZE;
		ret |= 1;
	} else {
		start = (unsigned char *)(fsh + 1);
		start += offset;
		size = 0;
	}

	if (fsh->info.flags & FSH_FLAGS_CRC32) {
		size += fs_image_get_size(fsh, false);
		ret |= 2;
	}

	/* CRC32 is in type[12..15]; temporarily set to 0 while computing */
	pcs = (u32 *)&fsh->type[12];
	expected_cs = *pcs;
	*pcs = 0;
	computed_cs = crc32(0, start, size);
	*pcs = expected_cs;
	if (computed_cs != expected_cs)
		return -EILSEQ;

	return ret;
}

int fs_image_check_crc32(const struct fs_header_v1_0 *fsh)
{
	return fs_image_check_crc32_offset(fsh, 0);
}

void fs_image_print_crc32_status(const struct fs_header_v1_0 *fsh, int err)
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

/* Update size, flags and padsize, calculate CRC32 if requested */
void fs_image_update_header(struct fs_header_v1_0 *fsh,
				   uint size, uint fsh_flags)
{
	u8 padsize = 0;

	padsize = size % 16;
	if (padsize)
		padsize = 16 - padsize;

	fsh->info.file_size_low = size;
	fsh->info.padsize = padsize;
	fsh->info.flags = fsh_flags | FSH_FLAGS_DESCR;

	if (fsh_flags & (FSH_FLAGS_CRC32 | FSH_FLAGS_SECURE)) {
		unsigned char *crc32_start = (unsigned char *)fsh;
		unsigned int crc32_size = 0;
		u32 *pcs = (u32 *)&fsh->type[12];

		*pcs = 0;
		if (fsh_flags & FSH_FLAGS_SECURE)
			crc32_size += FSH_SIZE;
		else
			crc32_start += FSH_SIZE;

		if (fsh_flags & FSH_FLAGS_CRC32)
			crc32_size += size;

		*pcs = crc32(0, crc32_start, crc32_size);
		debug("- Setting CRC32 for %s to 0x%08x\n", fsh->type, *pcs);
	}
}

/* Add the board revision as BOARD-ID to the given BOARD-CFG and update CRC32 */
void fs_image_board_cfg_set_board_rev(struct fs_header_v1_0 *cfg_fsh)
{
	ulong size = fs_image_get_size(cfg_fsh, false);
	/* Set compare_id rev in file_size_high, compute new CRC32 */
	cfg_fsh->info.file_size_high = compare_bnr.rev;

	cfg_fsh->info.flags |= FSH_FLAGS_SECURE | FSH_FLAGS_CRC32;
	
	/* calc new crc32 */
	fs_image_update_header(cfg_fsh, size, cfg_fsh->info.flags);
}

/* Return the current BOARD-ID */
const char *fs_image_get_board_id(void)
{
	return board_id;
}

void fs_image_get_bcfg_name(char *bcfg_name, ulong len)
{
	const char* board_id = fs_image_get_board_id();
	const char* ptr;
	ulong cnt;

	ptr = memchr(board_id, '.', len);
	cnt = (int)(ptr - board_id);
	strncpy(bcfg_name, board_id, cnt);
}

void fs_image_get_compare_id(char *id, uint len)
{
	char c;
	int i;

	/* Copy base name */
	for (i = 0; i < len; i++) {
		c = compare_bnr.name[i];
		if (!c)
			break;
		id[i] = c;
	}

	/* Add board revision */
	snprintf(&id[i], len - i, ".%03d", compare_bnr.rev);
}

/* Store current compare_id as board_id */
void fs_image_set_board_id(void)
{
	fs_image_get_compare_id(board_id, MAX_DESCR_LEN + 1);
}

/* Set the compare_id that will be used in fs_image_match_board_id() */
void fs_image_set_compare_id(const char id[MAX_DESCR_LEN])
{
	fs_image_get_board_name_rev(id, &compare_bnr);
}

/* Get the board-rev from BOARD-ID (in compare-id) */
unsigned int fs_image_get_board_rev(void)
{
	return compare_bnr.rev;
}

/* Get the BOARD-ID from the BOARD-CFG in OCRAM */
static void _get_board_id_from_cfg(struct bnr *bnr)
{
	struct fs_header_v1_0 *cfg_fsh = fs_image_get_cfg_addr();
	unsigned int rev;

	/* Take base ID from descr; in old layout, this includes the rev */
	fs_image_get_board_name_rev(cfg_fsh->param.descr, bnr);

	/* The BOARD-ID rev is stored in file_size_high, 0 for old layout */
	rev = cfg_fsh->info.file_size_high;
	if (rev) {
		debug("Taking BOARD-ID rev from BOARD-CFG: %d\n", rev);
		bnr->rev = rev;
	}
}

/* Set the board_id and compare_id from the BOARD-CFG */
void fs_image_set_board_id_from_cfg(void)
{
	_get_board_id_from_cfg(&compare_bnr);

	fs_image_set_board_id();
}

/*
 * Find board_rev and return board-cfg subnode matching the given id_rev, 0 if
 * no subnode was found.
 */
static int _get_board_rev_subnode(const void *fdt, int offs, uint id_rev)
{
	int subnode;
	int rev_subnode = 0;
	int rev = 0;
	unsigned int temp;

	subnode = fdt_first_subnode(fdt, offs);
	while (subnode >= 0) {
		temp = fdt_getprop_u32_default_node(fdt, subnode, 0,
						    "board-rev", 100);
		if ((temp > rev) && (temp <= id_rev)) {
			rev = temp;
			rev_subnode = subnode;
			if (rev == id_rev)
				break;
		}
		subnode = fdt_next_subnode(fdt, subnode);
	}

	/* If no subnode was found, try the board-cfg node itself */
	if (!rev_subnode) {
		rev = fdt_getprop_u32_default_node(fdt, offs, 0,
						   "board-rev", 100);
	}

	debug("BOARD-ID rev=%u, BOARD-CFG rev=%u\n", id_rev, rev);

	return rev_subnode;
}

/*
 * Find board_rev of BOARD-ID (in compare-id) and return board-cfg subnode
 * matching it. The compare-id has to be set before this call.
 */
int fs_image_get_board_rev_subnode(const void *fdt, int offs)
{
	return _get_board_rev_subnode(fdt, offs, compare_bnr.rev);
}

/*
 * In the f-phase of U-Boot, when there are no variables available, we cannot
 * call fs_image_get_board_rev_subnode() because it uses the compare-id. We
 * have to determine the BOARD-ID directly from the BOARD-CFG in OCRAM. Also
 * return the board-rev (from the BOARD-ID) in this case.
 */
int fs_image_get_board_rev_subnode_f(const void *fdt, int offs, uint *board_rev)
{
	struct bnr bnr;

	/* Get the BOARD-ID from the BOARD-CFG in OCRAM */
	_get_board_id_from_cfg(&bnr);
	if (board_rev)
		*board_rev = bnr.rev;

	return _get_board_rev_subnode(fdt, offs, bnr.rev);
}

/*
 * Make sure that BOARD-CFG in OCRAM is valid. This function is called early
 * in the boot_f phase of U-Boot, and therefore must not access any variables.
 */
bool fs_image_is_ocram_cfg_valid(void)
{
	struct fs_header_v1_0 *cfg_fsh = fs_image_get_cfg_addr();
	int err;
	const char *type = "BOARD-CFG";

	if (!fs_image_match(cfg_fsh, type, NULL))
		return false;

	/*
	 * Check the additional CRC32. The BOARD-CFG in OCRAM also holds the
	 * BOARD-ID, which is not covered by the signature, but it is covered
	 * by the CRC32. So also do the check in case of Secure Boot.
	 *
	 * The BOARD-ID is given by the board-revision that is stored (as
	 * number) in unused entry file_size_high and is typically between 100
	 * and 999. This is the only part that may differ, the base name is
	 * always the same as of the ID of the BOARD-CFG itself (in descr).
	 */
	err = fs_image_check_crc32(cfg_fsh);
	if (err < 0)
		return false;

#if defined(CONFIG_IMX_HAB)
	/* Handle signed image */
	if (fs_image_is_signed(cfg_fsh))
		return fs_image_is_valid_signature(cfg_fsh);
#endif

	/* Handle unsigned image */
#ifdef CONFIG_FS_SECURE_BOOT
	if (imx_hab_is_enabled()) {
		printf("\nError: Refusing unsigned %s on closed board\n", type);
		return false;
	}
#endif

	return true;
}

