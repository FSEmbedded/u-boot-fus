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
#include <linux/bitfield.h>
#include <dm.h>
#include "../common/fs_fdt_common.h"	/* fs_fdt_set_val(), ... */
#include "../common/fs_board_common.h"	/* fs_board_*() */
#include "sec_mipi_dphy_ln14lpp.h"
#include "sec_mipi_pll_1432x.h"

#define BT_PICOCOREMX8MM 	0
#define BT_PICOCOREMX8MX	1
#define BT_TBS2 		2

#define FEAT_LVDS	(1<<8)
#define FEAT_MIPI_DSI	(1<<9)

#ifdef CONFIG_VIDEO_MXS

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				9
#define MIPI				10


/* tc358775.c start ---------------------------------- */

#define MEDIA_BUS_FMT_RGB888_1X7X4_SPWG		0x1011
#define MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA	0x1012

#define FLD_VAL(val, start, end) FIELD_PREP(GENMASK(start, end), val)

/* Registers */

/* DSI D-PHY Layer Registers */
#define D0W_DPHYCONTTX  0x0004  /* Data Lane 0 DPHY Tx Control */
#define CLW_DPHYCONTRX  0x0020  /* Clock Lane DPHY Rx Control */
#define D0W_DPHYCONTRX  0x0024  /* Data Lane 0 DPHY Rx Control */
#define D1W_DPHYCONTRX  0x0028  /* Data Lane 1 DPHY Rx Control */
#define D2W_DPHYCONTRX  0x002C  /* Data Lane 2 DPHY Rx Control */
#define D3W_DPHYCONTRX  0x0030  /* Data Lane 3 DPHY Rx Control */
#define COM_DPHYCONTRX  0x0038  /* DPHY Rx Common Control */
#define CLW_CNTRL       0x0040  /* Clock Lane Control */
#define D0W_CNTRL       0x0044  /* Data Lane 0 Control */
#define D1W_CNTRL       0x0048  /* Data Lane 1 Control */
#define D2W_CNTRL       0x004C  /* Data Lane 2 Control */
#define D3W_CNTRL       0x0050  /* Data Lane 3 Control */
#define DFTMODE_CNTRL   0x0054  /* DFT Mode Control */

/* DSI PPI Layer Registers */
#define PPI_STARTPPI    0x0104  /* START control bit of PPI-TX function. */
#define PPI_START_FUNCTION      1

#define PPI_BUSYPPI     0x0108
#define PPI_LINEINITCNT 0x0110  /* Line Initialization Wait Counter  */
#define PPI_LPTXTIMECNT 0x0114
#define PPI_LANEENABLE  0x0134  /* Enables each lane at the PPI layer. */
#define PPI_TX_RX_TA    0x013C  /* DSI Bus Turn Around timing parameters */

/* Analog timer function enable */
#define PPI_CLS_ATMR    0x0140  /* Delay for Clock Lane in LPRX  */
#define PPI_D0S_ATMR    0x0144  /* Delay for Data Lane 0 in LPRX */
#define PPI_D1S_ATMR    0x0148  /* Delay for Data Lane 1 in LPRX */
#define PPI_D2S_ATMR    0x014C  /* Delay for Data Lane 2 in LPRX */
#define PPI_D3S_ATMR    0x0150  /* Delay for Data Lane 3 in LPRX */

#define PPI_D0S_CLRSIPOCOUNT    0x0164  /* For lane 0 */
#define PPI_D1S_CLRSIPOCOUNT    0x0168  /* For lane 1 */
#define PPI_D2S_CLRSIPOCOUNT    0x016C  /* For lane 2 */
#define PPI_D3S_CLRSIPOCOUNT    0x0170  /* For lane 3 */

#define CLS_PRE         0x0180  /* Digital Counter inside of PHY IO */
#define D0S_PRE         0x0184  /* Digital Counter inside of PHY IO */
#define D1S_PRE         0x0188  /* Digital Counter inside of PHY IO */
#define D2S_PRE         0x018C  /* Digital Counter inside of PHY IO */
#define D3S_PRE         0x0190  /* Digital Counter inside of PHY IO */
#define CLS_PREP        0x01A0  /* Digital Counter inside of PHY IO */
#define D0S_PREP        0x01A4  /* Digital Counter inside of PHY IO */
#define D1S_PREP        0x01A8  /* Digital Counter inside of PHY IO */
#define D2S_PREP        0x01AC  /* Digital Counter inside of PHY IO */
#define D3S_PREP        0x01B0  /* Digital Counter inside of PHY IO */
#define CLS_ZERO        0x01C0  /* Digital Counter inside of PHY IO */
#define D0S_ZERO        0x01C4  /* Digital Counter inside of PHY IO */
#define D1S_ZERO        0x01C8  /* Digital Counter inside of PHY IO */
#define D2S_ZERO        0x01CC  /* Digital Counter inside of PHY IO */
#define D3S_ZERO        0x01D0  /* Digital Counter inside of PHY IO */

