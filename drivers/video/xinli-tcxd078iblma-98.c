// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <power/regulator.h>

#define CMD_TABLE_LEN 2
typedef u8 cmd_set_table[CMD_TABLE_LEN];

struct tcxd078_panel_priv {
	struct gpio_desc reset_gpio;
	struct gpio_desc enable_gpio;
	struct udevice *vci;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	unsigned int lanes;
	bool prepared;
	bool enabled;
};

/* Manufacturer Command Set */
static const cmd_set_table mcs_tcxd078[] = {
	{0xB0, 0x5A},

	{0xB1, 0x00},
	{0x89, 0x01},
	{0x91, 0x17},

	{0xB1, 0x03},
	{0x2C, 0x28},

	{0x00, 0xB7},
	{0x01, 0x1B},
	{0x02, 0x00},
	{0x03, 0x00},
	{0x04, 0x00},
	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x00},
	{0x0A, 0x01},
	{0x0B, 0x01},
	{0x0C, 0x20},
	{0x0D, 0x00},
	{0x0E, 0x24},
	{0x0F, 0x1C},
	{0x10, 0xC9},
	{0x11, 0x60},
	{0x12, 0x70},
	{0x13, 0x01},
	{0x14, 0xE3},
	{0x15, 0xFF},
	{0x16, 0x3D},
	{0x17, 0x0E},
	{0x18, 0x01},
	{0x19, 0x00},
	{0x1A, 0x00},
	{0x1B, 0xFC},
	{0x1C, 0x0B},
	{0x1D, 0xA0},
	{0x1E, 0x03},
	{0x1F, 0x04},
	{0x20, 0x0C},
	{0x21, 0x00},
	{0x22, 0x04},
	{0x23, 0x81},
	{0x24, 0x1F},
	{0x25, 0x10},
	{0x26, 0x9B},
	{0x2D, 0x01},
	{0x2E, 0x84},
	{0x2F, 0x00},
	{0x30, 0x02},
	{0x31, 0x08},
	{0x32, 0x01},
	{0x33, 0x1C},
	{0x34, 0x70},
	{0x35, 0xFF},
	{0x36, 0xFF},
	{0x37, 0xFF},
	{0x38, 0xFF},
	{0x39, 0xFF},
	{0x3A, 0x05},
	{0x3B, 0x00},
	{0x3C, 0x00},
	{0x3D, 0x00},
	{0x3E, 0xCF},
	{0x3F, 0x84},
	{0x40, 0x28},
	{0x41, 0xFC},
	{0x42, 0x01},
	{0x43, 0x40},
	{0x44, 0x05},
	{0x45, 0xE8},
	{0x46, 0x16},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x88},
	{0x4A, 0x08},
	{0x4B, 0x05},
	{0x4C, 0x03},
	{0x4D, 0xD0},
	{0x4E, 0x13},
	{0x4F, 0xFF},
	{0x50, 0x0A},
	{0x51, 0x53},
	{0x52, 0x26},
	{0x53, 0x22},
	{0x54, 0x09},
	{0x55, 0x22},
	{0x56, 0x00},
	{0x57, 0x1C},
	{0x58, 0x03},
	{0x59, 0x3F},
	{0x5A, 0x28},
	{0x5B, 0x01},
	{0x5C, 0xCC},
	{0x5D, 0x21},
	{0x5E, 0x84},
	{0x5F, 0x10},
	{0x60, 0x42},
	{0x61, 0x40},
	{0x62, 0x06},
	{0x63, 0x3A},
	{0x64, 0xA6},
	{0x65, 0x04},
	{0x66, 0x09},
	{0x67, 0x21},
	{0x68, 0x84},
	{0x69, 0x10},
	{0x6A, 0x42},
	{0x6B, 0x08},
	{0x6C, 0x21},
	{0x6D, 0x84},
	{0x6E, 0x74},
	{0x6F, 0xE2},
	{0x70, 0x6B},
	{0x71, 0x6B},
	{0x72, 0x94},
	{0x73, 0x10},
	{0x74, 0x42},
	{0x75, 0x08},
	{0x76, 0x00},
	{0x77, 0x00},
	{0x78, 0x0F},
	{0x79, 0xE0},
	{0x7A, 0x01},
	{0x7B, 0xFF},
	{0x7C, 0xFF},
	{0x7D, 0x0F},
	{0x7E, 0x41},
	{0x7F, 0xFE},

	{0xB1, 0x02},
	{0x00, 0xFF},
	{0x01, 0x05},
	{0x02, 0xC8},
	{0x03, 0x00},
	{0x04, 0x14},
	{0x05, 0x4B},
	{0x06, 0x64},
	{0x07, 0x0A},
	{0x08, 0xC0},
	{0x09, 0x00},
	{0x0A, 0x00},
	{0x0B, 0x10},
	{0x0C, 0xE6},
	{0x0D, 0x0D},
	{0x0F, 0x00},
	{0x10, 0x79},
	{0x11, 0xAB},
	{0x12, 0xA7},
	{0x13, 0xD7},
	{0x14, 0x7B},
	{0x15, 0x9D},
	{0x16, 0x74},
	{0x17, 0x6D},
	{0x18, 0x73},
	{0x19, 0xB3},
	{0x1A, 0x6E},
	{0x1B, 0x0E},
	{0x1C, 0xFF},
	{0x1D, 0xFF},
	{0x1E, 0xFF},
	{0x1F, 0xFF},
	{0x20, 0xFF},
	{0x21, 0xFF},
	{0x22, 0xFF},
	{0x23, 0xFF},
	{0x24, 0xFF},
	{0x25, 0xFF},
	{0x26, 0xFF},
	{0x27, 0x1F},
	{0x28, 0xFF},
	{0x29, 0xFF},
	{0x2A, 0xFF},
	{0x2B, 0xFF},
	{0x2C, 0xFF},
	{0x2D, 0x07},
	{0x33, 0x3F},
	{0x35, 0x7F},
	{0x36, 0x3F},
	{0x38, 0xFF},
	{0x3A, 0x80},
	{0x3B, 0x01},
	{0x3C, 0x80},
	{0x3D, 0x2C},
	{0x3E, 0x00},
	{0x3F, 0x90},
	{0x40, 0x05},
	{0x41, 0x00},
	{0x42, 0xB2},
	{0x43, 0x00},
	{0x44, 0x40},
	{0x45, 0x06},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x9B},
	{0x49, 0xD2},
	{0x4A, 0x21},
	{0x4B, 0x43},
	{0x4C, 0x16},
	{0x4D, 0xC0},
	{0x4E, 0x0F},
	{0x4F, 0xF1},
	{0x50, 0x78},
	{0x51, 0x7A},
	{0x52, 0x34},
	{0x53, 0x99},
	{0x54, 0xA2},
	{0x55, 0x02},
	{0x56, 0x14},
	{0x57, 0xB8},
	{0x58, 0xDC},
	{0x59, 0xD4},
	{0x5A, 0xEF},
	{0x5B, 0xF7},
	{0x5C, 0xFB},
	{0x5D, 0xFD},
	{0x5E, 0x7E},
	{0x5F, 0xBF},
	{0x60, 0xEF},
	{0x61, 0xE6},
	{0x62, 0x76},
	{0x63, 0x73},
	{0x64, 0xBB},
	{0x65, 0xDD},
	{0x66, 0x6E},
	{0x67, 0x37},
	{0x68, 0x8C},
	{0x69, 0x08},
	{0x6A, 0x31},
	{0x6B, 0xB8},
	{0x6C, 0xB8},
	{0x6D, 0xB8},
	{0x6E, 0xB8},
	{0x6F, 0xB8},
	{0x70, 0x5C},
	{0x71, 0x2E},
	{0x72, 0x17},
	{0x73, 0x00},
	{0x74, 0x00},
	{0x75, 0x00},
	{0x76, 0x00},
	{0x77, 0x00},
	{0x78, 0x00},
	{0x79, 0x00},
	{0x7A, 0xDC},
	{0x7B, 0xDC},
	{0x7C, 0xDC},
	{0x7D, 0xDC},
	{0x7E, 0xDC},
	{0x7F, 0x6E},
	{0x0B, 0x00},

	{0xB1, 0x03},
	{0x2C, 0x2C},

	{0xB1, 0x00},
	{0x89, 0x03},
};


