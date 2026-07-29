/*
 * (C) Copyright 2020
 * Hartmut Keller <keller@fs-net.de>
 *
 * Linker script to generate DDR RAM timing blobs for i.MX8 CPUs.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

MEMORY { .npu_sram : ORIGIN = ###DRAM_TIMING_ADDR###, LENGTH = 0x20000 }
OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
SECTIONS
{
	. = 0x00000000;
	.data : {
		_start = .;
		*(.data.dram_timing)		/* Have dram_timing right at beginning */
		*(SORT_BY_NAME(.data.ddr_*))	/* Sort by name to have fix order */
		. = ALIGN(8);
		_end = .;
	} >.npu_sram
}