#define PPI_CLRFLG      0x01E0  /* PRE Counters has reached set values */
#define PPI_CLRSIPO     0x01E4  /* Clear SIPO values, Slave mode use only. */
#define HSTIMEOUT       0x01F0  /* HS Rx Time Out Counter */
#define HSTIMEOUTENABLE 0x01F4  /* Enable HS Rx Time Out Counter */
#define DSI_STARTDSI    0x0204  /* START control bit of DSI-TX function */
#define DSI_RX_START	1

#define DSI_BUSYDSI     0x0208
#define DSI_LANEENABLE  0x0210  /* Enables each lane at the Protocol layer. */
#define DSI_LANESTATUS0 0x0214  /* Displays lane is in HS RX mode. */
#define DSI_LANESTATUS1 0x0218  /* Displays lane is in ULPS or STOP state */

#define DSI_INTSTATUS   0x0220  /* Interrupt Status */
#define DSI_INTMASK     0x0224  /* Interrupt Mask */
#define DSI_INTCLR      0x0228  /* Interrupt Clear */
#define DSI_LPTXTO      0x0230  /* Low Power Tx Time Out Counter */

#define DSIERRCNT       0x0300  /* DSI Error Count */
#define APLCTRL         0x0400  /* Application Layer Control */
#define RDPKTLN         0x0404  /* Command Read Packet Length */

#define VPCTRL          0x0450  /* Video Path Control */
#define HTIM1           0x0454  /* Horizontal Timing Control 1 */
#define HTIM2           0x0458  /* Horizontal Timing Control 2 */
#define VTIM1           0x045C  /* Vertical Timing Control 1 */
#define VTIM2           0x0460  /* Vertical Timing Control 2 */
#define VFUEN           0x0464  /* Video Frame Timing Update Enable */
#define VFUEN_EN	BIT(0)  /* Upload Enable */

/* Mux Input Select for LVDS LINK Input */
#define LV_MX0003        0x0480  /* Bit 0 to 3 */
#define LV_MX0407        0x0484  /* Bit 4 to 7 */
#define LV_MX0811        0x0488  /* Bit 8 to 11 */
#define LV_MX1215        0x048C  /* Bit 12 to 15 */
#define LV_MX1619        0x0490  /* Bit 16 to 19 */
#define LV_MX2023        0x0494  /* Bit 20 to 23 */
#define LV_MX2427        0x0498  /* Bit 24 to 27 */
#define LV_MX(b0, b1, b2, b3)	(FLD_VAL(b0, 4, 0) | FLD_VAL(b1, 12, 8) | \
				FLD_VAL(b2, 20, 16) | FLD_VAL(b3, 28, 24))

/* Input bit numbers used in mux registers */
enum {
	LVI_R0,
	LVI_R1,
	LVI_R2,
	LVI_R3,
	LVI_R4,
	LVI_R5,
	LVI_R6,
	LVI_R7,
	LVI_G0,
	LVI_G1,
	LVI_G2,
	LVI_G3,
	LVI_G4,
	LVI_G5,
	LVI_G6,
	LVI_G7,
	LVI_B0,
	LVI_B1,
	LVI_B2,
	LVI_B3,
	LVI_B4,
	LVI_B5,
	LVI_B6,
	LVI_B7,
	LVI_HS,
	LVI_VS,
	LVI_DE,
	LVI_L0
};

#define LVCFG           0x049C  /* LVDS Configuration  */
#define LVPHY0          0x04A0  /* LVDS PHY 0 */
#define LV_PHY0_RST(v)          FLD_VAL(v, 22, 22) /* PHY reset */
#define LV_PHY0_IS(v)           FLD_VAL(v, 15, 14)
#define LV_PHY0_ND(v)           FLD_VAL(v, 4, 0) /* Frequency range select */
#define LV_PHY0_PRBS_ON(v)      FLD_VAL(v, 20, 16) /* Clock/Data Flag pins */

