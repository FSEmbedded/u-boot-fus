// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * ZFS filesystem porting to Uboot by
 * Jorgen Lundman <lundman at lundman.net>
 *
 * zfsfs support
 * made from existing GRUB Sources by Sun, GNU and others.
 */

#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <env.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <zfs_common.h>
#include <linux/stat.h>
#include <malloc.h>

#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
#include <usb.h>
#endif

#if !CONFIG_IS_ENABLED(DOS_PARTITION) && !CONFIG_IS_ENABLED(EFI_PARTITION)
#error DOS or EFI partition support must be selected
#endif

#define DOS_PART_MAGIC_OFFSET	0x1fe
#define DOS_FS_TYPE_OFFSET	0x36
#define DOS_FS32_TYPE_OFFSET	0x52

static int do_zfs_load(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	char *filename = NULL;
	int dev;
	int part;
	ulong addr = 0;
	struct disk_partition info;
	struct blk_desc *dev_desc;
	unsigned long count;
	const char *addr_str;
	struct zfs_file zfile;
	struct device_s vdev;

	if ((argc < 3) || (argc > 6))
		return CMD_RET_USAGE;

	addr = (argc > 3) ? parse_loadaddr(argv[3], NULL) : get_loadaddr();
	filename = (argc > 4) ? env_parse_bootfile(argv[4]) : env_get_bootfile();
	count = (argc > 5) ? hextoul(argv[5], NULL) : 0;
	pos = (argc > 6) ? hextoul(argv[6], NULL) : 0;
	set_fileaddr(addr);

	part = blk_get_device_part_str(argv[1], argv[2], &dev_desc, &info, 1);
	if (part < 0)
		return 1;

	dev = dev_desc->devnum;
	printf("Loading file \"%s\" from %s device %d%c%c\n",
		filename, argv[1], dev,
		part ? ':' : ' ', part ? part + '0' : ' ');

	zfs_set_blk_dev(dev_desc, &info);
	vdev.part_length = info.size;

	memset(&zfile, 0, sizeof(zfile));
	zfile.device = &vdev;
	if (zfs_open(&zfile, filename)) {
		printf("** File not found %s **\n", filename);
		return 1;
	}

	if ((count < zfile.size) && (count != 0))
		zfile.size = (uint64_t)count;

	if (zfs_read(&zfile, (char *)addr, zfile.size) != zfile.size) {
		printf("** Unable to read \"%s\" from %s %d:%d **\n",
			   filename, argv[1], dev, part);
		zfs_close(&zfile);
		return 1;
	}

	zfs_close(&zfile);

	printf("%llu bytes read\n", zfile.size);
	env_set_fileinfo(zfile.size);

	return 0;
}


int zfs_print(const char *entry, const struct zfs_dirhook_info *data)
{
	printf("%s %s\n",
		   data->dir ? "<DIR> " : "		 ",
		   entry);
	return 0; /* 0 continue, 1 stop */
}


static int do_zfs_ls(struct cmd_tbl *cmdtp, int flag, int argc,
		     char *const argv[])
{
	const char *filename = "/";
	int part;
	struct blk_desc *dev_desc;
	struct disk_partition info;
	struct device_s vdev;

	if (argc < 2)
		return cmd_usage(cmdtp);

	if (argc == 4)
		filename = argv[3];

	part = blk_get_device_part_str(argv[1], argv[2], &dev_desc, &info, 1);
	if (part < 0)
		return 1;

	zfs_set_blk_dev(dev_desc, &info);
	vdev.part_length = info.size;

	zfs_ls(&vdev, filename,
		   zfs_print);

	return 0;
}


U_BOOT_CMD(zfsls, 4, 1, do_zfs_ls,
		   "list files in a directory (default /)",
		   "<interface> <dev[:part]> [directory]\n"
		   "	  - list files from 'dev' on 'interface' in a '/DATASET/@/$dir/'");

U_BOOT_CMD(zfsload, 6, 0, do_zfs_load,
		   "load binary file from a ZFS filesystem",
		   "<interface> <dev[:part]> [addr] [filename] [bytes]\n"
		   "	  - load binary file '/DATASET/@/$dir/$file' from 'dev' on 'interface'\n"
		   "		 to address 'addr' from ZFS filesystem");
