/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Self-Extractor (SFX) for decompressing a compressed U-Boot image.
 *
 * Copyright (C) 2026 Hartmut Keller (keller@fs-net.de)
 *
 * On F&S boards with ARMv7 CPUs, U-Boot is expected to be smaller than 512KiB.
 * However when switching to Driver Model and Device Trees, this space will
 * only be sufficient if U-Boot is compressed.
 *
 * List of compression ratios for different compression algorithms
 *
 *   Method       20x dtb     U-Boot    U-Boot + 20x dtb
 *   ---------------------------------------------------
 *   (none)       938.728     492.768   1.431.496
 *   gzip         220.764     289.036     509.875
 *   gzip -9      210.545     288.852     499.405
 *   bzip2         64.246     299.142     384.040
 *   bzip2 -9      64.246     299.142     384.040
 *   lzop         361.985     401.883     761.648
 *   lzop -9      170.471     322.805     494.548
 *   lz4          281.113     382.474     663.563
 *   lz4 -9       143.369     346.769     490.079
 *   zstd          64.457     307.502     372.888
 *   zstd -9       55.915     301.762     358.167
 *   zstd -19      48.168     276.530     323.763
 *   xz            39.548     247.596     286.928
 *   xz -9         39.548     247.596     286.928
 *   lzma -0       51.144     261.802     312.876
 *   lzma          39.505     247.512     286.928
 *   lzma -9       39.505     247.512     286.846
 *
 * As there is already some LZMA support in U-Boot's Makefile and the
 * compression is the best, we use LZMA compression.
 *
 * This self-extractor is prepended to the compressed U-Boot image and
 * decompresses it when called from NBoot. This means the compressed U-Boot
 * together with the self-extractor must look exactly the same for N-Boot as
 * a regular U-Boot. This requires a magic number at offset 0x3c and an
 * absolute address at offset 0x20.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>

#define SFX_USE_CACHE
//#define SFX_DCACHE_WRITETHROUGH

 	.arch	armv7-a

	.section ".start", "ax"

	/* Start in ARM mode, this is what NBoot expects to call */
	.arm

	.globl _start
_start:
	b	start			@ 0x00

	.word	0			@ 0x04
	.word	0			@ 0x08
	.word	0			@ 0x0c
	.word	0			@ 0x10
	.word	0			@ 0x14
	.word	0			@ 0x18
	.word	0			@ 0x1c
/* NBoot needs an absolute value at offset 0x20 to get U-Boot's load address */	
	.word	start			@ 0x20: absolute value
	.word	0			@ 0x24
	.word	0			@ 0x28
	.word	0			@ 0x2c
	.word	0			@ 0x30
	.word	0			@ 0x34
	.word	0			@ 0x38
/* NBoot needs the following magic number at offset 0x3c to identify U-Boot */
        .word	0x12345678		@ 0x3c: U-BOOT magic
	.word	0			@ 0x40
	.word	0			@ 0x44
	.word	0			@ 0x48
	.word	0			@ 0x4c
/* Old F&S NBoot versions expect the entry point at offset 0x5C */
fs_pseudo_start:
	nop				@ 0x50
	nop				@ 0x54
	nop				@ 0x58
	nop				@ 0x5c

	/* Switch to THUMB mode as soon as possible */
start:
	.type	start,#function
	adr	r0, start_thumb + 1
	bx	r0
	.word	0
	.word	0

	.thumb