#define LVPHY1          0x04A4  /* LVDS PHY 1 */
#define SYSSTAT         0x0500  /* System Status  */
#define SYSRST          0x0504  /* System Reset  */

#define SYS_RST_I2CS	BIT(0) /* Reset I2C-Slave controller */
#define SYS_RST_I2CM	BIT(1) /* Reset I2C-Master controller */
#define SYS_RST_LCD	BIT(2) /* Reset LCD controller */
#define SYS_RST_BM	BIT(3) /* Reset Bus Management controller */
#define SYS_RST_DSIRX	BIT(4) /* Reset DSI-RX and App controller */
#define SYS_RST_REG	BIT(5) /* Reset Register module */

/* GPIO Registers */
#define GPIOC           0x0520  /* GPIO Control  */
#define GPIOO           0x0524  /* GPIO Output  */
#define GPIOI           0x0528  /* GPIO Input  */

/* I2C Registers */
#define I2CTIMCTRL      0x0540  /* I2C IF Timing and Enable Control */
#define I2CMADDR        0x0544  /* I2C Master Addressing */
#define WDATAQ          0x0548  /* Write Data Queue */
#define RDATAQ          0x054C  /* Read Data Queue */

/* Chip ID and Revision ID Register */
#define IDREG           0x0580

#define LPX_PERIOD		4
#define TTA_GET			0x40000
#define TTA_SURE		6
#define SINGLE_LINK		1
#define DUAL_LINK		2

#define TC358775XBG_ID  0x00007500

/* Debug Registers */
#define DEBUG00         0x05A0  /* Debug */
#define DEBUG01         0x05A4  /* LVDS Data */

#define DSI_CLEN_BIT		BIT(0)
#define DIVIDE_BY_3		3 /* PCLK=DCLK/3 */
#define DIVIDE_BY_6		6 /* PCLK=DCLK/6 */
#define LVCFG_LVEN_BIT		BIT(0)

#define L0EN BIT(1)

#define TC358775_VPCTRL_VSDELAY__MASK	0x3FF00000
#define TC358775_VPCTRL_VSDELAY__SHIFT	20
static inline u32 TC358775_VPCTRL_VSDELAY(uint32_t val)
{
	return ((val) << TC358775_VPCTRL_VSDELAY__SHIFT) &
			TC358775_VPCTRL_VSDELAY__MASK;
}

#define TC358775_VPCTRL_OPXLFMT__MASK	0x00000100
#define TC358775_VPCTRL_OPXLFMT__SHIFT	8
static inline u32 TC358775_VPCTRL_OPXLFMT(uint32_t val)
{
	return ((val) << TC358775_VPCTRL_OPXLFMT__SHIFT) &
			TC358775_VPCTRL_OPXLFMT__MASK;
}

#define TC358775_VPCTRL_MSF__MASK	0x00000001
#define TC358775_VPCTRL_MSF__SHIFT	0
static inline u32 TC358775_VPCTRL_MSF(uint32_t val)
{
	return ((val) << TC358775_VPCTRL_MSF__SHIFT) &
			TC358775_VPCTRL_MSF__MASK;
}

#define TC358775_LVCFG_PCLKDIV__MASK	0x000000f0
#define TC358775_LVCFG_PCLKDIV__SHIFT	4
static inline u32 TC358775_LVCFG_PCLKDIV(uint32_t val)
{
	return ((val) << TC358775_LVCFG_PCLKDIV__SHIFT) &
			TC358775_LVCFG_PCLKDIV__MASK;
}

#define TC358775_LVCFG_LVDLINK__MASK                         0x00000002
#define TC358775_LVCFG_LVDLINK__SHIFT                        0
static inline u32 TC358775_LVCFG_LVDLINK(uint32_t val)
{
	return ((val) << TC358775_LVCFG_LVDLINK__SHIFT) &
			TC358775_LVCFG_LVDLINK__MASK;
}

/* tc358775.c end ------------------------------------ */


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


#define TC358775_ADDR 0xF
#define PCA9634_ADDR 0x61

static int i2c_bus = 0;
static struct display_info_t const *display;

static int tc358775_i2c_reg_write(struct udevice *dev, uint addr, uint32_t *data, int length)
{
	int err;

	err = dm_i2c_write (dev, addr, (uint8_t*)data, length);
	return err;
}

static int tc358775_i2c_reg_write_u32(struct udevice *dev, uint addr, uint32_t data)
{
	int err;

	err = tc358775_i2c_reg_write(dev, addr, &data, sizeof(uint32_t));
	return err;
}

