#include <common.h>
#include <dm/device.h>
#include <dm.h>
#include <linux/delay.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h> // LPUART_BASE_ADDR
#include <asm/mach-imx/iomux-v3.h>
#include "../common/fus_sai.h"
#include "../common/fsl_sgtl5000.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define TEST_SAI ((I2S_Type *)0x30010000)

/*set Bclk source to Mclk clock*/
#define TEST_SAI_CLOCK_SOURCE (1U)

#define TEST_SAI_CLK_FREQ   24576000
#define OVER_SAMPLE_RATE (384U)
#define TEST_AUDIO_SAMPLE_RATE (kSAI_SampleRate32KHz)
#define TEST_AUDIO_MASTER_CLOCK OVER_SAMPLE_RATE *TEST_AUDIO_SAMPLE_RATE

/* demo audio data channel */
#define TEST_AUDIO_DATA_CHANNEL (2U)
/* demo audio bit width */
#define TEST_AUDIO_BIT_WIDTH kSAI_WordWidth16bits

#ifndef TEST_SAI_TX_SYNC_MODE
#define TEST_SAI_TX_SYNC_MODE kSAI_ModeAsync
#endif
#ifndef TEST_SAI_RX_SYNC_MODE
#define TEST_SAI_RX_SYNC_MODE kSAI_ModeSync
#endif

#define LOCK_STATUS 	BIT(31)
#define LOCK_SEL_MASK	BIT(29)
#define RST_MASK	BIT(9)
#define BYPASS_MASK	BIT(4)
#define	MDIV_SHIFT	12
#define	MDIV_MASK	GENMASK(21, 12)
#define PDIV_SHIFT	4
#define PDIV_MASK	GENMASK(9, 4)
#define SDIV_SHIFT	0
#define SDIV_MASK	GENMASK(2, 0)
#define KDIV_SHIFT	0
#define KDIV_MASK	GENMASK(15, 0)

#define PLL_1443X_RATE(_rate, _m, _p, _s, _k)			\
	{							\
		.rate	=	(_rate),			\
		.mdiv	=	(_m),				\
		.pdiv	=	(_p),				\
		.sdiv	=	(_s),				\
		.kdiv	=	(_k),				\
	}

static struct imx_int_pll_rate_table imx8mm_fracpll_tbl[] = {
		PLL_1443X_RATE(393216000, 262, 2, 3, 9437)
};

static int fracpll_configure_audioPll1(void)
{
	u32 tmp, div_val;
	void *pll_base;
	struct imx_int_pll_rate_table *rate;

	rate = &imx8mm_fracpll_tbl[0];

	pll_base = (void __iomem *)0x30360000;
	/* Bypass clock and set lock to pll output lock */
	tmp = readl(pll_base);
	tmp |= BYPASS_MASK;
	writel(tmp, pll_base);

	/* Enable RST */
	tmp &= ~RST_MASK;
	writel(tmp, pll_base);

	div_val = (rate->mdiv << MDIV_SHIFT) | (rate->pdiv << PDIV_SHIFT) |
		(rate->sdiv << SDIV_SHIFT);
	writel(div_val, pll_base + 4);
	writel(rate->kdiv << KDIV_SHIFT, pll_base + 8);

	__udelay(100);

	/* Disable RST */
	tmp |= RST_MASK;
	writel(tmp, pll_base);

	/* Wait Lock*/
	while (!(readl(pll_base) & LOCK_STATUS))
		;

	/* Bypass */
	tmp &= ~BYPASS_MASK;
	writel(tmp, pll_base);

	return 0;
}

u64 sai_baseaddr(struct udevice *dev) {
	const void *fdt = gd->fdt_blob;
	u32 regvals[4];
	int offset;
	ofnode node;

	/* TODO: Add error handling */

	/* Get offset of the stgl5000 node */
	offset = dev_of_offset(dev);

	/* Get phandle sai = <&saiX>; */
	offset = fdtdec_lookup_phandle(fdt, offset, "sai");

	node = offset_to_ofnode(offset);
	ofnode_read_u32_array(node, "reg", regvals, 4);

	return (u64)regvals[1];
}

sai_transceiver_t config;

