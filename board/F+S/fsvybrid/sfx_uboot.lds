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
 * 0x8000_0000: (unused) (4KiB)
 * 0x8000_1000: NBoot-Args coming from NBoot (4KiB, untouched)
 * 0x8000_2000: (unused) (56KiB)
 * 0x8010_0000: Compressed U-Boot image with self-extractor (384KiB)
 * 0x8016_0000: (unused, space for bigger U-Boot)
 * 0x8018_0000: Stack (512KiB)
 * 0x8020_0000: Uncompressed U-Boot image (3MiB)
 * 0x8050_0000: Free memory area for decompressor dictionary (unlimited)
 */

OUTPUT_ARCH(arm)
OUTPUT_FORMAT("elf32-littlearm")
ENTRY(_start)

MEMORY {
	uboot_sfx (rwx): ORIGIN = 0x80100000, LENGTH = 0x60000
	stack (rw):     ORIGIN = 0x80180000, LENGTH = 0x80000
	ubootdata (rw): ORIGIN = 0x80200000, LENGTH = 0x300000
	free_mem (rw):  ORIGIN = 0x80500000, LENGTH = 0x4100000
	pagetable (rw):	ORIGIN = 0x3f054000, LENGTH = 0x4000
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
	.pagetable : {
		*(.pagetable)
	} > pagetable
}
