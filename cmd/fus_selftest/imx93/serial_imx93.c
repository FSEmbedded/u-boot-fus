#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <fsl_lpuart.h>
#include <asm/io.h>
#include "../serial_test_dm.h"

#define LPUART_FLAG_REGMAP_32BIT_REG	BIT(0)
#define LPUART_FLAG_REGMAP_ENDIAN_BIG	BIT(1)

#define LPUART_CTRL_LOOPS	(1 <<  7)

enum lpuart_devtype {
	DEV_VF610 = 1,
	DEV_LS1021A,
	DEV_MX7ULP,
	DEV_IMX8,
	DEV_IMXRT,
};

struct lpuart_serial_plat {
	void *reg;
	enum lpuart_devtype devtype;
	ulong flags;
};

#define CTRL_TE		(1 << 19)
#define CTRL_RE		(1 << 18)

static void lpuart_write32(u32 flags, u32 *addr, u32 val)
{
	if (flags & LPUART_FLAG_REGMAP_32BIT_REG) {
		if (flags & LPUART_FLAG_REGMAP_ENDIAN_BIG)
			out_be32(addr, val);
		else
			out_le32(addr, val);
	}
}

static void lpuart_read32(u32 flags, u32 *addr, u32 *val)
{
	if (flags & LPUART_FLAG_REGMAP_32BIT_REG) {
		if (flags & LPUART_FLAG_REGMAP_ENDIAN_BIG)
			*(u32 *)val = in_be32(addr);
		else
			*(u32 *)val = in_le32(addr);
	}
}

struct udevice * get_debug_dev()
{
    int ret;
    struct udevice *dev;
    ofnode dbgnode;


	dbgnode = ofnode_get_aliases_node("serial0");
	if (!ofnode_valid(dbgnode))
		return NULL;

	ret = uclass_get_device_by_ofnode(UCLASS_SERIAL, dbgnode, &dev);
	if (!ret)
		return dev;

	return NULL;
}

void set_loopback(void *dev, int on)
{
	struct lpuart_serial_plat *plat = dev_get_plat(dev);
	struct lpuart_fsl_reg32 *base = (struct lpuart_fsl_reg32 *)plat->reg;
 	u32 val;

	lpuart_read32(plat->flags, &base->ctrl, &val);

	if( on )
	{
		val |= LPUART_CTRL_LOOPS;
		lpuart_write32(plat->flags, &base->ctrl, val);
		pinctrl_select_state(dev,"mute");
	}
	else
	{
		val &= ~LPUART_CTRL_LOOPS;
		lpuart_write32(plat->flags, &base->ctrl, val);
		pinctrl_select_state(dev,"default");
	}
}