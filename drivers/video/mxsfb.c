// SPDX-License-Identifier: GPL-2.0+
/*
 * Freescale i.MX23/i.MX28 LCDIF driver
 *
 * Copyright (C) 2011-2013 Marek Vasut <marex@denx.de>
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 *
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <env.h>
#include <log.h>
#include <asm/cache.h>
#include <dm/device_compat.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <malloc.h>
#include <video.h>
#include <video_fb.h>

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/dma.h>
#include <asm/io.h>

#include "videomodes.h"
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <mxsfb.h>

#ifdef CONFIG_VIDEO_GIS
#include <gis.h>
#endif

#ifdef CONFIG_IMX_MIPI_DSI_BRIDGE
#include <imx_mipi_dsi_bridge.h>
#endif

#define	PS2KHZ(ps)	(1000000000UL / (ps))
#define HZ2PS(hz)	(1000000000UL / ((hz) / 1000))

#define BITS_PP		18
#define BYTES_PP	4

struct mxs_dma_desc desc;

struct mxsfb_info {
	struct mxs_lcdif_regs *regs;
	unsigned long fb_addr;
	int bpp;
	int rgb_pattern;
};

/**
 * mxsfb_system_setup() - Fine-tune LCDIF configuration
 *
 * This function is used to adjust the LCDIF configuration. This is usually
 * needed when driving the controller in System-Mode to operate an 8080 or
 * 6800 connected SmartLCD.
 */
__weak void mxsfb_system_setup(void)
{
}

/*
 * ARIES M28EVK:
 * setenv videomode
 * video=ctfb:x:800,y:480,depth:18,mode:0,pclk:30066,
 *       le:0,ri:256,up:0,lo:45,hs:1,vs:1,sync:100663296,vmode:0
 *
 * Freescale mx23evk/mx28evk with a Seiko 4.3'' WVGA panel:
 * setenv videomode
 * video=ctfb:x:800,y:480,depth:24,mode:0,pclk:29851,
 * 	 le:89,ri:164,up:23,lo:10,hs:10,vs:10,sync:0,vmode:0
 */

static int mxs_probe_common(struct ctfb_res_modes *mode,
			    struct mxsfb_info *info)
{
	struct mxs_lcdif_regs *regs = info->regs;
	uint32_t word_len = 0, bus_width = 0;
	uint8_t valid_data = 0;

	/* Kick in the LCDIF clock */
	/* ### Clock configuration is board specific, handle outside of the
           driver. Clocks should all be set and started when coming here. */
//###	mxs_set_lcdclk(panel->isaBase, PS2KHZ(mode->pixclock));

	/* Restart the LCDIF block */
	mxs_reset_block((struct mxs_register_32 *)&regs->hw_lcdif_ctrl);

	switch (info->bpp) {
	case 24:
		word_len = LCDIF_CTRL_WORD_LENGTH_24BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_24BIT;
		valid_data = 0x7;
		break;
	case 18:
		word_len = LCDIF_CTRL_WORD_LENGTH_24BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_18BIT;
		valid_data = 0x7;
		break;
	case 16:
		word_len = LCDIF_CTRL_WORD_LENGTH_16BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_16BIT;
		valid_data = 0xf;
		break;
	case 8:
		word_len = LCDIF_CTRL_WORD_LENGTH_8BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_8BIT;
		valid_data = 0xf;
		break;
	}

	writel(bus_width | word_len | LCDIF_CTRL_DOTCLK_MODE |
		LCDIF_CTRL_BYPASS_COUNT | LCDIF_CTRL_LCDIF_MASTER,
		&regs->hw_lcdif_ctrl);

	writel(valid_data << LCDIF_CTRL1_BYTE_PACKING_FORMAT_OFFSET,
		&regs->hw_lcdif_ctrl1);

	writel(LCDIF_CTRL2_OUTSTANDING_REQS_REQ_16
	       | (info->rgb_pattern << LCDIF_CTRL2_ODD_LINE_PATTERN_OFFSET)
	       | (info->rgb_pattern << LCDIF_CTRL2_EVEN_LINE_PATTERN_OFFSET),
	       &regs->hw_lcdif_ctrl2);

	mxsfb_system_setup();

