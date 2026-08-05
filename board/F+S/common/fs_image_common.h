// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 F&S Elektronik Systeme GmbH
 *
 * Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
 *
 * F&S image processing
 *
 */

#ifndef __FS_IMAGE_COMMON_H__
#define __FS_IMAGE_COMMON_H__

#include <linux/compiler_attributes.h>	/* __nonstring */
#include <asm/mach-imx/boot_mode.h>	/* enum boot_mode */

#define MAX_TYPE_LEN 16
#define MAX_DESCR_LEN 32

struct index_info {
	const struct fs_header_v1_0 *fsh_idx;	/* Header of INDEX image */
	const struct fs_header_v1_0 *fsh;	/* Header within INDEX */
	const void *fsi;			/* Pointer to image part */
};

/* F&S header (V0.0) for a generic file */
struct fs_header_v0_0 {			/* Size: 16 Bytes */
	char magic[4];			/* "FS" + two bytes operating system
					   (e.g. "LX" for Linux) */
	u32 file_size_low;		/* Image size [31:0] */
	u32 file_size_high;		/* Image size [63:32] */
	u16 flags;			/* See flags below */
	u8 padsize;			/* Number of padded bytes at end */
	u8 version;			/* Header version x.y:
					   [7:4] major x, [3:0] minor y */
};

/* F&S header (V1.0) for a generic file */
struct fs_header_v1_0 {			/* Size: 64 bytes */
	struct fs_header_v0_0 info;	/* Image info, see above */
	char type[16] __nonstring;	/* Image type, e.g. "U-BOOT" */
	union {
		char descr[32] __nonstring; /* Description, null-terminated */
		u8 p8[32];		/* 8-bit parameters */
		u16 p16[16];		/* 16-bit parameters */
		u32 p32[8];		/* 32-bit parameters */
		u64 p64[4];		/* 64-bit parameters */
	} param;
};

/* Possible values for flags entry above */
#define FSH_FLAGS_DESCR		0x8000	/* Description descr is present */
#define FSH_FLAGS_CRC32		0x4000	/* CRC32 of image in type[12..15] */
#define FSH_FLAGS_SECURE	0x2000	/* CRC32 of header in type[12..15] */
#define FSH_FLAGS_INDEX		0x1000	/* Image contains an index */
#define FSH_FLAGS_EXTRA 	0x0800	/* Extra offset sub-header in p32[7] */
#define FSH_SIZE sizeof(struct fs_header_v1_0)

/* Get the boot device number from the string */
enum boot_device fs_image_get_boot_dev_from_name(const char *name);

/* Get the string from the boot device number */
const char *fs_image_get_name_from_boot_dev(enum boot_device boot_dev);

/* Return the F&S architecture */
const char *fs_image_get_arch(void);

/* Check if this is an F&S image */
bool fs_image_is_fs_image(const struct fs_header_v1_0 *fsh);

/* Return the intended address of the board configuration in OCRAM */
void *fs_image_get_regular_cfg_addr(void);

/* Return the real address of the board configuration in OCRAM */
void *fs_image_get_cfg_addr(void);

/* Return the fdt part of the board configuration in OCRAM */
const void *fs_image_get_cfg_fdt(void);

/* Return the fdt part of the given board configuration */
const void *fs_image_find_cfg_fdt(const struct fs_header_v1_0 *fsh);

/* Return the fdt part of the given board configuration with index header */
const void *fs_image_find_cfg_fdt_idx(struct index_info *cfg_info);

#if 0 //###
const struct fs_header_v1_0 *fs_image_find(const struct fs_header_v1_0 *fsh,
					   const char *type, const char *descr,
					   struct index_info *idx_info);
#endif //###

/* Return the address of the /board-cfg node */
int fs_image_get_board_cfg_offs(const void *fdt);

/* Return the address of the /nboot-info node */
int fs_image_get_nboot_info_offs(const void *fdt);

/* Return NBoot version by looking in given fdt (or BOARD-CFG if NULL) */
const char *fs_image_get_nboot_version(const void *fdt);

/* Read the image size (incl. padding) from an F&S header */
unsigned int fs_image_get_size(const struct fs_header_v1_0 *fsh,
			       bool with_fs_header);

/* return the size of extra data after FS HEADER */
unsigned int fs_image_get_extra_size(const struct fs_header_v1_0 *fsh);

bool fs_image_is_index(const struct fs_header_v1_0 *fsh);

/* return number of index entries */
unsigned int fs_image_index_get_n(const struct fs_header_v1_0 *fsh);

