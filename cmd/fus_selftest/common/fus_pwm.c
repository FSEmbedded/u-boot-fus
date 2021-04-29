/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dm.h>

static const struct udevice_id pwm_ids[] = {
	{ .compatible = "fus,test-pwm" },
	{ }
};

U_BOOT_DRIVER(fus_pwm) = {
	.name	= "fus_pwm",
	.id	= UCLASS_PWM,
	.of_match = pwm_ids,
	.ops	= NULL,
};
