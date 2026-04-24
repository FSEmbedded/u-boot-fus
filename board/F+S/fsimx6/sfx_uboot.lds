/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Hartmut Keller <keller@fs-net.de>
 *
 * Linker script to generate LZMA compressed U-Boot image with Self Extractor.
 * NBoot expects U-Boot at offset 0x0010_0000 from start of DRAM. Final U-Boot
 * is expected to start at offset 0x0020_0000 (TEXT_BASE).
 *
 * Memory Map
 * ----------
 * 0x1000_0000: (unused) (4KiB)
 * 0x1000_1000: NBoot-Args coming from NBoot (4KiB, untouched)
 * 0x1000_2000: (unused) (56KiB)
 * 0x1010_0000: Compressed U-Boot image with self-extractor (512KiB)
 * 0x1018_0000: Stack (512KiB)
 * 0x1020_0000: Uncompressed U-Boot image (3MiB)
 * 0x1050_0000: Free memory area for decompressor dictionary (unlimited)
 */

OUTPUT_ARCH(arm)
OUTPUT_FORMAT("elf32-littlearm")
ENTRY(_start)

MEMORY {
        pagetable (rw): ORIGIN = 0x1000c000, LENGTH = 0x4000
	uboot_sfx (rwx): ORIGIN = 0x10100000, LENGTH = 0x80000
	stack (rw):     ORIGIN = 0x10180000, LENGTH = 0x80000
	ubootdata (rw): ORIGIN = 0x10200000, LENGTH = 0x300000
	free_mem (rw):  ORIGIN = 0x10500000, LENGTH = 0x4100000
}
SECTIONS
{
	. = 0x00000000;
	.uboot_sfx : {
		_start = .;
	      	*(.start)
	      	*(.text)
	      	*(.text.*)
		. = ALIGN(8);
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(8);
		*(.data)
		*(.data.*)
		. = ALIGN(16);
		__bss_start = .;
		*(.bss)
		*(.bss.*)
		. = ALIGN(16);
		__bss_end = .;
	} > uboot_sfx
	.stack : {
		*(.stack)
	} > stack
	.ubootdata : {
		*(.ubootdata)
	} > ubootdata
	.freemem : {
		*(.free_mem)
	} > free_mem
}