static int tc358775_i2c_reg_read(struct udevice *dev, uint addr, uint32_t *data, int length)
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


int tc358775_setup(struct mipi_dsi_client_dev * dev)
{
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
	int ret;
	uint32_t hback_porch, hsync_len, hfront_porch, hactive, htime1, htime2;
	uint32_t vback_porch, vsync_len, vfront_porch, vactive, vtime1, vtime2;
	uint32_t val = 0;
	uint16_t dsiclk, clkdiv, byteclk, t1, t2, t3, vsdelay;
	uint32_t bpc;

	ret = uclass_get_device_by_seq (UCLASS_I2C, i2c_bus, &bus);
	if (ret)
	{
		printf ("%s: No bus %d\n", __func__, i2c_bus);
		return 1;
	}

	ret = dm_i2c_probe (bus, TC358775_ADDR, 0, &mipi2lvds_dev);
	if (ret)
	{
		printf ("%s: Can't find device id=0x%x, on bus %d, ret %d\n", __func__,
			TC358775_ADDR, i2c_bus, ret);
		return 1;
	}

	/* offset */
	i2c_set_chip_offset_len (mipi2lvds_dev, 2);

	/* read chip/rev register with */
	tc358775_i2c_reg_read (mipi2lvds_dev, SYS_ID, &val, sizeof(val));

	if (((val >> 8) & 0xFF) == 0x75)
		printf ("DSI2LVDS:  TC358775 Rev. 0x%x.\n", (uint8_t) (val & 0xFF));
	else
		printf ("DSI2LVDS:  ID: 0x%x Rev. 0x%x.\n", (uint8_t) ((val >> 8) & 0xFF),
			(uint8_t) (val & 0xFF));

	udelay(200);

	switch (dev->format) {
	case MIPI_DSI_FMT_RGB888:
		/* RGB888 */
		bpc = 8;
		break;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
	case MIPI_DSI_FMT_RGB565:
		/* RGB666 */
		bpc = 6;
		break;
	default:
		bpc = 8;
	}

	hback_porch = display->mode.left_margin;
	hsync_len  = display->mode.hsync_len;
	vback_porch = display->mode.upper_margin;
	vsync_len  = display->mode.vsync_len;

	htime1 = (hback_porch << 16) + hsync_len;
	vtime1 = (vback_porch << 16) + vsync_len;

	hfront_porch = display->mode.right_margin;
	hactive = display->mode.xres;
	vfront_porch = display->mode.lower_margin;
	vactive = display->mode.yres;

	htime2 = (hfront_porch << 16) + hactive;
	vtime2 = (vfront_porch << 16) + vactive;

	val = SYS_RST_REG | SYS_RST_DSIRX | SYS_RST_BM | SYS_RST_LCD;
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, SYSRST, val);

	mdelay(30);

	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_TX_RX_TA, TTA_GET | TTA_SURE);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_LPTXTIMECNT, LPX_PERIOD);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_D0S_CLRSIPOCOUNT, 3);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_D1S_CLRSIPOCOUNT, 3);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_D2S_CLRSIPOCOUNT, 3);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_D3S_CLRSIPOCOUNT, 3);

	val = ((L0EN << dev->lanes) - L0EN) | DSI_CLEN_BIT;
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_LANEENABLE, val);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, DSI_LANEENABLE, val);

	tc358775_i2c_reg_write_u32(mipi2lvds_dev, PPI_STARTPPI, PPI_START_FUNCTION);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, DSI_STARTDSI, DSI_RX_START);

	if (bpc == 8)
		val = TC358775_VPCTRL_OPXLFMT(1);
	else /* bpc = 6; */
		val = TC358775_VPCTRL_MSF(1);

	dsiclk = PICOS2KHZ(display->mode.pixclock) * 3 * bpc / dev->lanes / 1000;
	clkdiv = dsiclk / DIVIDE_BY_3 * 1;
	byteclk = dsiclk / 4;
	t1 = hactive * (bpc * 3 / 8) / dev->lanes;
	t2 = ((100000 / clkdiv)) * (hactive + hback_porch + hsync_len + hfront_porch) / 1000;
	t3 = ((t2 * byteclk) / 100) - (hactive * (bpc * 3 / 8) /
		dev->lanes);

	vsdelay = (clkdiv * (t1 + t3) / byteclk) - hback_porch - hsync_len - hactive;

	val |= TC358775_VPCTRL_VSDELAY(vsdelay);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, VPCTRL, val);

	tc358775_i2c_reg_write_u32(mipi2lvds_dev, HTIM1, htime1);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, VTIM1, vtime1);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, HTIM2, htime2);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, VTIM2, vtime2);

	tc358775_i2c_reg_write_u32(mipi2lvds_dev, VFUEN, VFUEN_EN);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, SYSRST, SYS_RST_LCD);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, LVPHY0, LV_PHY0_PRBS_ON(4) | LV_PHY0_ND(6));

	/*
	 * Default hardware register settings of tc358775 configured
	 * with MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA jeida-24 format
	 */
	if (display->mode.flag != MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA) {
		if (display->mode.flag == MEDIA_BUS_FMT_RGB888_1X7X4_SPWG) {
			/* VESA-24 */
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX0003, LV_MX(LVI_R0, LVI_R1, LVI_R2, LVI_R3));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX0407, LV_MX(LVI_R4, LVI_R7, LVI_R5, LVI_G0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX0811, LV_MX(LVI_G1, LVI_G2, LVI_G6, LVI_G7));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX1215, LV_MX(LVI_G3, LVI_G4, LVI_G5, LVI_B0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX1619, LV_MX(LVI_B6, LVI_B7, LVI_B1, LVI_B2));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX2023, LV_MX(LVI_B3, LVI_B4, LVI_B5, LVI_L0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX2427, LV_MX(LVI_HS, LVI_VS, LVI_DE, LVI_R6));
		} else { /*  MEDIA_BUS_FMT_RGB666_1X7X3_SPWG - JEIDA-18 */
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX0003, LV_MX(LVI_R0, LVI_R1, LVI_R2, LVI_R3));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX0407, LV_MX(LVI_R4, LVI_L0, LVI_R5, LVI_G0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX0811, LV_MX(LVI_G1, LVI_G2, LVI_L0, LVI_L0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX1215, LV_MX(LVI_G3, LVI_G4, LVI_G5, LVI_B0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX1619, LV_MX(LVI_L0, LVI_L0, LVI_B1, LVI_B2));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX2023, LV_MX(LVI_B3, LVI_B4, LVI_B5, LVI_L0));
			tc358775_i2c_reg_write_u32(mipi2lvds_dev, LV_MX2427, LV_MX(LVI_HS, LVI_VS, LVI_DE, LVI_L0));
		}
	}

	tc358775_i2c_reg_write_u32(mipi2lvds_dev, VFUEN, VFUEN_EN);

	val = LVCFG_LVEN_BIT | TC358775_LVCFG_PCLKDIV(DIVIDE_BY_3);
	tc358775_i2c_reg_write_u32(mipi2lvds_dev, LVCFG, val);

	return ret;
}