static const struct display_timing default_timing = {
	.pixelclock.typ		= 47000000,
	.hactive.typ		= 400,
	.hfront_porch.typ	= 50,
	.hback_porch.typ	= 50,
	.hsync_len.typ		= 30,
	.vactive.typ		= 1280,
	.vfront_porch.typ	= 30,
	.vback_porch.typ	= 30,
	.vsync_len.typ		= 20,
	.flags = DISPLAY_FLAGS_HSYNC_LOW |
		 DISPLAY_FLAGS_VSYNC_LOW |
		 DISPLAY_FLAGS_DE_HIGH |
		 DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};


static int tcxd078_push_cmd_list(struct mipi_dsi_device *device,
				     const cmd_set_table *cmd_set,
				     size_t count)
{
	size_t i;
	const cmd_set_table *cmd;
	int ret;

	for (i = 0; i < count; i++) {
		cmd = cmd_set++;
		ret = mipi_dsi_generic_write(device, cmd, CMD_TABLE_LEN);
		if (ret < 0)
			return ret;
	}

	return 0;
};

static int tcxd078_enable(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	u16 brightness;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = tcxd078_push_cmd_list(dsi, &mcs_tcxd078[0],
		sizeof(mcs_tcxd078) / CMD_TABLE_LEN);
	if (ret < 0) {
		printf("Failed to send MCS (%d)\n", ret);
		return -EIO;
	}

	/* Set display brightness */
	brightness = 255; /* Max brightness */
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 2);
	if (ret < 0) {
		printf("Failed to set display brightness (%d)\n",
				  ret);
		return -EIO;
	}

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		printf("Failed to exit sleep mode (%d)\n", ret);
		return -EIO;
	}

	mdelay(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		printf("Failed to set display ON (%d)\n", ret);
		return -EIO;
	}

	mdelay(100);

	return 0;
}


