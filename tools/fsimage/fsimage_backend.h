#ifndef FSLIB_H
#define FSLIB_H
#ifdef __UBOOT__
#include <common.h>
#include <fslib_common.h>
#include <fslib_cntr_common.h>
#include <fslib_board_common.h>
#include <fdt_support.h>
#else
#include "../../board/F+S/common/fs_image_common.h"
#include "../../board/F+S/common/fs_cntr_common.h"
#include "../../board/F+S/common/fs_board_common.h"
#include "../../include/linux/libfdt.h"
#define KILO 1024
#define MEGA KILO*KILO
#define BUFFER_SIZE = 4*MEGA

extern char saved_nboot_buffer[1024*1024*4];
extern char nboot_buffer[1024*1024*4];
extern char saved_board_cfg_buffer[4*1024*1024];

extern int current_boot_part;
#endif

struct index_info {
	uint offset;		// offset after fsh_entry to blob
	struct fs_header_v1_0 *fsh_idx; // header of INDEX image
	struct fs_header_v1_0 *fsh_idx_entry; // header within INDEX image
};

/* Structure to hold regions in NAND/eMMC for an image, taken from nboot-info */
struct storage_info {
	uint start[2];		/* -start entries */
	uint size;		/* -size entry */
#if __UBOOT__
#ifdef CONFIG_CMD_MMC
	u8 hwpart[2];			/* hwpart (in case of eMMC) */
#endif
#else
	u8 hwpart[2];			/* hwpart (in case of eMMC) */
#endif
	const char *type;		/* Name of storage region */
};

struct sub_info {
	void *img;			/* Pointer to image */
	const char *type;		/* "BOARD-CFG", "FIRMWARE", "SPL" */
	const char *descr;		/* e.g. board architecture */
	uint size;		/* Size of image */
	uint offset;		/* Offset of image within si */
	uint flags;		/* See SUB_* above */
};

struct region_info {
	struct storage_info *si;	/* Region information */
	struct sub_info *sub;		/* Pointer to subimages */
	int count;			/* Number of subimages */
};

#ifndef __UBOOT__
#define BIT(n) (1 << (n))
#endif

#define SUB_SYNC          BIT(0)	/* After writing image, flush temp */
#define SUB_HAS_FS_HEADER BIT(1)	/* Image has an F&S header in flash */
#define SUB_IS_SPL        BIT(2)	/* SPL: has IVT, may beed offset */
#define SUB_IS_ENV        BIT(3)	/* Environment data */
#ifdef CONFIG_NAND_MXS
#define SUB_IS_FCB        BIT(4)	/* FCB: needs other ECC */
#define SUB_IS_DBBT       BIT(5)
#define SUB_IS_DBBT_DATA  BIT(6)
#endif

/* Storage info from the nboot-info of a BOARD-CFG in binary form */
#define NI_SUPPORT_CRC32       BIT(0)	/* Support CRC32 in F&S headers */
#define NI_SAVE_BOARD_ID       BIT(1)	/* Save the board-rev. in BOARD-CFG */
#define NI_UBOOT_WITH_FSH      BIT(2)	/* Save U-Boot with F&S Header */
#define NI_UBOOT_EMMC_BOOTPART BIT(3)	/* On eMMC when booting from boot part,
					   also save U-Boot in boot part */
#define NI_EMMC_BOTH_BOOTPARTS BIT(4)	/* On eMMC when booting from boot part,
					   use both boot partitions, one for
					   each copy */

#define MAX_SUB_IMGS	8 /* Max Array-Size for Sub-Images */

struct nboot_info {
	uint flags;		/* See NI_* above */
	uint board_cfg_size;
	struct storage_info spl;
	struct storage_info nboot;
	struct storage_info uboot;
	struct storage_info env;
};

/* Access functions that differ between NAND and MMC */
struct flash_info;
struct flash_ops {
	bool (*check_for_uboot)(struct storage_info *si, bool force);
	bool (*check_for_nboot)(struct flash_info *fi, struct storage_info *si,
				bool force);
	int (*get_nboot_info)(struct flash_info *fi, void *fdt, int offs,
			      struct nboot_info *ni, int hwpart, bool show);
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
			  struct region_info *spl_ri);
	int (*set_boot_hwpart)(struct flash_info *fi, int boot_hwpart);
	void (*get_flash)(struct flash_info *fi);
	void (*put_flash)(struct flash_info *fi);
};

struct flash_info {
#if __UBOOT__
#ifdef CONFIG_NAND_MXS
	struct mtd_info *mtd;		/* Handle to NAND */
	uint env_used;		/* From env-size entry, region size
					   is from env-range */
#endif
#ifdef CONFIG_CMD_MMC
	struct udevice *bdev;		/* blkdev driver instance */
	u8 boot_hwpart;			/* HW partition we boot from (0..2) */
	u8 old_hwpart;			/* Previous partition before command */
#endif
#else
	u8 boot_hwpart;			/* HW partition we boot from (0..2) */
	u8 old_hwpart;			/* Previous partition before command */
#endif
	char devname[6];		/* Name of device (NAND, mmc<n>) */
	u8 *temp;			/* Buffer for one NAND page/MMC block */
	uint temp_size;		/* Size of temp buffer */
	uint base_offs;		/* Offset where temp will be written */
	uint write_pos;		/* temp contains data up to this pos */
	uint bb_extra_offs;	/* Extra offset due to bad blocks */
	u8 temp_fill;			/* Default value for temp buffer */
	enum boot_device boot_dev;	/* Device to boot from */
	const char *boot_dev_name;	/* Boot device as string */
	struct flash_ops *ops;		/* Access functions for NAND/MMC */
};

/* Struct that is big enough for all headers that we may want to load */
union any_header {
	struct fs_header_v1_0 fsh;
#if __UBOOT__
#if CONFIG_IS_ENABLED(IMX_HAB)
	u8 ivt[HAB_HEADER];
#endif
#endif
	struct fdt_header fdt;
};

/* Argument of option -e in fsimage save */
static uint early_support_index;

enum parse_type {
	PARSE_CONTENT,
	PARSE_CHECKSUM,
};

struct fs_header_v1_0 *fs_image_find(struct fs_header_v1_0 *fsh,
		const char *type,
		const char *descr,
		struct index_info *idx_info);
void fs_image_print_crc(struct fs_header_v1_0 *fsh_parent, struct fs_header_v1_0 *fsh, uint offs, int level);
void fs_image_print_line(struct fs_header_v1_0 *fsh, uint offs, int level);
void fs_image_parse_image(enum parse_type ptype, ulong addr, uint offs, int level);
void fs_image_parse_index_image(enum parse_type ptype,
		struct fs_header_v1_0 *fsh_parent, ulong addr, uint offs,
		int level, uint remaining);
void fs_image_parse_subimage(enum parse_type ptype,
		ulong addr, uint offs, int level, uint remaining);
void fs_image_print_line(struct fs_header_v1_0 *fsh, uint offs, int level);

int fs_image_check_saved_cfg(void);

int do_list(int argc, char* const argv[]);
int do_save_nboot_uboot(int argc, char* const argv[]);
int do_load_from_system(int argc, char* const argv[]) ;

#endif