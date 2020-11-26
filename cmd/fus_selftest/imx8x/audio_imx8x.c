#include <common.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/sci/sci.h> // SCU Functions
#include <asm/arch/lpcg.h> // Clocks
#include <asm/arch/iomux.h> // IOMUX
#include <asm/arch/imx8-pins.h> // Pins
#include <asm/arch/imx-regs.h> // LPUART_BASE_ADDR
#include "../common/fus_sai.h"
#include "../common/fsl_sgtl5000.h"

DECLARE_GLOBAL_DATA_PTR; // gd for SCU Handle

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define TEST_SAI ((I2S_Type *)0x59040000)

/*set Bclk source to Mclk clock*/
#define TEST_SAI_CLOCK_SOURCE (1U)



#define TEST_SAI_CLK_FREQ   24576000
#define OVER_SAMPLE_RATE (256U)
#define TEST_AUDIO_SAMPLE_RATE (kSAI_SampleRate96KHz)
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

int config_sai(void){

	sai_transceiver_t config;
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	//sc_pm_clock_rate_t rate = 80000000;
	sc_pm_clock_rate_t pll_rate = TEST_SAI_CLK_FREQ;

	/* Power up SAI0 */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_SAI_0, SC_PM_PW_MODE_ON);

	/* Power up AUDIO_PLL_0 */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_AUDIO_PLL_0, SC_PM_PW_MODE_ON);

	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_AUDIO_PLL_0, 4, true, false);

	/* Set UART clock rate */
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_AUDIO_PLL_0, 0, &pll_rate);
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_AUDIO_PLL_0, 1, &pll_rate);

	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_AUDIO_PLL_0, 0, true, false);
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_AUDIO_PLL_0, 1, true, false);

	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_MCLK_OUT_0, SC_PM_PW_MODE_ON);

	//imx_iomux_v3_setup_multiple_pads(sai1_pads, ARRAY_SIZE(sai1_pads));

    SAI_Init(TEST_SAI);

    SAI_GetClassicI2SConfig(&config, TEST_AUDIO_BIT_WIDTH, kSAI_Stereo, kSAI_Channel0Mask);

    config.syncMode = TEST_SAI_TX_SYNC_MODE;
    SAI_TxSetConfig(TEST_SAI, &config);

    /* set bit clock divider */
    SAI_TxSetBitClockRate(TEST_SAI, TEST_AUDIO_MASTER_CLOCK, TEST_AUDIO_SAMPLE_RATE, TEST_AUDIO_BIT_WIDTH,
                          TEST_AUDIO_DATA_CHANNEL);

    return 0;

}

int config_sgtl(struct udevice *dev){

	int ret;

	sgtl_config_t sgtlConfig = {
	    .route        = kSGTL_RoutePlaybackandRecord,
	    .slaveAddress = SGTL5000_I2C_ADDR,
	    .bus          = kSGTL_BusI2S,
	    .format = {.mclk_HZ = TEST_AUDIO_MASTER_CLOCK,
	    		   .sampleRate = TEST_AUDIO_SAMPLE_RATE,
				   .bitWidth = 16},
	    .master_slave = false,
	};

	sgtl_handle_t sgtlHandle;
	sgtlHandle.i2cHandle = dev;
	ret = SGTL_Init(&sgtlHandle, &sgtlConfig);
	if (ret){
		printf("sgtl_initfaield: %i\n",ret);
		return -1;
	}
	uint16_t val;
	for (int i = 0;i< 0x3c;i=i+2){
	    SGTL_ReadReg(&sgtlHandle,i,&val);
	    printf("0x%x: 0x%x\n",i,val);

	}
return ret;
}

int run_audioTest(uint8_t* data, uint32_t len){

	SAI_TxEnable(TEST_SAI,true);
	printf("Sending\n");
	SAI_WriteBlocking(TEST_SAI, 0, TEST_AUDIO_BIT_WIDTH, data, len);
	SAI_TxEnable(TEST_SAI,false);

	return 0;
}


