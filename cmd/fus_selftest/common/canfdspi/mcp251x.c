#include <common.h>
#include <linux/delay.h>

#include "mcp251x.h"

#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"
#include "drv_spi.h"

#define CAN_ID 0x1

// Transmit Channels
#define APP_TX_FIFO CAN_FIFO_CH2

// Receive Channels
#define APP_RX_FIFO CAN_FIFO_CH1
#define DRV_CANFDSPI_INDEX_0 0

static CAN_CONFIG config;

static REG_CiFLTOBJ fObj;
static REG_CiMASK mObj;

// Transmit objects
static CAN_TX_FIFO_CONFIG txConfig;
static CAN_TX_FIFO_EVENT txFlags;


// Receive objects
static CAN_RX_FIFO_CONFIG rxConfig;
static CAN_RX_FIFO_EVENT rxFlags;
static CAN_RX_MSGOBJ rxObj;


static uint8_t tec;
static uint8_t rec;
static CAN_ERROR_STATE errorFlags;

static bool ramInitialized = false;

int TEST_CANFDSPI_TestPresence(void){
	uint8_t reg;

	/* Bit 7 should always read 1 */
	DRV_CANFDSPI_ReadByte(DRV_CANFDSPI_INDEX_0, cREGADDR_CiTXQCON, &reg);

	if (reg & (1 << 7))
			return 0;
	else
			return -1;
}

int TEST_CANFDSPI_Init(CAN_BITTIME_SETUP selectedBitTime)
{
	uint32_t attempts = 10000;
	uint8_t mode;

    // Reset device
    DRV_CANFDSPI_Reset(DRV_CANFDSPI_INDEX_0);

    // Enable ECC and initialize RAM

    DRV_CANFDSPI_EccEnable(DRV_CANFDSPI_INDEX_0);

    if (!ramInitialized) {
        DRV_CANFDSPI_RamInit(DRV_CANFDSPI_INDEX_0, 0xff);
        ramInitialized = true;
    }

    // Configure device
    DRV_CANFDSPI_ConfigureObjectReset(&config);
    config.IsoCrcEnable = 1;
    config.StoreInTEF = 0;
    DRV_CANFDSPI_Configure(DRV_CANFDSPI_INDEX_0, &config);

    // Setup TX FIFO
    DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
    txConfig.FifoSize = 7;
    txConfig.PayLoadSize = CAN_PLSIZE_64;
    txConfig.TxPriority = 1;

    DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, &txConfig);

    // Setup RX FIFO
    DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
    rxConfig.FifoSize = 15;
    rxConfig.PayLoadSize = CAN_PLSIZE_64;

    DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxConfig);

    // Setup RX Filter
    fObj.word = 0;
    fObj.bF.SID = 0xda;
    fObj.bF.EXIDE = 0;
    fObj.bF.EID = 0x00;

    DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &fObj.bF);

    // Setup RX Mask
    mObj.word = 0;
    mObj.bF.MSID = 0x0;
    mObj.bF.MIDE = 1; // Only allow standard IDs
    mObj.bF.MEID = 0x0;
    DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &mObj.bF);

    // Link FIFO and Filter
    DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, APP_RX_FIFO, true);

    // Setup Bit Time
    DRV_CANFDSPI_BitTimeConfigure(DRV_CANFDSPI_INDEX_0, selectedBitTime, CAN_SSP_MODE_AUTO, CAN_SYSCLK_20M);

    // Setup Transmit and Receive Interrupts
    DRV_CANFDSPI_GpioModeConfigure(DRV_CANFDSPI_INDEX_0, GPIO_MODE_INT, GPIO_MODE_INT);
    DRV_CANFDSPI_TransmitChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
    DRV_CANFDSPI_ReceiveChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
    DRV_CANFDSPI_ModuleEventEnable(DRV_CANFDSPI_INDEX_0, CAN_TX_EVENT | CAN_RX_EVENT);

    // Select Normal Mode
    DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_NORMAL_MODE);
	do {
		mode = DRV_CANFDSPI_OperationModeGet(0);
        if (attempts == 0) {
            return -1;
        }
		attempts--;
	}while(mode !=CAN_NORMAL_MODE);
	return 0;
}

int TEST_TransmitMessage_8byte(uint8_t *txd, uint32_t can_id)
{
	uint8_t attempts = 5;

	CAN_TX_MSGOBJ txObj;
	txObj.word[0] = 0;
	txObj.word[1] = 0;

    txObj.bF.id.SID = can_id;
	txObj.bF.id.EID = 0;

	txObj.bF.ctrl.FDF = 0;
	txObj.bF.ctrl.BRS = 0;
	txObj.bF.ctrl.IDE = 0;
	txObj.bF.ctrl.RTR = 0;
    txObj.bF.ctrl.DLC = CAN_DLC_8;

    // Check if FIFO is not full
    do {

        DRV_CANFDSPI_TransmitChannelEventGet(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, &txFlags);

        if (attempts == 0) {
			udelay(1);
            DRV_CANFDSPI_ErrorCountStateGet(DRV_CANFDSPI_INDEX_0, &tec, &rec, &errorFlags);
            return -1;
        }
        attempts--;
    }
    while (!(txFlags & CAN_TX_FIFO_NOT_FULL_EVENT));

    // Load message and transmit
    uint8_t n = DRV_CANFDSPI_DlcToDataBytes(txObj.bF.ctrl.DLC);

    DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, &txObj, txd, n, true);

	return 0;
}
int TEST_ReceiveMessage(uint8_t *rxd, int size)
{
	uint32_t attempts = 100000;

	 do {
    	// Check if FIFO is not empty
    	DRV_CANFDSPI_ReceiveChannelEventGet(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxFlags);
        if (attempts == 0) {
			udelay(1);
            DRV_CANFDSPI_ErrorCountStateGet(DRV_CANFDSPI_INDEX_0, &tec, &rec, &errorFlags);
            return -1;
        }
        attempts--;
	} while (!(rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT));

	DRV_CANFDSPI_ReceiveMessageGet(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxObj, rxd, size);

	return rxObj.bF.id.SID;
}
