/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dm.h>

static const struct udevice_id relay_ids[] = {
	{ .compatible = "fus,test-relay" },
	{ }
};

U_BOOT_DRIVER(fus_relay) = {
	.name	= "fus_relay",
	.id	= UCLASS_GPIO,
	.of_match = relay_ids,
	.ops	= NULL,
};