static int sn65dsi84_init(void)
{
  struct udevice *bus = 0, *mipi2lvds_dev = 0;
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
	 tc358775_i2c_reg_write (mipi2lvds_dev, sn65_addr[i], &sn65_values[i], sizeof(sn65_values[i]));
	 mdelay(10);
  }
  mdelay(10);
  val[0] = 1;
  tc358775_i2c_reg_write (mipi2lvds_dev, 0x9, &val[0], sizeof(val[0]));
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

struct mipi_dsi_client_dev tc358775_dev = {
	.channel	= 0,
	.lanes = 4,
	.format  = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST,
	.name = "TC358775",
};

int detect_tc358775(struct display_info_t const *dev)
{
	return (fs_board_get_features() & (FEAT_LVDS | FEAT_MIPI_DSI)) ? 1 : 0;
}

int detect_mipi_disp(struct display_info_t const *dev)
{
	return 0;
}

static struct mipi_dsi_client_driver tc358775_drv = {
	.name = "TC358775",
	.dsi_client_setup = tc358775_setup,
};

void tc358775_init(void)
{
	imx_mipi_dsi_bridge_add_client_driver(&tc358775_drv);
}

#define LED(X) (1<<(X<<1))
/* Reg 0x1 */
#define OUTDRV (1<<2)
/* Reg 0xC */
#define BL_PWM LED(1)
#define MIPI_RST LED(3)
/* Reg 0xD */
#define MIPI_STBY LED(0)

