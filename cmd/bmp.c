// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002
 * Detlev Zundel, DENX Software Engineering, dzu@denx.de.
 */

/*
 * BMP handling routines
 */

#include <common.h>
#include <bmp_layout.h>
#include <command.h>
#include <dm.h>
#include <gzip.h>
#include <image.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <splash.h>
#include <video.h>
#include <video_link.h>
#include <asm/byteorder.h>

static int bmp_info (ulong addr);

/*
 * Allocate and decompress a BMP image using gunzip().
 *
 * Returns a pointer to the decompressed image data. This pointer is
 * aligned to 32-bit-aligned-address + 2.
 * See doc/README.displaying-bmps for explanation.
 *
 * The allocation address is passed to 'alloc_addr' and must be freed
 * by the caller after use.
 *
 * Returns NULL if decompression failed, or if the decompressed data
 * didn't contain a valid BMP signature.
 */
#ifdef CONFIG_VIDEO_BMP_GZIP
struct bmp_image *gunzip_bmp(unsigned long addr, unsigned long *lenp,
			     void **alloc_addr)
{
	void *dst;
	unsigned long len;
	struct bmp_image *bmp;

	/*
	 * Decompress bmp image
	 */
	len = CONFIG_VIDEO_LOGO_MAX_SIZE;
	/* allocate extra 3 bytes for 32-bit-aligned-address + 2 alignment */
	dst = malloc(CONFIG_VIDEO_LOGO_MAX_SIZE + 3);
	if (!dst) {
		puts("Error: malloc in gunzip failed!\n");
		return NULL;
	}

	/* align to 32-bit-aligned-address + 2 */
	bmp = dst + 2;

	if (gunzip(bmp, CONFIG_VIDEO_LOGO_MAX_SIZE, map_sysmem(addr, 0),
		   &len)) {
		free(dst);
		return NULL;
	}
	if (len == CONFIG_VIDEO_LOGO_MAX_SIZE)
		puts("Image could be truncated (increase CONFIG_VIDEO_LOGO_MAX_SIZE)!\n");

	/*
	 * Check for bmp mark 'BM'
	 */
	if (!((bmp->header.signature[0] == 'B') &&
	      (bmp->header.signature[1] == 'M'))) {
		free(dst);
		return NULL;
	}

	debug("Gzipped BMP image detected!\n");

	*alloc_addr = dst;
	return bmp;
}
#else
struct bmp_image *gunzip_bmp(unsigned long addr, unsigned long *lenp,
			     void **alloc_addr)
{
	return NULL;
}
#endif

static int do_bmp_info(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	ulong addr;

	switch (argc) {
	case 1:		/* use default address */
		addr = get_loadaddr();
		break;
	case 2:		/* use argument */
		addr = parse_loadaddr(argv[1], NULL);
		break;
	default:
		return CMD_RET_USAGE;
	}

	return (bmp_info(addr));
}

static int do_bmp_display(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	ulong addr;
	int x = 0, y = 0;

	splash_get_pos(&x, &y);

	switch (argc) {
	case 1:		/* use default address */
		addr = get_loadaddr();
		break;
	case 2:		/* use argument */
		addr = parse_loadaddr(argv[1], NULL);
		break;
	case 4:
		addr = parse_loadaddr(argv[1], NULL);
		if (!strcmp(argv[2], "m"))
			x = BMP_ALIGN_CENTER;
		else
			x = dectoul(argv[2], NULL);
		if (!strcmp(argv[3], "m"))
			y = BMP_ALIGN_CENTER;
		else
			y = dectoul(argv[3], NULL);
		break;
	default:
		return CMD_RET_USAGE;
	}

	 return (bmp_display(addr, x, y));
}

static struct cmd_tbl cmd_bmp_sub[] = {
	U_BOOT_CMD_MKENT(info, 3, 0, do_bmp_info, "", ""),
	U_BOOT_CMD_MKENT(display, 5, 0, do_bmp_display, "", ""),
};

#ifdef CONFIG_NEEDS_MANUAL_RELOC
void bmp_reloc(void) {
	fixup_cmdtable(cmd_bmp_sub, ARRAY_SIZE(cmd_bmp_sub));
}
#endif

/*
 * Subroutine:  do_bmp
 *
 * Description: Handler for 'bmp' command..
 *
 * Inputs:	argv[1] contains the subcommand
 *
 * Return:      None
 *
 */
static int do_bmp(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct cmd_tbl *c;

	/* Strip off leading 'bmp' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_bmp_sub[0], ARRAY_SIZE(cmd_bmp_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}

U_BOOT_CMD(
	bmp,	5,	1,	do_bmp,
	"manipulate BMP image data",
	"info <imageAddr>          - display image info\n"
	"bmp display <imageAddr> [x y] - display image at x,y"
);

/*
 * Subroutine:  bmp_info
 *
 * Description: Show information about bmp file in memory
 *
 * Inputs:	addr		address of the bmp file
 *
 * Return:      None
 *
 */
static int bmp_info(ulong addr)
{
	struct bmp_image *bmp = (struct bmp_image *)map_sysmem(addr, 0);
	void *bmp_alloc_addr = NULL;
	unsigned long len;

	if (!((bmp->header.signature[0]=='B') &&
	      (bmp->header.signature[1]=='M')))
		bmp = gunzip_bmp(addr, &len, &bmp_alloc_addr);

	if (bmp == NULL) {
		printf("There is no valid bmp file at the given address\n");
		return 1;
	}

	printf("Image size    : %d x %d\n", le32_to_cpu(bmp->header.width),
	       le32_to_cpu(bmp->header.height));
	printf("Bits per pixel: %d\n", le16_to_cpu(bmp->header.bit_count));
	printf("Compression   : %d\n", le32_to_cpu(bmp->header.compression));

	if (bmp_alloc_addr)
		free(bmp_alloc_addr);

	return(0);
}

int bmp_display(ulong addr, int x, int y)
{
	struct udevice *dev;
	int ret;
	struct bmp_image *bmp = map_sysmem(addr, 0);
	void *bmp_alloc_addr = NULL;
	unsigned long len;

	if (!((bmp->header.signature[0]=='B') &&
	      (bmp->header.signature[1]=='M')))
		bmp = gunzip_bmp(addr, &len, &bmp_alloc_addr);

	if (!bmp) {
		printf("There is no valid bmp file at the given address\n");
		return 1;
	}
	addr = map_to_sysmem(bmp);

#ifdef CONFIG_VIDEO_LINK
	dev = video_link_get_video_device();
	if (!dev) {
		ret = -ENODEV;
	} else {
#else
	ret = uclass_first_device_err(UCLASS_VIDEO, &dev);
	if (!ret) {
#endif
		bool align = false;

		if (x == BMP_ALIGN_CENTER || y == BMP_ALIGN_CENTER)
			align = true;

		ret = video_bmp_display(dev, addr, x, y, align);
	}

	if (bmp_alloc_addr)
		free(bmp_alloc_addr);

	return ret ? CMD_RET_FAILURE : 0;
}