static int tcxd078_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return tcxd078_enable(dev);
}

static int tcxd078_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct tcxd078_panel_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int tcxd078_panel_probe(struct udevice *dev)
{
	struct tcxd078_panel_priv *priv = dev_get_priv(dev);
	int ret;
	u32 video_mode;

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO
		| MIPI_DSI_MODE_EOT_PACKET;

	ret = dev_read_u32(dev, "video-mode", &video_mode);
	if (!ret) {
		switch (video_mode) {
		case 0:
			/* burst mode */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_BURST;
			break;
		case 1:
			/* non-burst mode with sync event */
			break;
		case 2:
			/* non-burst mode with sync pulse */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
			break;
		default:
			dev_warn(dev, "invalid video mode %d\n", video_mode);
			break;
		}
	}

	ret = dev_read_u32(dev, "dsi-lanes", &priv->lanes);
	if (ret < 0) {
		printf("Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	ret = device_get_supply_regulator(dev, "vci", &priv->vci);
	if (ret < 0) {
		priv->vci = NULL;
	} else {
		regulator_set_enable(priv->vci, true);
		mdelay(150);
	}

	ret = gpio_request_by_name(dev, "enable-gpio", 0, &priv->enable_gpio,
				   GPIOD_IS_OUT);
	if (ret < 0)
		priv->enable_gpio.dev = NULL;
	else
		dm_gpio_set_value(&priv->enable_gpio, true);

	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset_gpio,
				   GPIOD_IS_OUT | GPIOD_ACTIVE_LOW);
	if (ret < 0)
		priv->reset_gpio.dev = NULL;
	else {
		/* reset panel */
		dm_gpio_set_value(&priv->reset_gpio, true);
		mdelay(100);
		dm_gpio_set_value(&priv->reset_gpio, false);
		mdelay(100);
	}

	return 0;
}

static int tcxd078_panel_disable(struct udevice *dev)
{
	struct tcxd078_panel_priv *priv = dev_get_priv(dev);

	if (priv->reset_gpio.dev)
		dm_gpio_set_value(&priv->reset_gpio, true);
	if (priv->enable_gpio.dev)
		dm_gpio_set_value(&priv->enable_gpio, false);
	if (priv->vci)
		regulator_set_enable(priv->vci, false);

	return 0;
}

static const struct panel_ops tcxd078_panel_ops = {
	.enable_backlight = tcxd078_panel_enable_backlight,
	.get_display_timing = tcxd078_panel_get_display_timing,
};

static const struct udevice_id tcxd078_panel_ids[] = {
	{ .compatible = "xinli,tcxd078iblma-98" },
	{ }
};

U_BOOT_DRIVER(tcxd078_panel) = {
	.name			  = "tcxd078iblma-98",
	.id			  = UCLASS_PANEL,
	.of_match		  = tcxd078_panel_ids,
	.ops			  = &tcxd078_panel_ops,
	.probe			  = tcxd078_panel_probe,
	.remove			  = tcxd078_panel_disable,
	.plat_auto = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto = sizeof(struct tcxd078_panel_priv),
};
