// SPDX-License-Identifier: GPL-2.0
/*
 * Generic code for all self extractors.
 */

#include <linux/compiler.h>	/* for inline */
#include <linux/types.h>
#include <linux/linkage.h>

/* Uncomment one of the following DEBUG_UART entries to activate debugging */
//#define DEBUG_UART 0x21f0000		/* efusA9dl */
//#define DEBUG_UART 0x2020000		/* efusA9x, efusA7ul */

extern u8 uboot_start_comp;
extern u8 uboot_end_comp;

#ifdef DEBUG_UART
#define debug(x) puts(x)

static inline void _ll_putc(char c)
{
	void *uart = (void *)DEBUG_UART;

	*(volatile u32 *)(uart + 0x40) = (u32)c;
	while (!(*(volatile u32 *)(uart + 0x98) & (1 << 3)))
		;
}

void putc(char c)
{
	if (c == '\n')
		_ll_putc('\r');
	_ll_putc(c);
}

void puts(const char *s)
{
	while (*s)
		putc(*s++);
}

static inline void hex4(unsigned int val)
{
	val &= 0xf;
	if (val >= 10)
		_ll_putc(val - 10 + 'a');
	else
		_ll_putc(val + '0');
}

void hex8(unsigned int val)
{
	hex4(val >> 4);
	hex4(val);
}

void hex16(unsigned int val)
{
	hex8(val >> 8);
	hex8(val);
}

void hex32(unsigned int val)
{
	hex16(val >> 16);
	hex16(val);
}

void hexdump(const void *start, unsigned int size)
{
	unsigned int i = 0;
	const char *p = start;

	if (!size)
		return;

	do {
		if ((i & 15) == 0) {
			hex32((unsigned int)p);
			putc(':');
		}
		putc(' ');
		hex8(p[i++]);
		if ((i < size) && (((i & 15) == 0)))
			putc('\n');
	} while (i < size);
	putc('\n');
}

#else

void debug(const char *s) {}
void putc(char c) {}
void puts(const char *s) {}
void hex8(unsigned int val) {}
void hex16(unsigned int val) {}
void hex32(unsigned int val) {}
void hexdump(const void *start, unsigned int size) {}

#endif /* DEBUG_UART */

void error(char *x)
{
	puts("\n\n## ");
	puts(x);
	puts(" -- System halted\n\n");

	while (1);	/* Halt */
}

asmlinkage void __div0(void)
{
	error("Attempting division by 0!");
}

extern int decompress(unsigned char *inbuf, long in_len,
		      unsigned char *outbuf, long out_len, long *out_size);

/* Set to a bigger value (e.g. 20) for benchmarking */
#define REPEAT_COUNT 1

/* Called from sfx_head.S */
void sfx_uboot(unsigned long uboot_out, long expected_size)
{
	int err;
	long uboot_size_comp = &uboot_end_comp - &uboot_start_comp;
	long real_size;
	int i;

	for (i = REPEAT_COUNT; i; i--)
	{
#if defined(DEBUG_UART) && (REPEAT_COUNT > 1)
		hex8(i);
		putc(':');
#endif
		debug("Decompressing U-Boot...");
		err = decompress(&uboot_start_comp, uboot_size_comp,
				 (unsigned char *)uboot_out, expected_size,
				 &real_size);
		if (err || (real_size != expected_size)) {
			debug("Failed\n");
			if (err)
				error("decompressor returned an error");
			else
				error("size mismatch");
		}
		debug("Done\n");
	}
}
