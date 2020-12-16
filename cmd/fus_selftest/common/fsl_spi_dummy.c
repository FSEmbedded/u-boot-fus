/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dm.h>

static const struct udevice_id spi_ids[] = {
	{ .compatible = "fsl,imx7ulp-spi" },
	{ .compatible = "fsl,imx51-ecspi" },
	{ }
};

U_BOOT_DRIVER(fsl_spi_dummy) = {
	.name	= "fsl_spi_dummy",
	.id	= UCLASS_SPI,
	.of_match = spi_ids,
	.ops	= NULL,
};