/* Check image magic, type and descr; return true on match */
bool fs_image_match(const struct fs_header_v1_0 *fsh,
		    const char *type, const char *descr);

/* Check id, return also true if revision is less than revision of compare_id */
bool fs_image_match_board_id(const struct fs_header_v1_0 *fsh);

/* Read property from board-rev subnode or board-cfg main node */
const void *fs_image_getprop(const void *fdt, int cfg_offs, int rev_offs,
			     const char *name, int *lenp);

/* Read u32 property from board-rev subnode or board-cfg main node */
u32 fs_image_getprop_u32(const void *fdt, int cfg_offs, int rev_offs,
			 int cell, const char *name, const u32 dflt);

/* Update size, flags and padsize, calculate CRC32 if requested */
void fs_image_update_header(struct fs_header_v1_0 *fsh,
				   uint size, uint fsh_flags);

/* Add the board revision as BOARD-ID to the given BOARD-CFG and update CRC32 */
void fs_image_board_cfg_set_board_rev(struct fs_header_v1_0 *cfg_fsh);

/* Return the current BOARD-ID */
const char *fs_image_get_board_id(void);

/* Return BCFG name based on BOARD-ID */
void fs_image_get_bcfg_name(char *bcfg_name, ulong len);

/* Set the compare_id that will be used in fs_image_match_board_id() */
void fs_image_set_compare_id(const char id[MAX_DESCR_LEN]);

/* Get the compare_id that will be used in fs_image_match_board_id() */
void fs_image_get_compare_id(char *id, uint len);

/* Get the board-rev from BOARD-ID (in compare-id) */
unsigned int fs_image_get_board_rev(void);

/* Store current compare_id as board_id */
void fs_image_set_board_id(void);

/* Set the board_id and compare_id from the BOARD-CFG */
void fs_image_set_board_id_from_cfg(void);

/* Find board-cfg subnode matching the board-rev in the BOARD-ID */
int fs_image_get_board_rev_subnode(const void *fdt, int offs);

/* Find board-rev and return matching board-cfg subnode (U-Boot f-phase) */
int fs_image_get_board_rev_subnode_f(const void *fdt, int offs,
				     uint *board_rev);

/* Check if the F&S image is signed (followed by an IVT or SIG. HEADER) */
bool fs_image_is_signed(const struct fs_header_v1_0 *fsh);

/* Validate a signed image; it has to be at the validation address */
bool fs_image_is_valid_signature(struct fs_header_v1_0 *fsh);

#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
/* Check IVT integrity of F&S image and return size and validation address */
void *fs_image_get_ivt_info(struct fs_header_v1_0 *fsh, u32 *size);
#endif

/* Verify CRC32 of given image at specific offset */
int fs_image_check_crc32_split(const struct fs_header_v1_0 *fsh,
			       const void *fsi);

/* Verify CRC32 of given image */
int fs_image_check_crc32(const struct fs_header_v1_0 *fsh);

/* Make sure that BOARD-CFG in OCRAM is valid */
bool fs_image_is_ocram_cfg_valid(void);

/* Authenticate an FS-Image at a testing address and copy it to its load address */
#ifdef CONFIG_FS_SECURE_BOOT
int authenticate_fs_image(void *final_addr, void *check_addr,
	ulong image_offset, int image_type, bool header);
struct sb_info {
	void *final_addr;
	void *check_addr;
	bool header;
	struct ivt* image_ivt;
	int image_type;
};
#endif

/* ------------- Stuff only for U-Boot ------------------------------------- */

#ifndef CONFIG_SPL_BUILD

/* Return if currently running from Secondary SPL. */
bool fs_image_is_secondary(void);

/* Return if currently running from Secondary UBoot. */
bool fs_image_is_secondary_uboot(void);

/*
 * Search board configuration in OCRAM; return true if it was found.
 * From now on, fs_image_get_cfg_addr() will return the right address.
 */
bool fs_image_find_cfg_in_ocram(void);

/* Get count values from given device tree property and check alignment */
int fs_image_get_fdt_val(const void *fdt, int offs, const char *name, uint align,
			 int count, uint *val);


/* --- ### from fsimage.c */

struct env_info {
	unsigned int start[2];
	unsigned int size;
};

/* Structure to hold regions in NAND/eMMC for an image, taken from nboot-info */
struct storage_info {
	uint start[2];			/* *-start entries */
	uint size;			/* *-size entry */
#ifdef CONFIG_CMD_MMC
	u8 hwpart[2];			/* hwpart (in case of eMMC) */
#endif
	const char *type;		/* Name of storage region */
};

