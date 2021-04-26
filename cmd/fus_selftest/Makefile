# Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_CMD_SELFTEST) += selftest.o
obj-$(CONFIG_CMD_SELFTEST) += check_config.o
obj-$(CONFIG_CMD_SELFTEST) += ethernet_test.o
ifndef CONFIG_DM_SERIAL
obj-$(CONFIG_CMD_SELFTEST) += serial_test.o
else
obj-$(CONFIG_CMD_SELFTEST) += serial_test_dm.o
endif
ifdef CONFIG_ENV_IS_IN_NAND
obj-$(CONFIG_CMD_SELFTEST) += nand_test.o
endif
obj-$(CONFIG_CMD_SELFTEST) += mmc_test.o
obj-$(CONFIG_CMD_SELFTEST) += pmic_test.o
obj-$(CONFIG_CMD_SELFTEST) += sec_test.o
obj-$(CONFIG_CMD_SELFTEST) += audio_test.o
obj-$(CONFIG_CMD_SELFTEST) += dram_test.o
obj-$(CONFIG_CMD_SELFTEST) += usb_test.o
obj-$(CONFIG_CMD_SELFTEST) += gpio_test.o
obj-$(CONFIG_CMD_SELFTEST) += display_test.o
obj-$(CONFIG_CMD_SELFTEST) += rtc_test.o
obj-$(CONFIG_CMD_SELFTEST) += processor_info.o
obj-$(CONFIG_CMD_SELFTEST) += can_spi_test.o
obj-$(CONFIG_CMD_SELFTEST) += common/canfdspi/drv_canfdspi_api.o
obj-$(CONFIG_CMD_SELFTEST) += common/canfdspi/drv_spi.o
obj-$(CONFIG_CMD_SELFTEST) += common/canfdspi/mcp251x.o
obj-$(CONFIG_CMD_SELFTEST) += common/pcf85063tp.o
obj-$(CONFIG_CMD_SELFTEST) += common/pcf85263atl.o
obj-$(CONFIG_CMD_SELFTEST) += common/fus_sai.o
obj-$(CONFIG_CMD_SELFTEST) += common/fsl_sgtl5000.o
obj-$(CONFIG_CMD_SELFTEST) += common/fsl_spi_dummy.o
obj-$(CONFIG_CMD_SELFTEST) += common/fus_sdio.o
obj-$(CONFIG_CMD_SELFTEST) += common/ksz9893r.o



ifdef CONFIG_IMX8MM
obj-$(CONFIG_CMD_SELFTEST) += imx8mm/audio_imx8mm.o
obj-$(CONFIG_CMD_SELFTEST) += imx8mm/serial_imx8mm.o
endif
ifdef CONFIG_IMX8QXP
obj-$(CONFIG_CMD_SELFTEST) += imx8x/ethernet_imx8x.o
obj-$(CONFIG_CMD_SELFTEST) += imx8x/serial_imx8x.o
obj-$(CONFIG_CMD_SELFTEST) += imx8x/audio_imx8x.o
endif


endif
