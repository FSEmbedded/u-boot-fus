#ifndef _MCP251X
#define	_DRV_SPI_H

#include "drv_canfdspi_api.h"

int TEST_CANFDSPI_TestPresence(void);

int TEST_CANFDSPI_Init(CAN_BITTIME_SETUP selectedBitTime);

int TEST_TransmitMessage_8byte(uint8_t *txd, uint32_t can_id);

int TEST_ReceiveMessage(uint8_t *rxd, int size);

#endif
