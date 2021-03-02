#include <common.h>
#include <dm.h>
#include <spi.h>
#include "can_spi_test.h"
#include "selftest.h"

#include "common/canfdspi/drv_spi.h"
#include "common/canfdspi/mcp251x.h"

#define CAN_ID 0x01

int test_can(char * szStrBuffer){

	struct udevice *dev;
	struct uclass *uc;

	int err = 1;

	int can_id;

    uint8_t txd[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
	uint8_t rxd[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	CAN_BITTIME_SETUP selectedBitTime = CAN_250K_2M;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	if (uclass_get(UCLASS_SPI, &uc))
		return err;

	uclass_foreach_dev(dev, uc) {

		if (dev_read_bool(dev, "can-spi-mcp251x"))
		{
			printf("CAN_SPI................");

			err = DRV_SPI_Initialize(dev);
			if (err) {
				sprintf(szStrBuffer,"SPI Init failed");
				goto error;
			}
			mdelay(20);

			err = TEST_CANFDSPI_TestPresence();
			if (err) {
				sprintf(szStrBuffer,"Could not find chip on SPI bus");
				goto error;
			}

			err = TEST_CANFDSPI_Init(selectedBitTime);
			if (err) {
				sprintf(szStrBuffer,"CAN Init failed");
				goto error;
			}
			mdelay(20);

			err = TEST_TransmitMessage_8byte(txd, CAN_ID);

			if (err) {
				sprintf(szStrBuffer,"CAN send failed");
				goto error;
			}

			can_id = TEST_ReceiveMessage(rxd, sizeof(rxd));;
			err = can_id;

			if (err < 0) {
				sprintf(szStrBuffer,"CAN revice failed");
				goto error;
			}

			if (can_id == CAN_ID && memcmp(txd, rxd, sizeof(rxd)) == 0){
				err = 0;
			}
			else {
				err = -1;
			}

error:
		DRV_SPI_Exit();
		test_OkOrFail(err,1,szStrBuffer);
		}
	}
	return err;
}
