// SPDX-License-Identifier: GPL-2.0

/*
 * Important notes about in-place decompression
 *
 * At least on x86, the kernel is decompressed in place: the compressed data
 * is placed to the end of the output buffer, and the decompressor overwrites
 * most of the compressed data. There must be enough safety margin to
 * guarantee that the write position is always behind the read position.
 *
 * The safety margin for ZSTD with a 128 KB block size is calculated below.
 * Note that the margin with ZSTD is bigger than with GZIP or XZ!
 *
 * The worst case for in-place decompression is that the beginning of
 * the file is compressed extremely well, and the rest of the file is
 * uncompressible. Thus, we must look for worst-case expansion when the
 * compressor is encoding uncompressible data.
 *
 * The structure of the .zst file in case of a compressed kernel is as follows.
 * Maximum sizes (as bytes) of the fields are in parenthesis.
 *
 *    Frame Header: (18)
 *    Blocks: (N)
 *    Checksum: (4)
 *
 * The frame header and checksum overhead is at most 22 bytes.
 *
 * ZSTD stores the data in blocks. Each block has a header whose size is
 * a 3 bytes. After the block header, there is up to 128 KB of payload.
 * The maximum uncompressed size of the payload is 128 KB. The minimum
 * uncompressed size of the payload is never less than the payload size
 * (excluding the block header).
 *
 * The assumption, that the uncompressed size of the payload is never
 * smaller than the payload itself, is valid only when talking about
 * the payload as a whole. It is possible that the payload has parts where
 * the decompressor consumes more input than it produces output. Calculating
 * the worst case for this would be tricky. Instead of trying to do that,
 * let's simply make sure that the decompressor never overwrites any bytes
 * of the payload which it is currently reading.
 *
 * Now we have enough information to calculate the safety margin. We need
 *   - 22 bytes for the .zst file format headers;
 *   - 3 bytes per every 128 KiB of uncompressed size (one block header per
 *     block); and
 *   - 128 KiB (biggest possible zstd block size) to make sure that the
 *     decompressor never overwrites anything from the block it is currently
 *     reading.
 *
 * We get the following formula:
 *
 *    safety_margin = 22 + uncompressed_size * 3 / 131072 + 131072
 *                 <= 22 + (uncompressed_size >> 15) + 131072
 */

/*
 * Preboot environments #include "path/to/decompress_unzstd.c".
 * All of the source files we depend on must be #included.
 * zstd's only source dependency is xxhash, which has no source
 * dependencies.
 *
 * Define __DISABLE_EXPORTS in preboot environments to prevent symbols
 * from xxhash and zstd from being exported by the EXPORT_SYMBOL macro.
 */
#define DEBUG
#include <linux/compiler.h>		/* for inline */
#include <linux/types.h>
#include <linux/linkage.h>
#include <linux/zstd_errors.h>
#include <linux/zstd_lib.h>
#include <string.h>			/* memcpy() */


/* ### Noch testen: kann man das aktivieren?
ccflags-y += -DHUF_FORCE_DECOMPRESS_X1
ccflags-y += -DZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT
ccflags-y += -DZSTD_NO_INLINE
ccflags-y += -DZSTD_STRIP_ERROR_STRINGS
ccflags-y += -DDYNAMIC_BMI2=0
*/

extern u8 free_mem_start;
extern u8 free_mem_end;

extern void puts(const char *s);

#ifndef CONFIG_USE_ARCH_MEMMOVE
/*
 * memmove() is part of string.o that pulls in malloc() for other reasons;
 * have a copy without dependency to malloc() here
 */
void *memmove(void * dest,const void *src,size_t count)
{
	char *d;
	char *s;

	if (dest <= src || (src + count) <= dest) {
		/*
		 * Use the fast memcpy implementation (ARCH optimized or
		 * lib/string.c) when it is possible:
		 * - when dest is before src (assuming that memcpy is doing
		 *   forward-copying)
		 * - when destination don't overlap the source buffer (src +
		 *   count <= dest)
		 *
		 * WARNING: the first optimisation cause an issue, when
		 * __HAVE_ARCH_MEMCPY is defined, __HAVE_ARCH_MEMMOVE is not
		 * defined and if the memcpy ARCH-specific implementation is
		 * not doing a forward-copying.
		 *
		 * No issue today because memcpy is doing a forward-copying in
		 * lib/string.c and for ARM32 architecture; no other arches
		 * use __HAVE_ARCH_MEMCPY without __HAVE_ARCH_MEMMOVE.
		 */
		return memcpy(dest, src, count);
	}

	d = (char *)dest;
	s = (char *)src;

	/* while all data is aligned (common case), copy a word at a
	   time */
	if ((count >= sizeof(ulong))
	    && ((((ulong)d | (ulong)s) & (sizeof(ulong) - 1)) == 0)) {
		while (count & (sizeof(ulong) - 1)) {
			--count;
			d[count] = s[count];
		}
		while (count) {
			count -= sizeof(ulong);
			*(ulong *)&d[count] = *(ulong *)&s[count];
		}
		return (void *)d;
	}

	/* copy the rest one byte at a time */
	while (count) {
		--count;
		d[count] = s[count];
	}
	return (void *)d;
}
#endif /* !CONFIG_USE_ARCH_MEMMOVE */

static int handle_zstd_error(size_t ret)
{
	const ZSTD_ErrorCode err = ZSTD_getErrorCode(ret);

	if (!ZSTD_isError(ret))
		return 0;

	/*
	 * zstd_get_error_name() cannot be used because error takes a char *
	 * not a const char *
	 */
	switch (err) {
	case ZSTD_error_memory_allocation:
		puts("ZSTD decompressor ran out of memory");
		break;
	case ZSTD_error_prefix_unknown:
		puts("Input is not in the ZSTD format (wrong magic bytes)");
		break;
	case ZSTD_error_dstSize_tooSmall:
	case ZSTD_error_corruption_detected:
	case ZSTD_error_checksum_wrong:
		puts("ZSTD-compressed data is corrupt");
		break;
	default:
		puts("ZSTD-compressed data is probably corrupt");
		break;
	}
	return -1;
}

/*
 * Handle the case where we have the entire input and output in one segment.
 * We can allocate less memory (no circular buffer for the sliding window),
 * and avoid some memcpy() calls.
 */
static size_t decompress_single(const u8 *in_buf, long in_len, u8 *out_buf,
				long out_len)
{
	const size_t wksp_size = ZSTD_estimateDCtxSize();
	void *wksp = &free_mem_start;
	ZSTD_DCtx *dctx = ZSTD_initStaticDCtx(wksp, wksp_size);
	int err;
	size_t ret;

	if (dctx == NULL) {
		puts("Out of memory while allocating zstd_dctx");
		return 0;
	}

#if 0	// We know the correct size
	/*
	 * Find out how large the frame actually is, there may be junk at
	 * the end of the frame that zstd_decompress_dctx() can't handle.
	 */
	ret = ZSTD_findFrameCompressedSize(in_buf, in_len);
	err = handle_zstd_error(ret);
	if (err)
		return 0;
	in_len = (long)ret;
#endif

	ret = ZSTD_decompressDCtx(dctx, out_buf, out_len, in_buf, in_len);
	err = handle_zstd_error(ret);
	if (err)
		return 0;

	return ret;
}

/* Called from sfx_uboot.c */
int decompress(unsigned char *inbuf, long in_len,
	       unsigned char *outbuf, long out_len, long *out_size)
{
	long real_size;

	real_size = decompress_single(inbuf, in_len, outbuf, out_len);
	if (!real_size)
		return -1;
	if (out_size)
		*out_size = real_size;

	return 0;
}
