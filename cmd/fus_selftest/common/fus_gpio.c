/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dm.h>

static const struct udevice_id gpio_ids[] = {
	{ .compatible = "fus,test-gpio" },
	{ }
};

U_BOOT_DRIVER(fus_gpio) = {
	.name	= "fus_gpio",
	.id	= UCLASS_GPIO,
	.of_match = gpio_ids,
	.ops	= NULL,
};