	writel((mode->yres << LCDIF_TRANSFER_COUNT_V_COUNT_OFFSET) | mode->xres,
		&regs->hw_lcdif_transfer_count);

#ifdef CONFIG_IMX_SEC_MIPI_DSI
	writel(LCDIF_VDCTRL0_ENABLE_PRESENT |
		LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT |
		LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT |
		mode->vsync_len, &regs->hw_lcdif_vdctrl0);
#else
	writel(LCDIF_VDCTRL0_ENABLE_PRESENT | LCDIF_VDCTRL0_ENABLE_POL |
		LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT |
		LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT |
		mode->vsync_len, &regs->hw_lcdif_vdctrl0);
#endif
	writel(mode->upper_margin + mode->lower_margin +
		mode->vsync_len + mode->yres,
		&regs->hw_lcdif_vdctrl1);
	writel((mode->hsync_len << LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH_OFFSET) |
		(mode->left_margin + mode->right_margin +
		mode->hsync_len + mode->xres),
		&regs->hw_lcdif_vdctrl2);
	writel(((mode->left_margin + mode->hsync_len) <<
		LCDIF_VDCTRL3_HORIZONTAL_WAIT_CNT_OFFSET) |
		(mode->upper_margin + mode->vsync_len),
		&regs->hw_lcdif_vdctrl3);
	writel((0 << LCDIF_VDCTRL4_DOTCLK_DLY_SEL_OFFSET) | mode->xres,
		&regs->hw_lcdif_vdctrl4);

	writel(info->fb_addr, &regs->hw_lcdif_cur_buf);
	writel(info->fb_addr, &regs->hw_lcdif_next_buf);

	/* Flush FIFO first */
	writel(LCDIF_CTRL1_FIFO_CLEAR, &regs->hw_lcdif_ctrl1_set);

#ifndef CONFIG_VIDEO_MXS_MODE_SYSTEM
	/* Sync signals ON */
	setbits_le32(&regs->hw_lcdif_vdctrl4, LCDIF_VDCTRL4_SYNC_SIGNALS_ON);
#endif

	/* FIFO cleared */
	writel(LCDIF_CTRL1_FIFO_CLEAR, &regs->hw_lcdif_ctrl1_clr);

	/* RUN! */
	writel(LCDIF_CTRL_RUN, &regs->hw_lcdif_ctrl_set);

#ifdef CONFIG_VIDEO_MXS_MODE_SYSTEM
	/*
	 * If the LCD runs in system mode, the LCD refresh has to be triggered
	 * manually by setting the RUN bit in HW_LCDIF_CTRL register. To avoid
	 * having to set this bit manually after every single change in the
	 * framebuffer memory, we set up specially crafted circular DMA, which
	 * sets the RUN bit, then waits until it gets cleared and repeats this
	 * infinitelly. This way, we get smooth continuous updates of the LCD.
	 */
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)MXS_LCDIF_BASE;

	memset(&desc, 0, sizeof(struct mxs_dma_desc));
	desc.address = (dma_addr_t)&desc;
	desc.cmd.data = MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
			MXS_DMA_DESC_WAIT4END |
			(1 << MXS_DMA_DESC_PIO_WORDS_OFFSET);
	desc.cmd.pio_words[0] = readl(&regs->hw_lcdif_ctrl) | LCDIF_CTRL_RUN;
	desc.cmd.next = (uint32_t)&desc.cmd;

	/* Execute the DMA chain. */
	mxs_dma_circ_start(MXS_DMA_CHANNEL_AHB_APBH_LCDIF, &desc);
#endif

	return 0;
}

static int mxs_remove_common(u32 fb, u32 base_addr)
{
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)base_addr;
	int timeout = 1000000;

	if (CONFIG_IS_ENABLED(IMX_MODULE_FUSE)) {
		if (check_module_fused(MODULE_LCDIF))
			return -ENODEV;
	}

	if (!fb)
		return -EINVAL;

#ifdef CONFIG_IMX_MIPI_DSI_BRIDGE
	imx_mipi_dsi_bridge_disable();
#endif

	writel(fb, &regs->hw_lcdif_cur_buf_reg);
	writel(fb, &regs->hw_lcdif_next_buf_reg);
	writel(LCDIF_CTRL1_VSYNC_EDGE_IRQ, &regs->hw_lcdif_ctrl1_clr);
	while (--timeout) {
		if (readl(&regs->hw_lcdif_ctrl1_reg) &
		    LCDIF_CTRL1_VSYNC_EDGE_IRQ)
			break;
		udelay(1);
	}
	mxs_reset_block((struct mxs_register_32 *)&regs->hw_lcdif_ctrl_reg);

	return 0;
}

#ifndef CONFIG_DM_VIDEO

static GraphicDevice panel;

static const struct fb_videomode *fbmode;
static struct mxsfb_info info;

int mxs_lcd_panel_setup(uint32_t base_addr, const struct fb_videomode *mode,
			int bpp, int rgb_pattern)
{
	fbmode = mode;
	info.bpp = bpp;
	info.regs = (struct mxs_lcdif_regs *)base_addr;
	info.rgb_pattern = rgb_pattern;

	return 0;
}

