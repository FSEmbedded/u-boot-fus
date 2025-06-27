#ifndef FSIMAGE_LINUX_HELPERS_H
#define FSIMAGE_LINUX_HELPERS_H
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int uint;
typedef unsigned long ulong;

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

#define BITS_PER_LONG 64
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0)
#endif

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

#define HASH_MAX_DIGEST_SIZE	64

#define TESTVAL_1 1
#define _CONFIG_IS_ENABLED(x) TESTVAL_##x
#define CONFIG_IS_ENABLED(x) _CONFIG_IS_ENABLED(x)

#endif /* FSIMAGE_LINUX_HELPERS_H */