void enable_tc358775(struct display_info_t const *dev)
{
	int ret = 0;

	if (fs_board_get_type() != BT_TBS2)
	{
		display = dev;

		if (detect_mipi_disp(dev))
		{
			struct udevice *bus = 0, *gpio_dev = 0;
			uint8_t val = 0;

			i2c_bus = 1;

			ret = uclass_get_device_by_seq (UCLASS_I2C, i2c_bus, &bus);
			if (ret)
			{
				printf ("%s: No bus %d\n", __func__, i2c_bus);
				return;
			}

			ret = dm_i2c_probe (bus, PCA9634_ADDR, 0, &gpio_dev);
			if (ret)
			{
				printf ("%s: Can't find device id=0x%x, on bus %d, ret %d\n", __func__,
					PCA9634_ADDR, i2c_bus, ret);
				return;
			}

			/* offset */
			i2c_set_chip_offset_len (gpio_dev, 1);

			udelay(50);
			/* Set from low power mode to normal mode, otherwise LEDs can't be set */
			val = 0;
			dm_i2c_write (gpio_dev, 0x0, &val, sizeof(val));
			udelay (5);
			val = OUTDRV;
			dm_i2c_write (gpio_dev, 0x1, &val, sizeof(val));
			val = BL_PWM | MIPI_RST;
			dm_i2c_write (gpio_dev, 0xC, &val, sizeof(val));
			val = MIPI_STBY;
			dm_i2c_write (gpio_dev, 0xD, &val, sizeof(val));
			mdelay (10);
			val = 0;
			dm_i2c_write (gpio_dev, 0xD, &val, sizeof(val));
			mdelay (10);
			val = BL_PWM;
			dm_i2c_write (gpio_dev, 0xC, &val, sizeof(val));
			udelay (10);
		}
		else
		{
			imx_iomux_v3_setup_multiple_pads (lvds_clk_8mm_pads, ARRAY_SIZE (lvds_clk_8mm_pads));
			gpio_request (LVDS_CLK_8MM_PAD, "LVDS_CLK");
			gpio_direction_output (LVDS_CLK_8MM_PAD, 0);

			switch (fs_board_get_type())
			{
			case BT_PICOCOREMX8MM:
				i2c_bus = 3;
				imx_iomux_v3_setup_multiple_pads (lvds_rst_8mm_pads, ARRAY_SIZE (lvds_rst_8mm_pads));
				gpio_request (LVDS_RST_8MM_PAD, "LVDS_RST");
				gpio_direction_output (LVDS_RST_8MM_PAD, 0);
				mdelay (10);
				gpio_direction_output (LVDS_RST_8MM_PAD, 1);
				udelay (10);
				break;
			case BT_PICOCOREMX8MX:
				i2c_bus = 0;
				imx_iomux_v3_setup_multiple_pads (lvds_rst_8mx_pads, ARRAY_SIZE (lvds_rst_8mx_pads));
				gpio_request (LVDS_STBY_8MX_PAD, "LVDS_STBY");
				gpio_request (LVDS_RST_8MX_PAD, "LVDS_RST");
				gpio_direction_output (LVDS_STBY_8MX_PAD, 0);
				gpio_direction_output (LVDS_RST_8MX_PAD, 0);
				mdelay (10);
				gpio_direction_output (LVDS_STBY_8MX_PAD, 1);
				mdelay (10);
				gpio_direction_output (LVDS_RST_8MX_PAD, 1);
				udelay (10);
				break;
			}
		}

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
		tc358775_init();
		tc358775_dev.name = display->mode.name;
		ret = imx_mipi_dsi_bridge_attach (&tc358775_dev); /* attach tc358775 device */
	}
}

void enable_sn65dsi84(struct display_info_t const *dev)
{
  int ret = 0;

  i2c_bus = 1;
  display = dev;

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

  ret = imx_mipi_dsi_bridge_attach (&tc358775_dev); /* attach tc358775 device */
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
		.detect = detect_tc358775,
		.enable	= enable_tc358775,
		.mode	= {
			.name			= "TC358775",
			.refresh		= 60,
			.xres			= 800,
			.yres			= 480,
			.pixclock		= 29762, // 10^12/freq
			.left_margin	= 42,
			.right_margin	= 210,
			.hsync_len		= 4,
			.upper_margin	= 22,
			.lower_margin	= 22,
			.vsync_len		= 1,
			.sync			= FB_SYNC_EXT,
			.vmode			= FB_VMODE_NONINTERLACED,
			.flag			= MEDIA_BUS_FMT_RGB888_1X7X4_SPWG
		}
	},
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
};
size_t display_count = ARRAY_SIZE(displays);
#endif /* CONFIG_VIDEO_MXS */

