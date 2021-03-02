#include <common.h>
#include <spi.h>
#include <dm.h>
#include "drv_spi.h"

static struct spi_slave *slave;

void Nop(void){
	udelay(1);
}


int8_t DRV_SPI_Initialize(struct udevice *dev)
{
		char name[30], *str;
		int bus, cs;
		unsigned int max_hz;
		int mode = 0;
		int ret = 0;

		bus= dev->req_seq;

		cs = dev_read_u32_default(dev, "cs", -1);
		max_hz = dev_read_u32_default(dev, "spi-max-frequency", 0);

		if (dev_read_bool(dev, "spi-cpol"))
			mode |= SPI_CPOL;
		if (dev_read_bool(dev, "spi-cpha"))
			mode |= SPI_CPHA;
		if (dev_read_bool(dev, "spi-cs-high"))
			mode |= SPI_CS_HIGH;
		if (dev_read_bool(dev, "spi-3wire"))
			mode |= SPI_3WIRE;
		if (dev_read_bool(dev, "spi-half-duplex"))
			mode |= SPI_PREAMBLE;

		snprintf(name, sizeof(name), "generic_%d:%d", bus, cs);
		str = strdup(name);

		ret = spi_get_bus_and_cs(bus, cs, max_hz, mode, "spi_generic_drv",
				 str, &dev, &slave);
		if (ret)
			printf("Faield to get bus\n");

		ret = spi_claim_bus(slave);
		if (ret){
			printf("Faield to claim bus\n");
		}

		return ret;
}

int8_t DRV_SPI_TransferData(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{
	u8 ret;
    /* spi_xfer needs size in bits */
	spiTransferSize = spiTransferSize*8;

	ret = spi_xfer(slave, spiTransferSize, SpiTxData, SpiRxData, SPI_XFER_BEGIN | SPI_XFER_END);

	return ret;
}

int8_t DRV_SPI_Exit()
{
	spi_release_bus(slave);
	return 0;
}