/* Storage info from the nboot-info of a BOARD-CFG in binary form */
#define NI_SUPPORT_CRC32       BIT(0)	/* Support CRC32 in F&S headers */
#define NI_SAVE_BOARD_ID       BIT(1)	/* Save the board-rev. in BOARD-CFG */
#define NI_UBOOT_WITH_FSH      BIT(2)	/* Save U-Boot with F&S Header */
#define NI_UBOOT_EMMC_BOOTPART BIT(3)	/* On eMMC when booting from boot part,
					   also save U-Boot in boot part */
#define NI_EMMC_BOTH_BOOTPARTS BIT(4)	/* On eMMC when booting from boot part,
					   use both boot partitions, one for
					   each copy */
#define NI_SUPPORT_U_ATF       BIT(5)	/* Support for user defined U_ATF/U_TEE
					   in addition to system ATF/TEE */

#define MAX_SUB_IMGS	8 		/* Max Array-Size for Sub-Images */

struct nboot_info {
	uint flags;			/* See NI_* above */
	uint board_cfg_size;
	struct storage_info spl;
	struct storage_info nboot;
	struct storage_info atf;
	struct storage_info uboot;
	struct storage_info env;
};

#define SUB_SYNC          BIT(0)	/* After writing image, flush temp */
#define SUB_HAS_FS_HEADER BIT(1)	/* Image has an F&S header in flash */
#define SUB_IS_SPL        BIT(2)	/* SPL: has IVT, may beed offset */
#define SUB_IS_ENV        BIT(3)	/* Environment data */
#ifdef CONFIG_NAND_MXS
#define SUB_IS_FCB        BIT(4)	/* FCB: needs other ECC */
#define SUB_IS_DBBT       BIT(5)
#define SUB_IS_DBBT_DATA  BIT(6)
#endif

struct sub_info {
	void *img;			/* Pointer to image */
	const char *type;		/* "BOARD-CFG", "FIRMWARE", "SPL" */
	const char *descr;		/* e.g. board architecture */
	uint size;			/* Size of image */
	uint offset;			/* Offset of image within si */
	uint flags;			/* See SUB_* above */
};

struct region_info {
	struct storage_info *si;	/* Region information */
	struct sub_info *sub;		/* Pointer to subimages */
	int count;			/* Number of subimages */
};

/* Access functions that differ between NAND and MMC */
struct flash_info;
struct flash_ops {
	bool (*check_for_uboot)(struct storage_info *si, bool force);
	bool (*check_for_nboot)(struct flash_info *fi, struct storage_info *si,
				bool force);
	int (*get_nboot_info)(struct flash_info *fi, const void *fdt, int offs,
			      struct nboot_info *ni, int hwpart, bool show,
			      uint index);
	bool (*si_differs)(const struct storage_info *si1,
			   const struct storage_info *si2);
	int (*read)(struct flash_info *fi, uint offs, uint size, uint lim,
		    uint flags, u8 *buf);
	int (*load_image)(struct flash_info *fi, int copy,
			  const struct storage_info *si, struct sub_info *sub);
	int (*load_extra)(struct flash_info *fi, struct storage_info *spl,
			  void *tempaddr);
	int (*invalidate)(struct flash_info *fi, int copy,
			  const struct storage_info *si);
	int (*write)(struct flash_info *fi, uint offs, uint size, uint lim,
		     uint flags, u8 *buf);
	int (*prepare_region)(struct flash_info *fi, int copy,
			      struct storage_info *si);
	int (*save_nboot)(struct flash_info *fi, struct region_info *nboot_ri,
			  struct region_info *atf_ri,
			  struct region_info *spl_ri);
	int (*set_hwpart)(struct flash_info *fi, int copy,
			  const struct storage_info *si);
	int (*set_boot_hwpart)(struct flash_info *fi, int boot_hwpart);
	int (*read_board_cfg)(struct flash_info *fi, int copy, void *board_cfg);
	void (*put_flash)(struct flash_info *fi);
};

