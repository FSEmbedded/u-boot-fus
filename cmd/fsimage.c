// SPDX-License-Identifier:	GPL-2.0+
/*
 * (C) Copyright 2021 F&S Elektronik Systeme GmbH
 * Hartmut Keller <keller@fs-net.de>
 *
 * Provide CLI commands to handle F&S nboot.fs and uboot.fs images.
 *
 */

#include <common.h>
#include <command.h>

#include "../board/F+S/common/fs_image_common.h"	/* fs_image_*() */

/* ------------- Command implementation ------------------------------------ */

#ifdef __UBOOT__
/* Show the F&S architecture */
static int do_fsimage_arch(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_arch(argc, argv);
}

/* Show the current BOARD-ID */
static int do_fsimage_boardid(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	return fs_image_do_boardid(argc, argv);
}

#ifdef CONFIG_CMD_FDT
/* Print FDT content of current BOARD-CFG */
static int do_fsimage_boardcfg(struct cmd_tbl *cmdtp, int flag, int argc,
			       char * const argv[])
{
	return fs_image_do_boardcfg(argc, argv);
}
#endif
#endif /*__UBOOT__*/

/* Show current boot settings */
static int do_fsimage_boot(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_boot(argc, argv);
}

/* List contents of an F&S image */
static int do_fsimage_list(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_list(argc, argv);
}

/* Load NBOOT and SPL regions from the boot device (NAND or MMC) to DRAM,
   create minimal NBoot image that could be saved again */
static int do_fsimage_load(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_load(argc, argv);
}

/* Save the F&S NBoot image to the boot device (NAND or MMC) */
static int do_fsimage_save(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_save(argc, argv);
}

/* Burn the fuses according to the NBoot in DRAM */
static int do_fsimage_fuse(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_fuse(argc, argv);
}

/* Load DRAM timings from the boot device (NAND or MMC) to DRAM,
   look for the CRC and print it out */
static int do_fsimage_checksum(struct cmd_tbl *cmdtp, int flag, int argc,
			   char * const argv[])
{
	return fs_image_do_checksum(argc, argv);
}


/* Subcommands for "fsimage" */
static struct cmd_tbl cmd_fsimage_sub[] = {
	U_BOOT_CMD_MKENT(arch, 0, 1, do_fsimage_arch, "", ""),
	U_BOOT_CMD_MKENT(board-id, 0, 1, do_fsimage_boardid, "", ""),
#ifdef CONFIG_CMD_FDT
	U_BOOT_CMD_MKENT(board-cfg, 4, 1, do_fsimage_boardcfg, "", ""),
#endif
	U_BOOT_CMD_MKENT(boot, 0, 1, do_fsimage_boot, "", ""),
	U_BOOT_CMD_MKENT(list, 4, 1, do_fsimage_list, "", ""),
	U_BOOT_CMD_MKENT(load, 5, 1, do_fsimage_load, "", ""),
	U_BOOT_CMD_MKENT(save, 8, 0, do_fsimage_save, "", ""),
	U_BOOT_CMD_MKENT(fuse, 6, 0, do_fsimage_fuse, "", ""),
	U_BOOT_CMD_MKENT(checksum, 6, 1, do_fsimage_checksum, "", ""),
};

static int do_fsimage(struct cmd_tbl *cmdtp, int flag, int argc,
		      char * const argv[])
{
	struct cmd_tbl *cp;
	void *found_cfg;
	void *expected_cfg;

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Drop argv[0] ("fsimage") */
	argc--;
	argv++;

	cp = find_cmd_tbl(argv[0], cmd_fsimage_sub,
			  ARRAY_SIZE(cmd_fsimage_sub));
	if (!cp)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp))
		return CMD_RET_SUCCESS;

	/*
	 * All fsimage commands will access the BOARD-CFG in OCRAM. Make sure
	 * it is still valid and not compromised in any way.
	 */
	if (!fs_image_is_ocram_cfg_valid()) {
		printf("Error: BOARD-CFG in OCRAM at 0x%lx damaged\n",
		       (ulong)fs_image_get_cfg_addr());
		return CMD_RET_FAILURE;
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

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(fsimage, 9, 1, do_fsimage,
	   "Handle F&S board configuration and F&S images, e.g. U-Boot, NBOOT",
	   "arch\n"
	   "    - Show F&S architecture\n"
	   "fsimage board-id\n"
	   "    - Show current BOARD-ID\n"
#ifdef CONFIG_CMD_FDT
	   "fsimage board-cfg [<addr> | stored]\n"
	   "    - List contents of current BOARD-CFG\n"
#endif
	   "fsimage boot\n"
	   "    - Show the current boot settings\n"
	   "fsimage list [<addr>]\n"
	   "    - List the content of the F&S image at <addr>\n"
	   "fsimage load [-f] [uboot | nboot] [<addr>]\n"
	   "    - Verify the current NBoot or U-Boot and load to <addr>\n"
	   "fsimage save [-f] [-e <n>] [-b <n>]"
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	   " [-s]"
#endif
	   " [<addr>]\n"
	   "    - Save the F&S image at the right place (NBoot, U-Boot)\n"
	   "fsimage fuse [-f] [<addr> | stored]\n"
	   "    - Program fuses according to the current BOARD-CFG.\n"
	   "      WARNING: This is a one time option and cannot be undone.\n"
	   "fsimage checksum [-t <type>] [<addr> | stored]\n"
	   "    - Print the checksum of all headers or <type> if specified.\n"
	   "      The NBoot first needs to be loaded with \"fsimage load\".\n"
	   "\n"
	   "If no addr is given, use loadaddr. Using -f forces the command to\n"
	   "continue without showing any confirmation queries. This is meant\n"
	   "for non-interactive installation procedures. Option -b also sets\n"
	   "the eMMC hwpart to boot from: 0: User, 1: Boot1, 2: Boot2. This\n"
	   "option is ignored on NAND. Option -e supports handling early\n"
	   "NBoot versions. If the environment is not found when updating\n"
	   "from a pre 2023.08 NBoot version, try increasing <n> until it\n"
	   "works. Be careful when storing such an old NBoot, you need to\n"
	   "know the right <n> or you will lose the environment.\n"
#if !CONFIG_IS_ENABLED(FS_CNTR_COMMON)
	   "\nIf a User-ATF is present in an U-Boot image, this replaces the\n"
	   "System-ATF from NBoot. From then on, ATF is ignored when saving\n"
	   "new NBoot images and has to be handled by U-Boot updates. With\n"
	   "option -s, this behavior can be reversed to prefer the System-ATF\n"
	   "again. Which means when saving U-Boot, any User-ATF in the image\n"
	   "is ignored, and when saving NBoot, the System-ATF included there\n"
	   "is saved again. (Remark: opTee, if present, must be grouped with\n"
	   "ATF and is handled with ATF in one go.)\n"
#endif
);
