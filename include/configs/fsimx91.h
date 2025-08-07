// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 F&S Elektronik Systeme GmbH
 */

/*
 * TCM layout (SPL)
 * ----------------
 * unused
 *
 * OCRAM layout SPL/ATF
 * --------------------
 * 0x2048_0000: (reserved)           (96KB)     (used by ROM loader)
 * 0x2049_8000: BOARD-CFG            (8KB)      CFG_FUS_BOARDCFG_ADDR
 * 0x2049_A000: SPL                  (176KB)    CONFIG_SPL_TEXT_BASE
 * 0x204C_6000: DRAM-FW              (96KB)     CFG_SPL_DRAM_FW_ADDR
 * 0x204D_E000: INDEX                (8KB)      CFG_FUS_INDEX_ADDR
 * 0x204E_0000: ATF/EARLY_AHAB_BASE  (96KB)     CFG_SPL_ATF_ADDR
 * 0x204F_8000: SPL_STACK            (136KB)    (+ MALLOC_F + GLOBAL_DATA)
 * 0x2051_9DD0:                                 CONFIG_SPL_STACK
 * 0x2051_A000: BSS data             (8KB)      CONFIG_SPL_BSS_START_ADDR
 * 0x2051_C000: DRAM-TIMING          (16KB)     CONFIG_SAVED_DRAM_TIMING_BASE
 * 0x2051_FFFF: (end of OCRAM)
 *
 * The DRAM_FW is loaded to the above address, validated and then copied to
 * &_end of SPL where it is expected by the DRAM initialization code. The
 * DRAM-TIMING is loaded to the above address, validated and used for DRAM
 * initialization. Then ddr_init() copies it to CONFIG_SAVED_DRAM_TIMING_BASE
 * where it is expected by the ATF (required later when switching bus speeds).
 *
 * DRAM Layout UBOOT/TEE
 * ---------------------
 * 0x8000_0000: AHAB_BASE            (64KB)     CFG_SYS_SDRAM_BASE
 * 0x8001_0000: (free)               (1984KB)
 * 0x8020_0000: UBOOT                (3MB)      CONFIG_TEXT_BASE
 * 0x8050_0000: (free)               (347MB)
 * 0x9600_0000: TEE                  (2MB)      CFG_SPL_TEE_ADDR
 * 0x9620_0000: (free)
 */

#include "fsimx93.h"