#define MAX_FI_DEVNAME 10
struct flash_info {
#ifdef CONFIG_NAND_MXS
	struct mtd_info *mtd;		/* Handle to NAND */
	uint env_used;			/* From env-size entry, region size
					   is from env-range */
#endif
#ifdef CONFIG_CMD_MMC
	u8 boot_hwpart;			/* HW partition we boot from (0..2) */
	u32 boot_part_size;		/* Size of each boot partition */
#endif
	char devname[MAX_FI_DEVNAME];	/* Name of device (NAND, mmc<n>) */
	u8 *temp;			/* Buffer for one NAND page/MMC block */
	uint temp_size;			/* Size of temp buffer */
	uint base_offs;			/* Offset where temp will be written */
	uint write_pos;			/* temp contains data up to this pos */
	uint bb_extra_offs;		/* Extra offset due to bad blocks */
	u8 temp_fill;			/* Default value for temp buffer */
	enum boot_device boot_dev;	/* Device to boot from */
	const char *boot_dev_name;	/* Boot device as string */
	struct flash_ops *ops;		/* Access functions for NAND/MMC */
};

struct fs_image_params {
	ulong addr;			/* Load address of image */
	ulong size;			/* Size of image */
	const char *fname;		/* Filename of image or NULL */
#ifdef __UBOOT__
	char *interface;		/* Interface (mmc, usb, ubifs, ...) */
	char *devpart;			/* <dev[:part]> */
#endif
};

extern const char fsimage_usage[];

/* Get start[0..1] and size for a storage info */
int fs_image_get_si(const void *fdt, int offs, uint align, const char *type,
		    struct storage_info *si);

//###int fs_image_get_nboot_info(struct flash_info *fi, void *fdt,
//###			    struct nboot_info *ni, int hwpart, bool show);

enum parse_type {
	PARSE_CONTENT,
	PARSE_CHECKSUM,
};

//###void fs_image_parse_image(enum parse_type ptype, ulong addr, uint offs,
//###			  int level);

/* Set all fields of the F&S header */
//###void fs_image_set_header(struct fs_header_v1_0 *fsh, const char *type,
//###			 const char *descr, uint size, uint fsh_flags);

//###struct fs_header_v1_0 *fs_image_find_concat(struct fs_header_v1_0 *fsh,
//###					    const char *type,
//###					    const char *descr,
//###					    struct index_info *idx_info);

void fs_image_region_create(struct region_info *ri, struct storage_info *si,
			    struct sub_info *sub);

/*
 * Add a subimage with any format to the region. Return offset for next
 * subimage or 0 in case of error.
 */
void fs_image_region_add_raw(struct region_info *ri, void *img,
			     const char *type, const char *descr, uint woffset,
			     uint flags, uint size);

//###uint fs_image_region_add(struct region_info *ri, struct fs_header_v1_0 *fsh,
//###			 const char *type, const char *descr, uint woffset,
//###			 uint flags);

/*
 * Add a single F&S header with given data to the region. Return offset for
 * next subimage or 0 in case of error.
 */
//###uint fs_image_region_add_fsh(struct region_info *ri, struct fs_header_v1_0 *fsh,
//###			     const char *type, const char *descr, uint woffset);

/*
 * Search the subimage with given type/descr and add it to the region. Return
 * offset for next image or 0 in case of error.
 */
//###uint fs_image_region_find_add(struct region_info *ri,
//###			      struct fs_header_v1_0 *fsh, const char *type,
//###			      const char *descr, uint woffset, uint flags);

/* Show status after handling a subimage */
void fs_image_show_sub_status(int err);

/* Show status after saving an image and return CMD_RET code */
//###int fs_image_show_save_status(int failed, const char *type);

int fs_image_confirm(void);

/* Determine first copy to modify depending on which SPL copy we booted */
int fs_image_get_start_copy(void);

/* Check boot device; Return 0: OK, 1: Not fused yet, <0: Error */
int fs_image_check_boot_dev_fuses(enum boot_device boot_dev, const char *action);

/* Check CRC32 from image and all sub-images */
int fs_image_check_all_crc32(struct fs_header_v1_0 *fsh);

/* Validate an image, either check signature or CRC32; 0: OK, <0: Error */
//###int fs_image_validate(struct fs_header_v1_0 *fsh, const char *type,
//###		      const char *descr, ulong addr);

/* Get image length any header (F&S header, IVT or FIT header) */
int fs_image_get_size_from_header(struct flash_info *fi, uint offs, uint lim,
				  struct sub_info *sub, uint *size);

/* Invalidate the temp buffer read cache */
void fs_image_drop_temp(struct flash_info *fi);

int fs_image_load_sub(struct flash_info *fi, uint offs, uint size, uint lim,
		      uint flags, u8 *buf);

#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
void fs_image_set_spl_secondary_bit(void *img, int copy);
#endif

int fs_image_load_image(struct flash_info *fi, const struct storage_info *si,
			struct sub_info *sub);