start_thumb:
	/*
	 * N-Boot leaves I-cache and MMU active, but D-cache is off; page
	 * table is in OCRAM at 0x09180000, SP also points to OCRAM.
	 */
	adr	r0, SFX_SETUP
	ldr	sp, [r0]		@ Stack pointer
	ldr	r2, [r0, #4]		@ bss start
	ldr	r3, [r0, #8]		@ bss end

	mov	r0, #0			@ clear bss (size may be 0)
	b	2f
1:	str	r0, [r2], #4
	str	r0, [r2], #4
	str	r0, [r2], #4
	str	r0, [r2], #4
2:	cmp	r2, r3
	blo	1b

#ifdef SFX_USE_CACHE
	adr	r1, _start
	ldr	r0, =0x918000		@ r0: Page table in OCRAM
	lsr	r1, #28			@ r1: Start of DRAM
	lsl	r1, #28			@ i.MX6: 0x10000000, SX/UL: 0x80000000
	bl	cache_on
#endif

	adr	r0, UBOOT_INFO
	ldr	r4, [r0]		@ start of inflated image
	ldr	r5, [r0, #4]		@ end of inflated image

	mov	r0, r4
	mov	r1, r5
	sub	r1, r1, r0
	bl	sfx_uboot

#ifdef SFX_USE_CACHE
	mov	r0, r4
	mov	r1, r5
	bl	cache_clean_flush
	bl	cache_off
#endif

	bx	r4			@ Jump to real U-Boot (in ARM mode)

	.align	2
SFX_SETUP:
	.type	SFX_SETUP,#object
	.word	stack_end		@ sp (end of stack, grows down)
	.word	__bss_start		@ r2
	.word	__bss_end		@ r3

UBOOT_INFO:
	.type	UBOOT_INFO,#object
	.word	uboot_start		@ U-Boot final address
	.word	uboot_end		@ U-Boot decompressed size

#ifdef SFX_USE_CACHE

/*
 * Turn on the cache on ARMv7.  We need to setup some page tables so that we
 * can have both the I and D caches on.
 *
 * On entry,
 *  r0 = Address for page table
 *  r1 = Start of DRAM
 * On exit,
 *  r0, r1, r2, r3, r9, r10, r11 corrupted
 * This routine must preserve:
 *  r4, r7, r8
 */
#ifdef SFX_DCACHE_WRITETHROUGH
#define CB_BITS 0x08
#else
#define CB_BITS 0x0c
#endif
	.align	5
	.macro	enable_cp15_barriers, reg
	mrc	p15, 0, \reg, c1, c0, 0	@ read SCTLR
	tst	\reg, #(1 << 5)		@ CP15BEN bit set?
	bne	.L_\@
	orr	\reg, \reg, #(1 << 5)	@ CP15 barrier instructions
	mcr	p15, 0, \reg, c1, c0, 0	@ write SCTLR
 	isb
 .L_\@:
	.endm

cache_on:
	enable_cp15_barriers	r11
	mov	r3, r0
	mrc	p15, 0, r11, c0, c1, 4	@ read ID_MMFR0
	tst	r11, #0xf		@ we can only handle VMSA on ARMv7-A,
	beq	no_vmsa			@ not PMSA on ARMv7-R

	/*
	 * Initialise the page tables, turning on the cacheable and bufferable
	 * bits for the RAM area only. Silently assume 256MiB RAM.
	 */
__setup_mmu:
	mov	r9, r1			@ r9: start of RAM
	add	r10, r9, #0x10000000	@ r10: end of RAM (start + 256MiB)
	mov	r1, #0x12		@ start addr. 0, XN|U + section mapping
	orr	r1, r1, #3 << 10	@ AP=11 (full access, R/W)
	add	r2, r0, #16384		@ Page table size is 16KiB
1:	cmp	r1, r9			@ if virt > start of RAM
	cmphs	r10, r1			@   && end of RAM > virt
	bic	r1, r1, #0x1c		@ clear XN|U + C + B
	orrlo	r1, r1, #0x10		@ Set XN|U for non-RAM sections
	orrhs	r1, r1, #CB_BITS	@ set !XN and CB for RAM sections
	str	r1, [r0], #4		@ 1:1 mapping, next page table entry
	add	r1, r1, #1048576	@ Next memory section at offset 1MiB
	teq	r0, r2
	bne	1b

no_vmsa:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4	@ drain write buffer
	tst	r11, #0xf		@ VMSA?
	mcrne	p15, 0, r0, c8, c7, 0	@ flush I,D TLBs

	mrc	p15, 0, r0, c1, c0, 0	@ read control reg (SCTLR)
	bic	r0, r0, #1 << 28	@ clear SCTLR.TRE
	orr	r0, r0, #0x5000		@ I-cache enable, RR cache replacement
	orr	r0, r0, #0x003c		@ write buffer, D-cache enable
	bic	r0, r0, #2		@ A (no unaligned access fault)
	orr	r0, r0, #1 << 22	@ U (v6 unaligned access model)
					@ (needed for ARM1176)
	mrcne   p15, 0, r6, c2, c0, 2   @ read ttb control reg (TTBCR)
	orrne	r0, r0, #1		@ enable MMU

	/* Keep domain access control (0x55555555) from NBoot */
//###	mrc	p15, 0, r1, c3, c0, 0	@ load domain access control (DACR)
//###	movne	r1, #0xfffffffd		@ domain 0 = client
//###	movne	r1, #0x55555555		@ all domains = client
	bic     r6, r6, #1 << 31        @ 32-bit translation system
	bic     r6, r6, #(7 << 0) | (1 << 4)	@ use only ttbr0
	mcrne	p15, 0, r3, c2, c0, 0	@ store page table pointer (TTBR0)
//###	mcrne	p15, 0, r1, c3, c0, 0	@ store domain access control (DACR)
	mcrne   p15, 0, r6, c2, c0, 2   @ store ttb control (TTBCR)

	mcr	p15, 0, r0, c7, c5, 4	@ ISB
	mcr	p15, 0, r0, c1, c0, 0	@ store control register (SCTLR)
	mrc	p15, 0, r0, c1, c0, 0	@ and read it back
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 4	@ ISB
	mov	pc, lr

/*
 * Turn off the Cache and MMU on ARMv7
 *
 * On exit,
 *  r0 is corrupted
 */
	.align	5
cache_off:
	mrc	p15, 0, r0, c1, c0	@ SCTRL
	bic	r0, r0, #0x0005		@ turn MMU and cache off
	mcr	p15, 0, r0, c1, c0	@ SCTRL
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate whole TLB
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate BTC (BPIALL)
	mcr	p15, 0, r0, c7, c10, 4	@ DSB
	mcr	p15, 0, r0, c7, c5, 4	@ ISB
	mov	pc, lr

/*
 * Clean and flush the cache to maintain consistency on ARMv7
 *
 * On entry,
 *  r0 = start address
 *  r1 = end address (exclusive)
 * On exit,
 *  r0, r1, r2, r10, r11 corrupted
 */
	.align	5
cache_clean_flush:
	mov	r11, r1
	enable_cp15_barriers	r10
	mrc	p15, 0, r10, c0, c1, 5	@ read ID_MMFR1
	tst	r10, #0xf << 16		@ hierarchical cache (ARMv7)
	mov	r10, #0
	bne	non_hierarchical
hierarchical:
	/* Get cache line size */
	mrc	p15, 0, r2, c0, c0, 1	@ read cache type register (CTR)
	lsr	r2, r2, #16
	and	r2, r2, #0xf		@ cache line size encoding
	mov	r1, #4			@ bytes per word
	mov	r1, r1, lsl r2		@ r1 := actual cache line size
	sub	r2, r1, #1		@ r2 := line size mask
	bic	r0, r0, r2		@ round down start to line size
	sub	r11, r11, #1		@ end address is exclusive
	bic	r11, r11, r2		@ round down end to line size
0:	cmp	r0, r11			@ finished?
	bgt	iflush
	mcr	p15, 0, r0, c7, c14, 1	@ Dcache clean/invalidate by VA
	add	r0, r0, r1
	b	0b
non_hierarchical:
	mcr	p15, 0, r10, c7, c14, 0	@ clean+invalidate D
iflush:
	mcr	p15, 0, r10, c7, c10, 4	@ DSB
	mcr	p15, 0, r10, c7, c5, 0	@ invalidate I+BTB (ICIALLU)
	mcr	p15, 0, r10, c7, c10, 4	@ DSB
	mcr	p15, 0, r10, c7, c5, 4	@ ISB
	mov	pc, lr
#endif /* SFX_USE_CACHE */

#if 0 // Debugging is done in C-Code now; keep code just for reference

#define DEBUG_UART 0x21f0000

/*
 * hexdump: output some memory as hex bytes
 * Can be called from ARM or THUMB mode.
 * r0: Pointer to data to show
 * r1: Number of bytes to show (0 will output nothing)
 * corrupted: r0, r1, r2, r3, r12
 */
	.globl	hexdump
hexdump:
	movs	r3, r1
	beq	5f
	mov	r2, r0
	push	{lr}
	mov	r12, r0
	add	r3, r3, r2
	b	1f
0:	mov	r0, r2
	eor	r0, r12
	tst	r0, #0xf
	bne	2f
	bl	newline
1:	mov	r0, r2
	bl	hex32
	mov	r0, #':'
	bl	_ll_putc
2:	mov	r0, #' '
	bl	_ll_putc
	ldrb	r0, [r2], #1
	bl	hex8
	cmp	r2, r3
	bne	0b
3:	pop	{lr}
	/* fall through to newline */

/*
 * putc: Put character to debug UART and wait until sent.
 * newline: output newline (CR/LF)
 * Can be called from ARM or THUMB mode.
 * r0: Character to send
 * corrupted: r0, r1
 */
	.globl	newline
	.globl	putc
newline:
	mov	r0, #'\n'
putc:	cmp	r0, #'\n'
	bne	_ll_putc
	push	{r0, lr}
	mov	r0, #'\r'
	bl	_ll_putc
	pop	{r0, lr}
_ll_putc:
	ldr	r1, =DEBUG_UART
	str	r0, [r1, #0x40]
1:	ldr	r0, [r1, #0x98]
	tst	r0, #1<<3
	beq	1b
5:	bx	lr

/*
 * puts: output string
 * r0: Pointer to string
 * corrupted: r0, r1, r2
 */
	.globl	puts
puts:	push	{lr}
	mov	r2, r0
	b	2f
1:	bl	putc
2:	ldrb	r0, [r2], #1
	cmp	r0, #0
	bne	1b
	pop	{pc}

/*
 * hex32/16/8/4: output 8/4/2/1 hex digit(s).
 * Can be called from ARM or THUMB mode.
 * r0: Number to show in hex
 * corrupted: r0, r1
 */
	.globl	hex32
	.globl	hex16
	.globl	hex8
	.globl	hex4
hex32:	push	{r0, lr}
	lsr	r0, r0, #16
	bl	hex16
	pop	{r0, lr}
hex16:	push	{r0, lr}
	lsr	r0, r0, #8
	bl	hex8
	pop	{r0, lr}
hex8:	push	{r0, lr}
	lsr	r0, r0, #4
	bl	hex4
	pop	{r0, lr}
hex4:	and	r0, r0, #0xf
	cmp	r0, #10
	addhs	r0, r0, #'a' - 10
	addlt	r0, r0, #'0'
	b	_ll_putc
#endif //0

	.ltorg

/* Define Stack */
	.align
	.section ".stack", "aw", %nobits
stack_start:
	.space	0x10000
stack_end:

/* Define free_mem area for decompression dictionary (LZMA: at most 65MB) */
	.section ".free_mem", "aw", %nobits
	.globl	free_mem_start
free_mem_start:
	.space	0x4100000
	.globl	free_mem_end
free_mem_end:

/* Include U-Boot compressed data as section .ubootcomp  */
	.section .ubootcomp, "a", %progbits
	.globl	uboot_start_comp
uboot_start_comp:
#ifdef CONFIG_FS_SFX_UBOOT_ZSTD
	.incbin	"u-boot.bin.zst"
#endif
#ifdef CONFIG_FS_SFX_UBOOT_LZMA
	.incbin	"u-boot.bin.lzma"
#endif
	.globl	uboot_end_comp
uboot_end_comp:

/* Include U-Boot data to determine size, but do not include in output  */
	.section ".ubootdata", "aw", %nobits
uboot_start:
	.incbin	"u-boot.bin"
uboot_end:
