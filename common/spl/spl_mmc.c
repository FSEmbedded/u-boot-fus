// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Aneesh V <aneesh@ti.com>
 */
#include <common.h>
#include <dm.h>
#include <log.h>
#include <part.h>
#include <spl.h>
#include <linux/compiler.h>
#include <errno.h>
#include <asm/u-boot.h>
#include <errno.h>
#include <mmc.h>
#include <image.h>

#ifdef CONFIG_FS_BOARD_CFG
#include "../../board/F+S/common/fs_image_common.h"
#endif

static int mmc_load_legacy(struct spl_image_info *spl_image,
			   struct spl_boot_device *bootdev,
			   struct mmc *mmc,
			   ulong sector, struct legacy_img_hdr *header)
{
	u32 image_offset_sectors;
	u32 image_size_sectors;
	unsigned long count;
	u32 image_offset;
	int ret;

	ret = spl_parse_image_header(spl_image, bootdev, header);
	if (ret)
		return ret;

	/* convert offset to sectors - round down */
	image_offset_sectors = spl_image->offset / mmc->read_bl_len;
	/* calculate remaining offset */
	image_offset = spl_image->offset % mmc->read_bl_len;

	/* convert size to sectors - round up */
	image_size_sectors = (spl_image->size + mmc->read_bl_len - 1) /
			     mmc->read_bl_len;

	/* Read the header too to avoid extra memcpy */
	count = blk_dread(mmc_get_blk_desc(mmc),
			  sector + image_offset_sectors,
			  image_size_sectors,
			  (void *)(ulong)spl_image->load_addr);
	debug("read %x sectors to %lx\n", image_size_sectors,
	      spl_image->load_addr);
	if (count != image_size_sectors)
		return -EIO;

	if (image_offset)
		memmove((void *)(ulong)spl_image->load_addr,
			(void *)(ulong)spl_image->load_addr + image_offset,
			spl_image->size);

	return 0;
}

ulong h_spl_load_read(struct spl_load_info *load, ulong sector,
			     ulong count, void *buf)
{
	struct mmc *mmc = load->dev;

	return blk_dread(mmc_get_blk_desc(mmc), sector, count, buf);
}

static __maybe_unused unsigned long spl_mmc_raw_uboot_offset(int part)
{
#if IS_ENABLED(CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR)
	if (part == 0)
		return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_DATA_PART_OFFSET;
#endif

	return 0;
}

#if defined(CONFIG_DUAL_BOOTLOADER)
int mmc_load_image_raw_sector_dual_uboot(struct spl_image_info *spl_image,
					 struct mmc *mmc);
#endif

int __weak mmc_image_load_late(struct spl_image_info *spl_image, struct mmc *mmc)
{
	return 0;
}

static __maybe_unused
int mmc_load_image_raw_sector(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev,
			      struct mmc *mmc, unsigned long sector)
{
	unsigned long count;
	struct legacy_img_hdr *header;
	struct blk_desc *bd = mmc_get_blk_desc(mmc);
	int ret = 0;
	int extra_offset = 0;

	header = spl_get_load_buffer(-sizeof(*header), bd->blksz);

	/* read image header to find the image size & load address */
	count = blk_dread(bd, sector, 1, header);

	debug("hdr read sector %lx, count=%lu\n", sector, count);
	if (count == 0) {
		ret = -EIO;
		goto end;
	}

#ifdef CONFIG_FS_BOARD_CFG
	/* Allow U-Boot image to be prepended with F&S header */
	extra_offset = spl_check_fs_header(header);
	if (extra_offset < 0)
		return -1;

	/* In case of signed U-Boot, load U-Boot image completely */
	if ((extra_offset > 0) && fs_image_is_signed((void *)header)) {
		u32 size;
		void *addr = fs_image_get_ivt_info((void *)header, &size);

		if (addr && size) {
			u32 blocks = (size + bd->blksz - 1) / bd->blksz;
			count = blk_dread(bd, sector, blocks, addr);
			if (count != blocks) {
				ret = -EIO;
				goto end;
			}
		}
		return secure_spl_load_simple_fit(spl_image, addr, size);
	}

	header = (void *)header + extra_offset;
#endif

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
		memset(&load, 0, sizeof(load));
		load.dev = mmc;
		load.bl_len = mmc->read_bl_len;
		load.read = h_spl_load_read;
		load.extra_offset = extra_offset;
		ret = spl_load_simple_fit(spl_image, &load, sector, header);
	} else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER)) {
		struct spl_load_info load;

		memset(&load, 0, sizeof(load));
		load.dev = mmc;
		load.bl_len = mmc->read_bl_len;
		load.read = h_spl_load_read;

		ret = spl_load_imx_container(spl_image, &load, sector);
	} else {
		ret = mmc_load_legacy(spl_image, bootdev, mmc, sector, header);
	}

end:
	if (ret) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		puts("mmc_load_image_raw_sector: mmc block read error\n");
#endif
		return -1;
	}

	ret = mmc_image_load_late(spl_image, mmc);
	return ret;
}

static int spl_mmc_get_device_index(u32 boot_device)
{
	switch (boot_device) {
	case BOOT_DEVICE_MMC1:
		return 0;
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		return 1;
	}

#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	printf("spl: unsupported mmc boot device.\n");
#endif

	return -ENODEV;
}

static int spl_mmc_find_device(struct mmc **mmcp, u32 boot_device)
{
	int err, mmc_dev;

	mmc_dev = spl_mmc_get_device_index(boot_device);
	if (mmc_dev < 0)
		return mmc_dev;

#if CONFIG_IS_ENABLED(DM_MMC)
	err = mmc_init_device(mmc_dev);
#else
	err = mmc_initialize(NULL);
#endif /* DM_MMC */
	if (err) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		printf("spl: could not initialize mmc. error: %d\n", err);
#endif
		return err;
	}
	*mmcp = find_mmc_device(mmc_dev);
	err = *mmcp ? 0 : -ENODEV;
	if (err) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		printf("spl: could not find mmc device %d. error: %d\n",
		       mmc_dev, err);
#endif
		return err;
	}

	return 0;
}

#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_PARTITION
static int mmc_load_image_raw_partition(struct spl_image_info *spl_image,
					struct spl_boot_device *bootdev,
					struct mmc *mmc, int partition,
					unsigned long sector)
{
	struct disk_partition info;
	int err;

#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_PARTITION_TYPE
	int type_part;
	/* Only support MBR so DOS_ENTRY_NUMBERS */
	for (type_part = 1; type_part <= DOS_ENTRY_NUMBERS; type_part++) {
		err = part_get_info(mmc_get_blk_desc(mmc), type_part, &info);
		if (err)
			continue;
		if (info.sys_ind ==
			CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_PARTITION_TYPE) {
			partition = type_part;
			break;
		}
	}
#endif

	err = part_get_info(mmc_get_blk_desc(mmc), partition, &info);
	if (err) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		puts("spl: partition error\n");
#endif
		return -1;
	}

#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
	return mmc_load_image_raw_sector(spl_image, bootdev, mmc, info.start + sector);
#else
	return mmc_load_image_raw_sector(spl_image, bootdev, mmc, info.start);
#endif
}
#endif

#if CONFIG_IS_ENABLED(FALCON_BOOT_MMCSD)
static int mmc_load_image_raw_os(struct spl_image_info *spl_image,
				 struct spl_boot_device *bootdev,
				 struct mmc *mmc)
{
	int ret;

#if CONFIG_VAL(SYS_MMCSD_RAW_MODE_ARGS_SECTOR)
	unsigned long count;

	count = blk_dread(mmc_get_blk_desc(mmc),
		CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTOR,
		CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTORS,
		(void *) CONFIG_SYS_SPL_ARGS_ADDR);
	if (count != CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTORS) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		puts("mmc_load_image_raw_os: mmc block read error\n");
#endif
		return -1;
	}
#endif	/* CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTOR */

	ret = mmc_load_image_raw_sector(spl_image, bootdev, mmc,
		CONFIG_SYS_MMCSD_RAW_MODE_KERNEL_SECTOR);
	if (ret)
		return ret;

	if (spl_image->os != IH_OS_LINUX && spl_image->os != IH_OS_TEE) {
		puts("Expected image is not found. Trying to start U-boot\n");
		return -ENOENT;
	}

	return 0;
}
#else
static int mmc_load_image_raw_os(struct spl_image_info *spl_image,
				 struct spl_boot_device *bootdev,
				 struct mmc *mmc)
{
	return -ENOSYS;
}
#endif

#ifndef CONFIG_SPL_OS_BOOT
int spl_start_uboot(void)
{
	return 1;
}
#endif

#ifdef CONFIG_SYS_MMCSD_FS_BOOT_PARTITION
static int spl_mmc_do_fs_boot(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev,
			      struct mmc *mmc,
			      const char *filename)
{
	int err = -ENOSYS;

	__maybe_unused int partition = CONFIG_SYS_MMCSD_FS_BOOT_PARTITION;

#if CONFIG_SYS_MMCSD_FS_BOOT_PARTITION == -1
	{
		struct disk_partition info;
		debug("Checking for the first MBR bootable partition\n");
		for (int type_part = 1; type_part <= DOS_ENTRY_NUMBERS; type_part++) {
			err = part_get_info(mmc_get_blk_desc(mmc), type_part, &info);
			if (err)
				continue;
			debug("Partition %d is of type %d and bootable=%d\n", type_part, info.sys_ind, info.bootable);
			if (info.bootable != 0) {
				debug("Partition %d is bootable, using it\n", type_part);
				partition = type_part;
				break;
			}
		}
		printf("Using first bootable partition: %d\n", partition);
		if (partition == CONFIG_SYS_MMCSD_FS_BOOT_PARTITION) {
			return -ENOSYS;
		}
	}
#endif

#ifdef CONFIG_SPL_FS_FAT
	if (!spl_start_uboot()) {
		err = spl_load_image_fat_os(spl_image, bootdev, mmc_get_blk_desc(mmc),
			partition);
		if (!err)
			return err;
	}
#ifdef CONFIG_SPL_FS_LOAD_PAYLOAD_NAME
	err = spl_load_image_fat(spl_image, bootdev, mmc_get_blk_desc(mmc),
				 partition,
				 filename);
	if (!err)
		return err;
#endif
#endif
#ifdef CONFIG_SPL_FS_EXT4
	if (!spl_start_uboot()) {
		err = spl_load_image_ext_os(spl_image, bootdev, mmc_get_blk_desc(mmc),
			partition);
		if (!err)
			return err;
	}
#ifdef CONFIG_SPL_FS_LOAD_PAYLOAD_NAME
	err = spl_load_image_ext(spl_image, bootdev, mmc_get_blk_desc(mmc),
				 partition,
				 filename);
	if (!err)
		return err;
#endif
#endif

#if defined(CONFIG_SPL_FS_FAT) || defined(CONFIG_SPL_FS_EXT4)
	err = -ENOENT;
#endif

	return err;
}
#else
static int spl_mmc_do_fs_boot(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev,
			      struct mmc *mmc,
			      const char *filename)
{
	return -ENOSYS;
}
#endif

u32 __weak spl_mmc_boot_mode(struct mmc *mmc, const u32 boot_device)
{
#if defined(CONFIG_SPL_FS_FAT) || defined(CONFIG_SPL_FS_EXT4)
	return MMCSD_MODE_FS;
#elif defined(CONFIG_SUPPORT_EMMC_BOOT)
	return MMCSD_MODE_EMMCBOOT;
#else
	return MMCSD_MODE_RAW;
#endif
}

#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_PARTITION
int __weak spl_mmc_boot_partition(const u32 boot_device)
{
	return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_PARTITION;
}
#endif

unsigned long __weak spl_mmc_get_uboot_raw_sector(struct mmc *mmc,
						  unsigned long raw_sect)
{
	return raw_sect;
}

int default_spl_mmc_emmc_boot_partition(struct mmc *mmc)
{
	int part;
#ifdef CONFIG_SYS_MMCSD_RAW_MODE_EMMC_BOOT_PARTITION
	part = CONFIG_SYS_MMCSD_RAW_MODE_EMMC_BOOT_PARTITION;
#else
	/*
	 * We need to check what the partition is configured to.
	 * 1 and 2 match up to boot0 / boot1 and 7 is user data
	 * which is the first physical partition (0).
	 */
#ifdef CONFIG_DUAL_BOOTLOADER
		/* Bootloader is stored in eMMC user partition for
		 * dual bootloader.
		 */
		part = 0;
#else
	part = (mmc->part_config >> 3) & PART_ACCESS_MASK;
	if (part == 7)
		part = 0;
#endif
#endif

	return part;
}

int __weak arch_spl_mmc_emmc_boot_partition(struct mmc *mmc)
{
	return default_spl_mmc_emmc_boot_partition(mmc);
}

int __weak spl_mmc_emmc_boot_partition(struct mmc *mmc)
{
	return arch_spl_mmc_emmc_boot_partition(mmc);
}

static int spl_mmc_get_mmc_devnum(struct mmc *mmc)
{
	struct blk_desc *block_dev;
#if !CONFIG_IS_ENABLED(BLK)
	block_dev = &mmc->block_dev;
#else
	block_dev = dev_get_uclass_plat(mmc->dev);
#endif
	return block_dev->devnum;
}

int spl_mmc_load(struct spl_image_info *spl_image,
		 struct spl_boot_device *bootdev,
		 const char *filename,
		 int raw_part,
		 unsigned long raw_sect)
{
	static struct mmc *mmc;
	u32 boot_mode;
	int err = 0;
	__maybe_unused int part = 0;
	int mmc_dev;

	/* Perform peripheral init only once for an mmc device */
	mmc_dev = spl_mmc_get_device_index(bootdev->boot_device);
	if (!mmc || spl_mmc_get_mmc_devnum(mmc) != mmc_dev) {
		err = spl_mmc_find_device(&mmc, bootdev->boot_device);
		if (err)
			return err;

		err = mmc_init(mmc);
		if (err) {
			mmc = NULL;
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
			printf("spl: mmc init failed with error: %d\n", err);
#endif
			return err;
		}
	}

	boot_mode = spl_mmc_boot_mode(mmc, bootdev->boot_device);
	err = -EINVAL;
	switch (boot_mode) {
	case MMCSD_MODE_EMMCBOOT:
		part = spl_mmc_emmc_boot_partition(mmc);

		if (CONFIG_IS_ENABLED(MMC_TINY))
			err = mmc_switch_part(mmc, part);
		else
			err = blk_dselect_hwpart(mmc_get_blk_desc(mmc), part);

		if (err) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
			puts("spl: mmc partition switch failed\n");
#endif
			return err;
		}
		/* Fall through */
	case MMCSD_MODE_RAW:
		debug("spl: mmc boot mode: raw\n");

		if (!spl_start_uboot()) {
			err = mmc_load_image_raw_os(spl_image, bootdev, mmc);
			if (!err)
				return err;
		}

#ifndef CONFIG_DUAL_BOOTLOADER
		raw_sect = spl_mmc_get_uboot_raw_sector(mmc, raw_sect);
#endif

#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_PARTITION
		err = mmc_load_image_raw_partition(spl_image, bootdev,
						   mmc, raw_part,
						   raw_sect);
		if (!err)
			return err;
#endif
#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#ifdef CONFIG_DUAL_BOOTLOADER
		err = mmc_load_image_raw_sector_dual_uboot(spl_image, mmc);
#else
		err = mmc_load_image_raw_sector(spl_image, bootdev, mmc,
				raw_sect + spl_mmc_raw_uboot_offset(part));
#endif
		if (!err)
			return err;
#endif
		/* If RAW mode fails, try FS mode. */
	case MMCSD_MODE_FS:
		debug("spl: mmc boot mode: fs\n");

		err = spl_mmc_do_fs_boot(spl_image, bootdev, mmc, filename);
		if (!err)
			return err;

		break;
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	default:
		puts("spl: mmc: wrong boot mode\n");
#endif
	}

	return err;
}

int spl_mmc_load_image(struct spl_image_info *spl_image,
		       struct spl_boot_device *bootdev)
{
	return spl_mmc_load(spl_image, bootdev,
#ifdef CONFIG_SPL_FS_LOAD_PAYLOAD_NAME
			    CONFIG_SPL_FS_LOAD_PAYLOAD_NAME,
#else
			    NULL,
#endif
#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_PARTITION
			    spl_mmc_boot_partition(bootdev->boot_device),
#else
			    0,
#endif
#ifdef CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR
			    CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR
#ifdef CONFIG_SECONDARY_BOOT_SECTOR_OFFSET
			    + CONFIG_SECONDARY_BOOT_SECTOR_OFFSET
#endif
			    );
#else
			    0);
#endif
}

int spl_mmc_load_image_redundant(struct spl_image_info *spl_image,
		       struct spl_boot_device *bootdev)
{
	int err;
	err = spl_mmc_load_image(spl_image, bootdev);
	if(!err)
		return err;
	printf("WARNING: Loading UBoot from primary partition failed, try secondary\n");
	printf("         Saving the UBoot again may fix this issue.\n");
#ifdef CONFIG_FS_BOARD_CFG
	fs_image_mark_secondary_uboot();
#endif
	return spl_mmc_load_image(spl_image, bootdev);
}

SPL_LOAD_IMAGE_METHOD("MMC1", 0, BOOT_DEVICE_MMC1, spl_mmc_load_image_redundant);
SPL_LOAD_IMAGE_METHOD("MMC2", 0, BOOT_DEVICE_MMC2, spl_mmc_load_image);
SPL_LOAD_IMAGE_METHOD("MMC2_2", 0, BOOT_DEVICE_MMC2_2, spl_mmc_load_image);
