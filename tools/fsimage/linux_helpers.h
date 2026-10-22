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

#define CMD_RET_FAILURE -1
#define CMD_RET_SUCCESS 0
#define CMD_RET_USAGE -2

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

//#define BITS_PER_LONG 64
//#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK(h, l) ((~1UL << (h)) ^ (~0UL << (l)))


#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)

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

#ifdef DEBUG
#define debug(fmt, args...) printf(fmt, ##args)
#else
#define debug(fmt, ...) do {} while (0)
#endif

#undef _CONFIG_PREFIX
#define _CONFIG_PREFIX

/* From include/image.h */

/**
 * fit_get_size() - Get FIT image size
 * @fit: pointer to the FIT format image header
 *
 * Return:
 *     size of the FIT image (blob) in memory
 *
 * The size of a FIT image is the FDT part, including all embedded images
 * (defined by data properties), but excluding all external images (defined by
 * data-offset or data-position properties). External images are appended
 * behind the FIT image part.
 */
static inline ulong fit_get_size(const void *fit)
{
	return fdt_totalsize(fit);
}

u32 fdt_getprop_u32_default_node(const void *fdt, int off, int cell,
				 const char *prop, const u32 dflt);

int fit_image_get_data_size(const void *fit, int noffset, int *data_size);
int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset);
int fit_image_get_data_position(const void *fit, int noffset,
				int *data_position);
int fit_image_get_data(const void *fit, int noffset,
		       const void **data, size_t *size);

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);
long simple_strtol(const char *cp, char **endp, unsigned int base);

int confirm_yesno(void);

#ifdef CONFIG_IMX_HAB
int imx_hab_authenticate_image(uint32_t ddr_start, uint32_t image_size,
			       uint32_t ivt_offset);
#endif

bool fs_board_is_closed(void);

#endif /* FSIMAGE_LINUX_HELPERS_H */