void mxs_lcd_get_panel(struct display_panel *dispanel)
{
	if (dispanel && fbmode) {
		dispanel->width = panel.winSizeX;
		dispanel->height = panel.winSizeY;
		dispanel->reg_base = panel.isaBase;
		dispanel->gdfindex = panel.gdfIndex;
		dispanel->gdfbytespp = panel.gdfBytesPP;
	}
}

void lcdif_power_down(void)
{
	mxs_remove_common(panel.frameAdrs, panel.isaBase);
}

void *video_hw_init(void)
{
	int ret = 0;
	char *penv;
	void *fb = NULL;
	struct ctfb_res_modes mode;

	puts("Video: ");

	if (!fbmode) {
	/* Suck display configuration from "videomode" variable */
	penv = env_get("videomode");
	if (!penv) {
			printf("MXSFB: 'videomode' variable not set!\n");
		return NULL;
	}

		info.bpp = video_get_params(&mode, penv);
		info.rgb_pattern = PATTERN_RGB;
		info.regs = (struct mxs_lcdif_regs *)MXS_LCDIF_BASE;
	} else {
		mode.xres = fbmode->xres;
		mode.yres = fbmode->yres;
		mode.pixclock = fbmode->pixclock;
		mode.left_margin = fbmode->left_margin;
		mode.right_margin = fbmode->right_margin;
		mode.upper_margin = fbmode->upper_margin;
		mode.lower_margin = fbmode->lower_margin;
		mode.hsync_len = fbmode->hsync_len;
		mode.vsync_len = fbmode->vsync_len;
		mode.sync = fbmode->sync;
		mode.vmode = fbmode->vmode;
	}

	if (CONFIG_IS_ENABLED(IMX_MODULE_FUSE)) {
		if (check_module_fused(MODULE_LCDIF)) {
			printf("LCDIF@0x%x is fused, disable it\n", MXS_LCDIF_BASE);
			return NULL;
		}
	}
	/* fill in Graphic device struct */
	sprintf(panel.modeIdent, "%dx%dx%d", mode.xres, mode.yres, info.bpp);

	panel.isaBase = (unsigned int)info.regs;
	panel.winSizeX = mode.xres;
	panel.winSizeY = mode.yres;
	panel.plnSizeX = mode.xres;
	panel.plnSizeY = mode.yres;

	switch (info.bpp) {
	case 24:
	case 18:
		panel.gdfBytesPP = 4;
		panel.gdfIndex = GDF_32BIT_X888RGB;
		break;
	case 16:
		panel.gdfBytesPP = 2;
		panel.gdfIndex = GDF_16BIT_565RGB;
		break;
	case 8:
		panel.gdfBytesPP = 1;
		panel.gdfIndex = GDF__8BIT_INDEX;
		break;
	default:
		printf("MXSFB: Invalid BPP specified! (bpp = %i)\n", info.bpp);
		return NULL;
	}

	panel.memSize = mode.xres * mode.yres * panel.gdfBytesPP;

	/* Allocate framebuffer */
	fb = memalign(ARCH_DMA_MINALIGN,
		      roundup(panel.memSize, ARCH_DMA_MINALIGN));
	if (!fb) {
		printf("MXSFB: Error allocating framebuffer!\n");
		return NULL;
	}

	/* Wipe framebuffer */
	memset(fb, 0, panel.memSize);

	panel.frameAdrs = (ulong)fb;
	info.fb_addr = (ulong)fb;

	printf("%s\n", panel.modeIdent);

#ifdef CONFIG_IMX_MIPI_DSI_BRIDGE
	int dsi_ret;

	imx_mipi_dsi_bridge_mode_set((struct fb_videomode *) fbmode);
	dsi_ret = imx_mipi_dsi_bridge_enable();
	if (dsi_ret) {
		printf("Enable DSI bridge failed, err %d\n", dsi_ret);
		return NULL;
	}
#endif

	ret = mxs_probe_common(&mode, &info);
	if (ret)
		goto dealloc_fb;

	return (void *)&panel;

dealloc_fb:
	free(fb);

	return NULL;
}
#else /* ifndef CONFIG_DM_VIDEO */