int config_sai(struct udevice *dev) {
	I2S_Type * base_addr = (I2S_Type *) sai_baseaddr(dev);
	u32 clk_root, ccgr;

	switch ((u64)base_addr)
	{
		case SAI1_BASE_ADDR:
			clk_root = SAI1_CLK_ROOT;
			ccgr = CCGR_SAI1;
			break;
		case SAI2_BASE_ADDR:
			clk_root = SAI2_CLK_ROOT;
			ccgr = CCGR_SAI2;
			break;
		case SAI3_BASE_ADDR:
			clk_root = SAI3_CLK_ROOT;
			ccgr = CCGR_SAI3;
			break;
		case SAI5_BASE_ADDR:
			clk_root = SAI5_CLK_ROOT;
			ccgr = CCGR_SAI5;
			break;
		case SAI6_BASE_ADDR:
			clk_root = SAI6_CLK_ROOT;
			ccgr = CCGR_SAI6;
			break;
		default:
			clk_root = 0;
			ccgr = 0;
	}

	/* Turn on AUDIO_PLL1 and set to 393215996HZ */
	fracpll_configure_audioPll1();

	/* Set root clock to 393215996HZ / 16 = 24.575999M */
	clock_enable(ccgr,0);
	clock_set_target_val(clk_root,CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(1) | CLK_ROOT_PRE_DIV(CLK_ROOT_PRE_DIV1) | CLK_ROOT_POST_DIV(CLK_ROOT_POST_DIV16));
	clock_enable(ccgr,1);

    SAI_Init(base_addr);

    SAI_GetClassicI2SConfig(&config, TEST_AUDIO_BIT_WIDTH, kSAI_Stereo, kSAI_Channel0Mask);

    config.syncMode = TEST_SAI_TX_SYNC_MODE;
    SAI_TxSetConfig(base_addr, &config);
    config.syncMode = TEST_SAI_RX_SYNC_MODE;
    SAI_RxSetConfig(base_addr, &config);

    /* set bit clock divider */
    SAI_TxSetBitClockRate(base_addr, TEST_AUDIO_MASTER_CLOCK, TEST_AUDIO_SAMPLE_RATE, TEST_AUDIO_BIT_WIDTH,
                          TEST_AUDIO_DATA_CHANNEL);
#if 1
    SAI_RxSetBitClockRate(base_addr, TEST_AUDIO_MASTER_CLOCK, TEST_AUDIO_SAMPLE_RATE, TEST_AUDIO_BIT_WIDTH,
    					  TEST_AUDIO_DATA_CHANNEL);
#endif

    sai_master_clock_t mclkConfig = {
        .mclkOutputEnable = true,
        .mclkHz          = TEST_AUDIO_MASTER_CLOCK,
        .mclkSourceClkHz = TEST_SAI_CLK_FREQ,
    };

    SAI_SetMasterClockConfig(base_addr, &mclkConfig);

    return 0;

}
sgtl_config_t sgtlConfig = {
    .route        = kSGTL_RoutePlaybackandRecord,
    .slaveAddress = SGTL5000_I2C_ADDR,
    .bus          = kSGTL_BusI2S,
    .format = {.mclk_HZ = TEST_AUDIO_MASTER_CLOCK,
    		   .sampleRate = TEST_AUDIO_SAMPLE_RATE,
			   .bitWidth = TEST_AUDIO_BIT_WIDTH},
    .master_slave = false,
};
sgtl_handle_t sgtlHandle;
int config_sgtl(struct udevice *dev){

	int ret;

	sgtlHandle.i2cHandle = dev;
	ret = SGTL_Init(&sgtlHandle, &sgtlConfig);

	// LINEOUT Amplifier Analog Ground Voltag to VDDIO/2
	SGTL_WriteReg(&sgtlHandle, CHIP_LINE_OUT_CTRL , 0x22);
	// Analog Ground Voltage Contro to VDD/2
	SGTL_WriteReg(&sgtlHandle, CHIP_REF_CTRL, 0x01F0U);
	// LINEOUT Left Channel Output Level VDDA = VDDIO 0 3,3V -> 0xF
	SGTL_WriteReg(&sgtlHandle, CHIP_LINE_OUT_VOL, 0x0F0F);
	// DAC Right Channel Volume to 0dB
	SGTL_WriteReg(&sgtlHandle, CHIP_DAC_VOL, 0x3c3c);
	//ADC volume +1.5dB
	SGTL_WriteReg(&sgtlHandle, CHIP_ANA_ADC_CTRL, 0x011U);

	mdelay(500);
	return ret;
}

int run_audioTest(struct udevice *dev,uint8_t* data_send,uint8_t* data_recv, uint32_t len){
	I2S_Type * base_addr = (I2S_Type *) sai_baseaddr(dev);

	/* TODO: Check clocks, so mdelays are not needed anymore */
	SAI_TxEnable(base_addr,true);
	mdelay(50);
	SAI_RxEnable(base_addr,true);
	mdelay(1);

	SAI_WriteReadBlocking(base_addr, config.startChannel,config.channelMask, TEST_AUDIO_BIT_WIDTH, data_send,data_recv, len);

	SAI_RxEnable(base_addr,false);
	SAI_TxEnable(base_addr,false);

	return 0;
}


