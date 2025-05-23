// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016
 * Xilinx, Inc.
 *
 * (C) Copyright 2016
 * Toradex AG
 *
 * Michal Simek <michal.simek@xilinx.com>
 * Stefan Agner <stefan.agner@toradex.com>
 */
#include <common.h>
#include <binman_sym.h>
#include <image.h>
#include <log.h>
#include <mapmem.h>
#include <spl.h>
#include <linux/libfdt.h>

unsigned long __weak spl_ram_get_uboot_base(void)
{
	return CONFIG_SPL_LOAD_FIT_ADDRESS;
}

static ulong spl_ram_load_read(struct spl_load_info *load, ulong sector,
			       ulong count, void *buf)
{
	ulong addr;

	debug("%s: sector %lx, count %lx, buf %lx\n",
	      __func__, sector, count, (ulong)buf);
	memcpy(buf, (void *)(sector), count);
	return count;
}

static int spl_ram_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	struct legacy_img_hdr *header;
	int ret;

	header = (struct legacy_img_hdr *)spl_ram_get_uboot_base();

	if (CONFIG_IS_ENABLED(IMAGE_PRE_LOAD)) {
		unsigned long addr = (unsigned long)header;
		ret = image_pre_load(addr);

		if (ret)
			return ret;

		addr += image_load_offset;
		header = (struct legacy_img_hdr *)addr;
	}

#if CONFIG_IS_ENABLED(DFU)
	if (bootdev->boot_device == BOOT_DEVICE_DFU)
		spl_dfu_cmd(0, "dfu_alt_info_ram", "ram", "0");
#endif

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
		memset(&load, 0, sizeof(load));
		load.bl_len = 1;
		load.read = spl_ram_load_read;
		ret = spl_load_simple_fit(spl_image, &load, (ulong)header, header);
	} else if (IS_ENABLED(CONFIG_SPL_LOAD_IMX_CONTAINER)) {
		struct spl_load_info load;

		memset(&load, 0, sizeof(load));
		load.bl_len = 1;
		load.read = spl_ram_load_read;

		ret = spl_load_imx_container(spl_image, &load, (ulong)header);
	} else {
		ulong u_boot_pos = spl_get_image_pos();

		debug("Legacy image\n");
		/*
		 * Get the header.  It will point to an address defined by
		 * handoff which will tell where the image located inside
		 * the flash.
		 */
		debug("u_boot_pos = %lx\n", u_boot_pos);
		if (u_boot_pos == BINMAN_SYM_MISSING) {
			/*
			 * No binman support or no information. For now, fix it
			 * to the address pointed to by U-Boot.
			 */
			u_boot_pos = (ulong)spl_get_load_buffer(-sizeof(*header),
								sizeof(*header));
		}
		header = (struct legacy_img_hdr *)map_sysmem(u_boot_pos, 0);

		ret = spl_parse_image_header(spl_image, bootdev, header);
	}

	return ret;
}
#if CONFIG_IS_ENABLED(RAM_DEVICE)
SPL_LOAD_IMAGE_METHOD("RAM", 0, BOOT_DEVICE_RAM, spl_ram_load_image);
#endif
#if CONFIG_IS_ENABLED(DFU)
SPL_LOAD_IMAGE_METHOD("DFU", 0, BOOT_DEVICE_DFU, spl_ram_load_image);
#endif