static int mxs_of_get_timings(struct udevice *dev,
			      struct display_timing *timings,
			      u32 *bpp)
{
	int ret = 0;
	u32 display_phandle;
	ofnode display_node;

	ret = ofnode_read_u32(dev_ofnode(dev), "display", &display_phandle);
	if (ret) {
		dev_err(dev, "required display property isn't provided\n");
		return -EINVAL;
	}

	display_node = ofnode_get_by_phandle(display_phandle);
	if (!ofnode_valid(display_node)) {
		dev_err(dev, "failed to find display subnode\n");
		return -EINVAL;
	}

	ret = ofnode_read_u32(display_node, "bits-per-pixel", bpp);
	if (ret) {
		dev_err(dev,
			"required bits-per-pixel property isn't provided\n");
		return -EINVAL;
	}

	ret = ofnode_decode_display_timing(display_node, 0, timings);
	if (ret) {
		dev_err(dev, "failed to get any display timings\n");
		return -EINVAL;
	}

	return ret;
}

static int mxs_video_probe(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);

	struct ctfb_res_modes mode;
	struct mxsfb_info info;
	struct display_timing timings;
	u32 fb_start, fb_end;
	int ret;

	debug("%s() plat: base 0x%lx, size 0x%x\n",
	       __func__, plat->base, plat->size);

	info.rgb_pattern = PATTERN_RGB;
	info.regs = (struct mxs_lcdif_regs *)MXS_LCDIF_BASE;
	info.fb_addr = plat->base;
	info.bpp = 0;

	ret = mxs_of_get_timings(dev, &timings, &info.bpp);
	if (ret)
		return ret;

	mode.xres = timings.hactive.typ;
	mode.yres = timings.vactive.typ;
	mode.left_margin = timings.hback_porch.typ;
	mode.right_margin = timings.hfront_porch.typ;
	mode.upper_margin = timings.vback_porch.typ;
	mode.lower_margin = timings.vfront_porch.typ;
	mode.hsync_len = timings.hsync_len.typ;
	mode.vsync_len = timings.vsync_len.typ;
	mode.pixclock = HZ2PS(timings.pixelclock.typ);

	ret = mxs_probe_common(&mode, &info);
	if (ret)
		return ret;

	switch (bpp) {
	case 32:
	case 24:
	case 18:
		uc_priv->bpix = VIDEO_BPP32;
		break;
	case 16:
		uc_priv->bpix = VIDEO_BPP16;
		break;
	case 8:
		uc_priv->bpix = VIDEO_BPP8;
		break;
	default:
		dev_err(dev, "invalid bpp specified (bpp = %i)\n", bpp);
		return -EINVAL;
	}

	uc_priv->xsize = mode.xres;
	uc_priv->ysize = mode.yres;

	/* Enable dcache for the frame buffer */
	fb_start = plat->base & ~(MMU_SECTION_SIZE - 1);
	fb_end = plat->base + plat->size;
	fb_end = ALIGN(fb_end, 1 << MMU_SECTION_SHIFT);
	mmu_set_region_dcache_behaviour(fb_start, fb_end - fb_start,
					DCACHE_WRITEBACK);
	video_set_flush_dcache(dev, true);
	gd->fb_base = plat->base;

	return ret;
}

static int mxs_video_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct display_timing timings;
	u32 bpp = 0;
	u32 bytes_pp = 0;
	int ret;

	ret = mxs_of_get_timings(dev, &timings, &bpp);
	if (ret)
		return ret;

	switch (bpp) {
	case 32:
	case 24:
	case 18:
		bytes_pp = 4;
		break;
	case 16:
		bytes_pp = 2;
		break;
	case 8:
		bytes_pp = 1;
		break;
	default:
		dev_err(dev, "invalid bpp specified (bpp = %i)\n", bpp);
		return -EINVAL;
	}

	plat->size = timings.hactive.typ * timings.vactive.typ * bytes_pp;

	return 0;
}

static int mxs_video_remove(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	mxs_remove_common(plat->base, MXS_LCDIF_BASE);

	return 0;
}

static const struct udevice_id mxs_video_ids[] = {
	{ .compatible = "fsl,imx23-lcdif" },
	{ .compatible = "fsl,imx28-lcdif" },
	{ .compatible = "fsl,imx7ulp-lcdif" },
	{ .compatible = "fsl,imxrt-lcdif" },
	{ .compatible = "fsl,imx8mm-lcdif" },
	{ .compatible = "fsl,imx8mn-lcdif" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(mxs_video) = {
	.name	= "mxs_video",
	.id	= UCLASS_VIDEO,
	.of_match = mxs_video_ids,
	.bind	= mxs_video_bind,
	.probe	= mxs_video_probe,
	.remove = mxs_video_remove,
	.flags	= DM_FLAG_PRE_RELOC | DM_FLAG_OS_PREPARE,
};
#endif /* ifndef CONFIG_DM_VIDEO */
