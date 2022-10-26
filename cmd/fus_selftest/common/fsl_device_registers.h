/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FSL_DEVICE_REGISTERS_H__
#define __FSL_DEVICE_REGISTERS_H__

/*
 * Include the cpu specific register header files.
 *
 * The CPU macro should be declared in the project or makefile.
 */
#ifdef CONFIG_IMX8QXP
/* CMSIS-style register definitions */
#include "i2s_regs/MIMX8QX6_cm4.h"
/* CPU specific feature definitions */
#include "i2s_regs/MIMX8QX6_cm4_features.h"
#endif

#ifdef CONFIG_IMX8MM
/* CMSIS-style register definitions */
#include "i2s_regs/MIMX8MM6_cm4.h"
/* CPU specific feature definitions */
#include "i2s_regs/MIMX8MM6_cm4_features.h"
#endif

#ifdef CONFIG_IMX8MP
/* CMSIS-style register definitions */
#include "i2s_regs/MIMX8ML8_cm7.h"
/* CPU specific feature definitions */
#include "i2s_regs/MIMX8ML8_cm7_features.h"
#endif

#endif /* __FSL_DEVICE_REGISTERS_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
