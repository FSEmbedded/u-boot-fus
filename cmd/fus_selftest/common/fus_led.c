/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dm.h>

static const struct udevice_id led_ids[] = {
	{ .compatible = "fus,test-led" },
	{ }
};

U_BOOT_DRIVER(fus_led) = {
	.name	= "fus_led",
	.id	= UCLASS_GPIO,
	.of_match = led_ids,
	.ops	= NULL,
};
