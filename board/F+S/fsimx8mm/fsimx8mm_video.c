#include <common.h>
#include <asm/mach-imx/video.h>
#include <i2c.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <sec_mipi_dsim.h>
#include <imx_mipi_dsi_bridge.h>
#include <mipi_dsi_panel.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/arch/clock.h>
#include <dm.h>
#include "../common/fs_fdt_common.h"	/* fs_fdt_set_val(), ... */
#include "../common/fs_board_common.h"	/* fs_board_*() */
#include "sec_mipi_dphy_ln14lpp.h"
#include "sec_mipi_pll_1432x.h"

#define BT_PICOCOREMX8MM 	0
#define BT_PICOCOREMX8MX	1

#define FEAT_LVDS	(1<<8)
#define FEAT_MIPI_DSI	(1<<9)

#ifdef CONFIG_VIDEO_MXS

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				9
#define MIPI				10


#define BL_ON_PAD IMX_GPIO_NR(5, 3)
static iomux_v3_cfg_t const bl_on_pads[] = {
	IMX8MM_PAD_SPDIF_TX_GPIO5_IO3 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define VLCD_ON_8MM_PAD IMX_GPIO_NR(4, 28)
static iomux_v3_cfg_t const vlcd_on_8mm_pads[] = {
	IMX8MM_PAD_SAI3_RXFS_GPIO4_IO28 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#define LVDS_RST_8MM_PAD IMX_GPIO_NR(4, 31)
static iomux_v3_cfg_t const lvds_rst_8mm_pads[] = {
	IMX8MM_PAD_SAI3_TXFS_GPIO4_IO31 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define LVDS_CLK_8MM_PAD IMX_GPIO_NR(1, 15)
static iomux_v3_cfg_t const lvds_clk_8mm_pads[] = {
	IMX8MM_PAD_GPIO1_IO15_GPIO1_IO15 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define VLCD_ON_8MX_PAD IMX_GPIO_NR(5, 1)
static iomux_v3_cfg_t const vlcd_on_8mx_pads[] = {
	IMX8MM_PAD_SAI3_TXD_GPIO5_IO1 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#define LVDS_RST_8MX_PAD IMX_GPIO_NR(1, 8)
#define LVDS_STBY_8MX_PAD IMX_GPIO_NR(1, 4)
static iomux_v3_cfg_t const lvds_rst_8mx_pads[] = {
	IMX8MM_PAD_GPIO1_IO08_GPIO1_IO8 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO04_GPIO1_IO4 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define MIPI_RST_PAD IMX_GPIO_NR(1, 1)
static iomux_v3_cfg_t const mipi_rst_pads[] = {
		IMX8MM_PAD_GPIO1_IO01_GPIO1_IO1 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

/*
 * Possible display configurations
 *
 *   Board          MIPI      LVDS0      LVDS1
 *   -------------------------------------------------------------
 *   PicoCoreMX8MM  4 lanes*  24 bit²    24 bit²
 *   PicoCoreMX8MN  4 lanes*  24 bit²    24 bit²
 *
 * The entry marked with * is the default port.
 * The entry marked with ² only work with a MIPI to LVDS converter
 *
 * Display initialization sequence:
 *
 *  1. board_r.c: board_init_r() calls stdio_add_devices().
 *  2. stdio.c: stdio_add_devices() calls drv_video_init().
 *  3. cfb_console.c: drv_video_init() calls board_video_skip(); if this
 *     returns non-zero, the display will not be started.
 *  4. video.c: board_video_skip(): Parse env variable "panel" if available
 *     and search struct display_info_t of board specific file. If env
 *     variable "panel" is not available parse struct display_info_t and call
 *     detect function, if successful use this display or try to detect next
 *     display. If no detect function is available use first display of struct
 *     display_info_t.
 *  5. fsimx8mx.c: board_video_skip parse display parameter of display_info_t,
 *     detect and enable function.
 *  6. cfb_console.c: drv_video_init() calls cfb_video_init().
 *  7. cfb_console.c: video_init() calls video_hw_init().
 *  8. video_common.c: video_hw_init() calls imx8m_display_init().
 *  9. video_common.c: imx8m_display_init() initialize registers of dccs.
 * 10. cfb_console.c: calls video_logo().
 * 11. cfb_console.c: video_logo() draws either the console logo and the welcome
 *     message, or if environment variable splashimage is set, the splash
 *     screen.
 * 12. cfb_console.c: drv_video_init() registers the console as stdio device.
 * 13. board_r.c: board_init_r() calls board_late_init().
 * 14. fsimx8mx.c: board_late_init() calls fs_board_set_backlight_all() to
 *     enable all active displays.
 */


#define TC358764_ADDR 0xF

static int tc358764_i2c_reg_write(struct udevice *dev, uint addr, uint32_t *data, int length)
{
	int err;

	err = dm_i2c_write (dev, addr, (uint8_t*)data, length);
	return err;
}

static int tc358764_i2c_reg_read(struct udevice *dev, uint addr, uint32_t *data, int length)
{
	int err;

	err = dm_i2c_read (dev, addr, (uint8_t*)data, length);
	if (err)
	{
		return err;
	}
	return 0;
}

/* System registers */
#define SYS_RST			0x0504 /* System Reset */
#define SYS_ID			0x0580 /* System ID */


int tc358764_setup(struct mipi_dsi_client_dev * dev)
{
	mdelay(100);
#ifdef CONFIG_VIDEO_MXS
	imx_iomux_v3_setup_multiple_pads (bl_on_pads, ARRAY_SIZE (bl_on_pads));
	/* backlight off */
	gpio_request (BL_ON_PAD, "BL_ON");
	gpio_direction_output (BL_ON_PAD, 0);

	/* set vlcd on*/
	switch (fs_board_get_type())
	{
	case BT_PICOCOREMX8MM:
		imx_iomux_v3_setup_multiple_pads (vlcd_on_8mm_pads, ARRAY_SIZE (vlcd_on_8mm_pads));
		gpio_request (VLCD_ON_8MM_PAD, "VLCD_ON");
		gpio_direction_output (VLCD_ON_8MM_PAD, 1);
		break;
	case BT_PICOCOREMX8MX:
		imx_iomux_v3_setup_multiple_pads (vlcd_on_8mx_pads, ARRAY_SIZE (vlcd_on_8mx_pads));
		gpio_request (VLCD_ON_8MX_PAD, "VLCD_ON");
		gpio_direction_output (VLCD_ON_8MX_PAD, 1);
		break;
	}
	/* backlight on */
	gpio_direction_output (BL_ON_PAD, 1);
#endif

	struct udevice *bus = 0, *mipi2lvds_dev = 0;
	int i2c_bus = 0;
	int ret;
	uint8_t val[4] =
		{ 0 };

	switch (fs_board_get_type()) 
	{
	case BT_PICOCOREMX8MM:
		i2c_bus = 3;
		break;
	case BT_PICOCOREMX8MX:
		i2c_bus = 0;
		break;
	}

	ret = uclass_get_device_by_seq (UCLASS_I2C, i2c_bus, &bus);
	if (ret)
	{
		printf ("%s: No bus %d\n", __func__, i2c_bus);
		return 1;
	}

	ret = dm_i2c_probe (bus, TC358764_ADDR, 0, &mipi2lvds_dev);
	if (ret)
	{
		printf ("%s: Can't find device id=0x%x, on bus %d, ret %d\n", __func__,
			TC358764_ADDR, i2c_bus, ret);
		return 1;
	}

	/* offset */
	i2c_set_chip_offset_len (mipi2lvds_dev, 2);

	/* read chip/rev register with */
	tc358764_i2c_reg_read (mipi2lvds_dev, SYS_ID, val, sizeof(val));

	if (val[1] == 0x65)
		printf ("DSI2LVDS:  TC358764 Rev. 0x%x.\n", (uint8_t) (val[0] & 0xFF));
	else
		printf ("DSI2LVDS:  ID: 0x%x Rev. 0x%x.\n", (uint8_t) (val[1] & 0xFF),
			(uint8_t) (val[0] & 0xFF));

	uint tc3_addr[] = {
		/* dsi basic parameters in lp mode */
		0x013c, 0x0114, 0x0164, 0x0168,
		0x016C ,0x0170, 0x0134, 0x0210,
		0x0104, 0x0204,
		/* Timing and mode settings */
		0x0450, 0x0454, 0x0458, 0x045C,
		0x0460, 0x0464 ,0x04A0, 0x04A0, 0x0504,
		/* color mapping settings	*/
		0x0480, 0x0484, 0x0488, 0x048C, 0x0490,
		0x0494, 0x0498,
		 /* LVDS enable */
        0x049C 
	};


	uint32_t tc3_values[] = {
		/* dsi basic parameters in lp mode */
		0x10002, 0x1, 0x0, 0x0,
		0x00000, 0x0, 0x1F, 0x1F,
		0x00001, 0x1,
		/* Timing and mode settings */
		0x03200120, 0x1A0014, 0xD20320, 0x160003,
		0x1501E0, 0x1, 0x44802D, 0x4802D, 0x4,
		/* color mapping settings	*/
		0x3020100, 0x8050704 ,0x0F0E0A09, 0x100D0C0B, 0x12111716,
		0x1B151413, 0x61A1918,
		0x00000031,
  	};

    for(int i = 0;i<(sizeof(tc3_values)/sizeof(tc3_values[0]));i++){
		ret = tc358764_i2c_reg_write (mipi2lvds_dev, tc3_addr[i], &tc3_values[i], sizeof(tc3_values[i]));
		udelay(100);
	}
	return ret;
}

static int sn65dsi84_init(void)
{
  struct udevice *bus = 0, *mipi2lvds_dev = 0;
  int i2c_bus = 1;
  int ret;
  uint8_t val[4] ={ 0 };

#ifdef CONFIG_VIDEO_MXS
	imx_iomux_v3_setup_multiple_pads (bl_on_pads, ARRAY_SIZE (bl_on_pads));
	/* backlight off */
	gpio_request (BL_ON_PAD, "BL_ON");
	gpio_direction_output (BL_ON_PAD, 0);

	/* set vlcd on*/
	switch (fs_board_get_type())
	{
	case BT_PICOCOREMX8MM:
		imx_iomux_v3_setup_multiple_pads (vlcd_on_8mm_pads, ARRAY_SIZE (vlcd_on_8mm_pads));
		gpio_request (VLCD_ON_8MM_PAD, "VLCD_ON");
		gpio_direction_output (VLCD_ON_8MM_PAD, 1);
		break;
	case BT_PICOCOREMX8MX:
		imx_iomux_v3_setup_multiple_pads (vlcd_on_8mx_pads, ARRAY_SIZE (vlcd_on_8mx_pads));
		gpio_request (VLCD_ON_8MX_PAD, "VLCD_ON");
		gpio_direction_output (VLCD_ON_8MX_PAD, 1);
		break;
	}
	/* backlight on */
	gpio_direction_output (BL_ON_PAD, 1);
#endif

  uint sn65_addr[] = {
		 0x09, 0x0A, 0x0B, 0x0D,
	     0x10, 0x11, 0x12, 0x13,
	   	 0x18, 0x19, 0x1A, 0x1B,
		 0x20, 0x21, 0x22, 0x23,
		 0x24, 0x25, 0x26, 0x27,
		 0x28, 0x29, 0x2A, 0x2B,
		 0x2C, 0x2D, 0x2E, 0x2F,
		 0x30, 0x31, 0x32, 0x33,
		 0x34, 0x35, 0x36, 0x37,
		 0x38, 0x39, 0x3A ,0x3B,
		 0x3C, 0x3D, 0x3E, 0x0D
  };

  uint8_t sn65_values[] = {
        0x00, 0x01, 0x10, 0x00,
        /* DSI registers */
        0x26, 0x00, 0x14, 0x00,
        /* LVDS registers */
        0x78, 0x00, 0x03, 0x00,
        /* video registers */
        /* cha-al-len-l, cha-al-len-h */
        0x20, 0x03, 0x00, 0x00,
        /* cha-v-ds-l, cha-v-ds-h */
        0x00, 0x00, 0x00, 0x00,
        /* cha-sdl, cha-sdh*/
        0x21, 0x00, 0x00, 0x00,
        /* cha-hs-pwl, cha-hs-pwh */
        0x01, 0x00, 0x00, 0x00,
        /* cha-vs-pwl, cha-vs-pwh */
        0x01, 0x00, 0x00, 0x00,
        /*cha-hbp, cha-vbp */
        0x2e, 0x00 ,0x00, 0x00,
        /* cha-hfp, cha-vfp*/
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00 ,0x00,
        0x01
	};

  ret = uclass_get_device_by_seq (UCLASS_I2C, i2c_bus, &bus);
  if (ret)
    {
      printf ("%s: No bus %d\n", __func__, i2c_bus);
      return 1;
    }

  ret = dm_i2c_probe (bus, 0x2c, 0, &mipi2lvds_dev);
  if (ret)
    {
      printf ("%s: Can't find device id=0x%x, on bus %d, ret %d\n", __func__,
    		  0x2c, i2c_bus, ret);
      return 1;
    }

  /* offset */
  i2c_set_chip_offset_len (mipi2lvds_dev, 1);

  for(int i = 0;i<(sizeof(sn65_values)/sizeof(sn65_values[0]));i++){
	 tc358764_i2c_reg_write (mipi2lvds_dev, sn65_addr[i], &sn65_values[i], sizeof(sn65_values[i]));
	 mdelay(10);
  }
  mdelay(10);
  val[0] = 1;
  tc358764_i2c_reg_write (mipi2lvds_dev, 0x9, &val[0], sizeof(val[0]));
  return 0;
}

static const struct sec_mipi_dsim_plat_data imx8mm_mipi_dsim_plat_data = {
	.version	= 0x1060200,
	.max_data_lanes = 4,
	.max_data_rate  = 1500000000ULL,
	.reg_base = MIPI_DSI_BASE_ADDR,
	.gpr_base = CSI_BASE_ADDR + 0x8000,
	.dphy_pll	= &pll_1432x,
	.dphy_timing	= dphy_timing_ln14lpp_v1p2,
	.num_dphy_timing = ARRAY_SIZE(dphy_timing_ln14lpp_v1p2),
	.dphy_timing_cmp = dphy_timing_default_cmp,
};

#define DISPLAY_MIX_SFT_RSTN_CSR		0x00
#define DISPLAY_MIX_CLK_EN_CSR		0x04

/* 'DISP_MIX_SFT_RSTN_CSR' bit fields */
#define BUS_RSTN_BLK_SYNC_SFT_EN	BIT(6)

/* 'DISP_MIX_CLK_EN_CSR' bit fields */
#define LCDIF_PIXEL_CLK_SFT_EN		BIT(7)
#define LCDIF_APB_CLK_SFT_EN		BIT(6)

void disp_mix_bus_rstn_reset(ulong gpr_base, bool reset)
{
	if (!reset)
		/* release reset */
		setbits_le32 (gpr_base + DISPLAY_MIX_SFT_RSTN_CSR,
			      BUS_RSTN_BLK_SYNC_SFT_EN);
	else
		/* hold reset */
		clrbits_le32 (gpr_base + DISPLAY_MIX_SFT_RSTN_CSR,
			      BUS_RSTN_BLK_SYNC_SFT_EN);
}

void disp_mix_lcdif_clks_enable(ulong gpr_base, bool enable)
{
	if (enable)
		/* enable lcdif clks */
		setbits_le32 (gpr_base + DISPLAY_MIX_CLK_EN_CSR,
			      LCDIF_PIXEL_CLK_SFT_EN | LCDIF_APB_CLK_SFT_EN);
	else
		/* disable lcdif clks */
		clrbits_le32 (gpr_base + DISPLAY_MIX_CLK_EN_CSR,
			      LCDIF_PIXEL_CLK_SFT_EN | LCDIF_APB_CLK_SFT_EN);
}

struct mipi_dsi_client_dev tc358764_dev = {
	.channel	= 0,
	.lanes = 4,
	.format  = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
	MIPI_DSI_MODE_VIDEO_AUTO_VERT,
	.name = "TC358764",
};

struct mipi_dsi_client_dev g050tan01_dev = {
	.channel	= 0,
	.lanes = 4,
	.format  = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO
	| MIPI_DSI_MODE_EOT_PACKET | MIPI_DSI_MODE_VIDEO_SYNC_PULSE,
};

int detect_tc358764(struct display_info_t const *dev)
{
	return (fs_board_get_features() & FEAT_LVDS) ? 1 : 0;
}

int detect_mipi_disp(struct display_info_t const *dev)
{
	return (fs_board_get_features() & FEAT_MIPI_DSI) ? 1 : 0;
}

static struct mipi_dsi_client_driver tc358764_drv = {
	.name = "TC358764",
	.dsi_client_setup = tc358764_setup,
};

void tc358764_init(void)
{
	imx_mipi_dsi_bridge_add_client_driver(&tc358764_drv);
}

void enable_tc358764(struct display_info_t const *dev)
{
	int ret = 0;
	imx_iomux_v3_setup_multiple_pads (lvds_clk_8mm_pads, ARRAY_SIZE (lvds_clk_8mm_pads));
	gpio_request (LVDS_CLK_8MM_PAD, "LVDS_CLK");
	gpio_direction_output (LVDS_CLK_8MM_PAD, 0);

#if 0
	mxs_set_lcdclk(dev->bus, PICOS2KHZ(dev->mode.pixclock));

	clock_set_target_val (IPP_DO_CLKO2, CLK_ROOT_ON
			      | CLK_ROOT_SOURCE_SEL(1) | CLK_ROOT_POST_DIV(CLK_ROOT_POST_DIV5));
#endif
	switch (fs_board_get_type()) 
	{
	case BT_PICOCOREMX8MM:
		imx_iomux_v3_setup_multiple_pads (lvds_rst_8mm_pads, ARRAY_SIZE (lvds_rst_8mx_pads));
		gpio_request (LVDS_RST_8MM_PAD, "LVDS_RST");
		gpio_direction_output (LVDS_RST_8MM_PAD, 0);
		/* period of reset signal > 50 ns */
		udelay (5);
		gpio_direction_output (LVDS_RST_8MM_PAD, 1);
		break;
	case BT_PICOCOREMX8MX:
		imx_iomux_v3_setup_multiple_pads (lvds_rst_8mx_pads, ARRAY_SIZE (lvds_rst_8mx_pads));
		gpio_request (LVDS_STBY_8MX_PAD, "LVDS_STBY");
		gpio_direction_output (LVDS_STBY_8MX_PAD, 1);
		udelay (50);
		gpio_request (LVDS_RST_8MX_PAD, "LVDS_RST");
		gpio_direction_output (LVDS_RST_8MX_PAD, 0);
		/* period of reset signal > 50 ns */
		udelay (5);
		gpio_direction_output (LVDS_RST_8MX_PAD, 1);
		break;
	}

	udelay (500);

	/* enable the dispmix & mipi phy power domain */
	call_imx_sip (FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
	call_imx_sip (FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

	/* Put lcdif out of reset */
	disp_mix_bus_rstn_reset (imx8mm_mipi_dsim_plat_data.gpr_base, false);
	disp_mix_lcdif_clks_enable (imx8mm_mipi_dsim_plat_data.gpr_base, true);

	/* Setup mipi dsim */
	ret = sec_mipi_dsim_setup (&imx8mm_mipi_dsim_plat_data);
	if (ret)
		return;
	tc358764_init();
	tc358764_dev.name = displays[1].mode.name;
	ret = imx_mipi_dsi_bridge_attach (&tc358764_dev); /* attach tc358764 device */
}

void enable_sn65dsi84(struct display_info_t const *dev)
{
  int ret = 0;

  imx_iomux_v3_setup_multiple_pads (mipi_rst_pads, ARRAY_SIZE (mipi_rst_pads));

  gpio_request(MIPI_RST_PAD, "MIPI_RST");

  /* period of reset signal > 50 ns */
  gpio_direction_output (MIPI_RST_PAD, 1);

  mdelay(50);

  sn65dsi84_init();
  /* enable the dispmix & mipi phy power domain */
  call_imx_sip (FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
  call_imx_sip (FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

  /* Put lcdif out of reset */
  disp_mix_bus_rstn_reset (imx8mm_mipi_dsim_plat_data.gpr_base, false);
  disp_mix_lcdif_clks_enable (imx8mm_mipi_dsim_plat_data.gpr_base, true);

  /* Setup mipi dsim */
  ret = sec_mipi_dsim_setup (&imx8mm_mipi_dsim_plat_data);

  if (ret)
    return;

  ret = imx_mipi_dsi_bridge_attach (&tc358764_dev); /* attach tc358764 device */
}

void enable_mipi_disp(struct display_info_t const *dev)
{
	/* enable the dispmix & mipi phy power domain */
	call_imx_sip (FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
	call_imx_sip (FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

	/* Put lcdif out of reset */
	disp_mix_bus_rstn_reset (imx8mm_mipi_dsim_plat_data.gpr_base, false);
	disp_mix_lcdif_clks_enable (imx8mm_mipi_dsim_plat_data.gpr_base, true);

	/* Setup mipi dsim */
	sec_mipi_dsim_setup (&imx8mm_mipi_dsim_plat_data);

	nt35521_init ();
	g050tan01_dev.name = displays[0].mode.name;
	imx_mipi_dsi_bridge_attach(&g050tan01_dev); /* attach g050tan01 device */
}

void board_quiesce_devices(void)
{
	gpio_request (IMX_GPIO_NR(1, 13), "DSI EN");
	gpio_direction_output (IMX_GPIO_NR(1, 13), 0);
}

struct display_info_t const displays[] = {
	{
		.bus = LCDIF_BASE_ADDR,
		.addr = 0,
		.pixfmt = 24,
		.detect = detect_mipi_disp,
		.enable	= enable_sn65dsi84,
		.mode	= {
			.name			= "SN65DSI84",
			.refresh		= 60,
			.xres			= 800,
			.yres			= 480,
			.pixclock		= 29850, // 10^12/freq
			.left_margin	= 37,
			.right_margin	= 21,
			.hsync_len		= 10,
			.upper_margin	= 22,
			.lower_margin	= 23,
			.vsync_len		= 1,
			.sync			= FB_SYNC_EXT,
			.vmode			= FB_VMODE_NONINTERLACED
		}
	},
	{	
		.bus = LCDIF_BASE_ADDR,
		.addr = 0,
		.pixfmt = 24,
		.detect = detect_tc358764,
		.enable	= enable_tc358764,
		.mode	= {
			.name			= "TC358764",
			.refresh		= 60,
			.xres			= 800,
			.yres			= 480,
			.pixclock		= 29850, // 10^12/freq
			.left_margin	= 46,
			.right_margin	= 210,
			.hsync_len		= 10,
			.upper_margin	= 21,
			.lower_margin	= 24,
			.vsync_len		= 3,
			.sync			= FB_SYNC_EXT,
			.vmode			= FB_VMODE_NONINTERLACED
		}
	},
};
size_t display_count = ARRAY_SIZE(displays);
#endif /* CONFIG_VIDEO_MXS */