/* Load the F&S header of ATF in the ATF region, return 0 if ATF, 1 if U-ATF */
//###bool fs_image_is_u_atf(struct flash_info *fi, const struct storage_info *atf_si);

/* Check CRC32 for an environment of given size */
int fs_image_check_env_crc32(void *env, uint size);

/* Load one ENV */
//###int fs_image_load_env(struct flash_info *fi, struct storage_info *si,
//###		      void *env_addr, int copy);

/*
 * Load U-Boot to given address. If SUB_HAS_FS_HEADER is not set as sub_flags,
 * then fs_image_load_image() will create a new one. If the image actually has
 * a header in this case (new U-BOOT versions are stored with header), it is
 * used for CRC32 checking, then removed, and the own new header is used
 * instead.
 */
//###int fs_image_load_uboot(struct flash_info *fi, struct nboot_info *ni,
//###			void *addr, uint sub_flags);

/* Flush the temp buffer to flash */
//###int fs_image_flush_temp(struct flash_info *fi, uint lim, uint flags);

/* Save the given region to flash */
int fs_image_save_region(struct flash_info *fi, int copy,
			 struct region_info *ri);

int fs_image_get_flash_nand(struct flash_info *fi, int devnum, bool rw);
int fs_image_get_known_env_nand(uint index, uint start[2], uint *size);

int fs_image_get_flash_mmc(struct flash_info *fi, int devnum, bool rw);
int fs_image_get_known_env_mmc(uint index, uint start[2], uint *size);


/* ------------- Command implementation ------------------------------------ */

/* Show the F&S architecture */
int fs_image_do_arch(int argc, char * const argv[]);

/* Show the current BOARD-ID */
int fs_image_do_boardid(int argc, char * const argv[]);

/* Print FDT content of current BOARD-CFG */
int fs_image_do_boardcfg(int argc, char * const argv[]);

/* Show current boot settings */
int fs_image_do_boot(int argc, char * const argv[]);

/* List contents of an F&S image */
int fs_image_do_list(int argc, char * const argv[]);

/* Load NBOOT and SPL regions from the boot device (NAND or MMC) to DRAM,
   create minimal NBoot image that could be saved again */
int fs_image_do_load(int argc, char * const argv[]);

/* Save the F&S NBoot image to the boot device (NAND or MMC) */
int fs_image_do_save(int argc, char * const argv[]);

/* Burn the fuses according to the NBoot in DRAM */
int fs_image_do_fuse(int argc, char * const argv[]);

/* Load DRAM timings from the boot device (NAND or MMC) to DRAM,
   look for the CRC and print it out */
int fs_image_do_checksum(int argc, char * const argv[]);

#endif /* !CONFIG_SPL_BUILD */

/* ------------- Stuff only for SPL ---------------------------------------- */

#ifdef CONFIG_SPL_BUILD
#if !defined(CONFIG_FS_CNTR_COMMON)
typedef void (*basic_init_t)(const char *layout_name);

/* Mark BOARD_CFG to tell U-Boot that we are running on Secondary SPL */
void fs_image_mark_secondary(void);

/* Mark BOARD_CFG to tell U-Boot that we are running on Secondary UBoot */
void fs_image_mark_secondary_uboot(void);

/* Load FIRMWARE and optionally BOARD-CFG via SDP from USB */
void fs_image_all_sdp(bool need_cfg, basic_init_t basic_init);

/* Load BOARD-CFG and optionally FIRMWARE from NAND or MMC */
int fs_image_load_system(enum boot_device boot_dev, bool secondary,
			 basic_init_t basic_init);

/* Load F&S image with given type/descr from NAND at offset to given buffer */
int fs_image_load_nand(unsigned int offset, char *type, char *descr,
		       void *buf, bool keep_header);

/* Load FIRMWARE from NAND */
unsigned int fs_image_fw_nand(unsigned int jobs_todo, basic_init_t basic_init);

/* Load BOARD-CFG from NAND */
int fs_image_cfg_nand(void);

/* Load F&S image with given type/descr from MMC at offset to given buffer */
int fs_image_load_mmc(unsigned int offset, char *type, char *descr,
		       void *buf, bool keep_header);

/* Load FIRMWARE from eMMC */
unsigned int fs_image_fw_mmc(unsigned int jobs_todo, basic_init_t basic_init);

/* Load BOARD-CFG from eMMC */
int fs_image_cfg_mmc(void);
#endif /* ! CONFIG_FS_CNTR_COMMON */
#endif /* CONFIG_SPL_BUILD */

#endif /* !__FS_IMAGE_COMMON_H__ */
