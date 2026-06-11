#ifndef FSIMAGE_LINUX_HELPERS_H
#define FSIMAGE_LINUX_HELPERS_H
#include <stdint.h>
#include <stdbool.h>
#include <linux/compiler_attributes.h>	/* __packed */

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int uint;
typedef unsigned long ulong;

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

//#define BITS_PER_LONG 64
//#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK(h, l) ((~1UL << (h)) ^ (~0UL << (l)))


#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0)
#endif

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

#ifndef BIT
#define BIT(nr)	(1UL << (nr))
#endif

/* From arch/arm/include/asm/mach-imx/checkboot.h */
#define HAB_HEADER         0x40

struct __packed boot_data {
        uint32_t        start;
        uint32_t        length;
        uint32_t        plugin;
};


//#undef CONFIG_VAL
//#define _CONFIG_VAL(option) CONFIG_ ## option
//#define CONFIG_VAL(option) _CONFIG_VAL(option)
#undef _CONFIG_PREFIX
#define _CONFIG_PREFIX

#if 0 //###
#define HASH_MAX_DIGEST_SIZE	64

#define _CONFIG_IS_ENABLED(x) CONFIG_##x
#define CONFIG_IS_ENABLED(x) _CONFIG_IS_ENABLED(x)
#endif //###

u32 fdt_getprop_u32_default_node(const void *fdt, int off, int cell,
				 const char *prop, const u32 dflt);

unsigned int fuse_read(int bank, int word, uint32_t *buf);

int fs_image_get_start_copy(void);
int fs_image_get_start_copy_uboot(void);


#endif /* FSIMAGE_LINUX_HELPERS_H */
