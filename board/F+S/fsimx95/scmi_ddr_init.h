/*
 * scmi_ddr_init.h
 *
 * (C) Copyright 2025
 * Kay Müller, F&S Elektronik Systeme GmbH, mueller@fs-net.de
 *
 * SCMI and DDR Init structs
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __SCMI_DDR_INIT_H__
#define __SCMI_DDR_INIT_H__

/* Defines */

#define DDR_PARAM_MAGIC    0x46534452u /* "FSDR" */
#define DDR_PARAM_VERSION  0x00010000u

/* Options-Bits */
#define DDR_OPT_FULL_TRAINING   (1u << 0)
#define DDR_OPT_USE_QB_DATA     (1u << 1)
#define DDR_OPT_SKIP_ZQCAL      (1u << 2)

/* Target-Memory-Type */
#define DDR_OPT_LPDDR5          (1u << 8)
#define DDR_OPT_LPDDR4X         (1u << 9)

/* Types */

struct ddr_init_params {
    uint32_t magic;
    uint32_t version;
    uint32_t fw_addr;         /* SRAM: DDR PHY Firmware-Blob */
    uint32_t fw_size;
    uint32_t timings_addr;    /* SRAM: Timing-Blob */
    uint32_t timings_size;
    uint32_t options;         /* DDR_OPT_* */
    uint32_t reserved;
};

enum ddr_init_status {
    DDR_INIT_OK                =  0,
    DDR_INIT_ERR_PARAM         = -1,
    DDR_INIT_ERR_TRDC          = -2,
    DDR_INIT_ERR_CLOCKS        = -3,
    DDR_INIT_ERR_FW_LOAD       = -4,
    DDR_INIT_ERR_TIMINGS       = -5,
    DDR_INIT_ERR_TRAINING      = -6,
    DDR_INIT_ERR_QB_RESTORE    = -7,
};

/*! SCMI message structure (status only) */
typedef struct
{
    int32_t status;   /*!< Status (see @ref STATUS "SM error codes") */
} scmi_msg_status_t;

/* Request type for MiscControlExtSet() */
typedef struct
{
    /* Identifier for the control */
    uint32_t ctrlId;
    /* Address of the control set */
    uint32_t addr;
    /* Requested size of the set */
    uint32_t len;
    /* Size of the value data */
    uint32_t numVal;
    /* Value data array */
    struct ddr_init_params params;
} msg_rmisc32_t;

#endif /* !__SCMI_DDR_INIT_H__ */
