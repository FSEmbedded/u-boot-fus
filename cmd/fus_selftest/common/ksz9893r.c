/*
 * ksz9893r.c
 *
 *  Created on: May 5, 2021
 *      Author: developer
 */
#include <common.h>
#include <dm.h>
#include <i2c.h>

#include "ksz9893r.h"

#define KSZ9893R_PORT_ADDR(PORT) (PORT << 12)

#define KSZ9893R_SLAVE_ADDR		0x5F
#define KSZ9893R_CHIP_ID_MSB	0x1
#define KSZ9893R_CHIP_ID_LSB	0x2
#define KSZ9893R_CHIP_ID		0x9893
#define KSZ9893R_REG_PHYCTRL(PORT)	KSZ9893R_PORT_ADDR(PORT) + 0x100
#define KSZ9893R_PWR_DOWN		BIT(3)
#define KSZ9893R_RESTART_AUTONEG		BIT(1)
#define KSZ9893R_REG_PORT_3_CTRL_1	0x3301
#define KSZ9893R_XMII_MODES		BIT(2)

static int ksz9893r_check_id(struct udevice *ksz9893_dev)
{
	uint8_t val = 0;
	uint16_t chip_id = 0;
	int ret;

	ret = dm_i2c_read(ksz9893_dev, KSZ9893R_CHIP_ID_MSB, &val, sizeof(val));
	if (ret != 0) {
		printf("%s: Can´t access ksz9893r %d\n", __func__, ret);
		return ret;
	}
	chip_id |= val << 8;
	ret = dm_i2c_read(ksz9893_dev, KSZ9893R_CHIP_ID_LSB, &val, sizeof(val));
	if (ret != 0) {
		printf("%s: Can´t access ksz9893r %d\n", __func__, ret);
		return ret;
	}
	chip_id |= val;

	if (KSZ9893R_CHIP_ID == chip_id)
		return 0;
	else
		return 1;
}

int ksz9893r_power_port(int i2c_bus, int port_mask)
{
	struct udevice *bus = 0;
	struct udevice *ksz9893_dev = NULL;
	int ret;
	bool port1 = port_mask & 0x1;
	bool port2 = (port_mask >> 1) & 0x1;
	uint8_t val = 0;

	ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
	if (ret)
		return -EINVAL;

	ret = dm_i2c_probe(bus, KSZ9893R_SLAVE_ADDR, 0, &ksz9893_dev);
	if (ret)
		return -ENODEV;

	/* offset - 16-bit address */
	i2c_set_chip_offset_len(ksz9893_dev, 2);

	/* check id if ksz9893 is available */
	ret = ksz9893r_check_id(ksz9893_dev);
	if (ret != 0)
		return ret;

	/* Disable port 1 */
	dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PHYCTRL(1), &val, sizeof(val));
	val |= KSZ9893R_PWR_DOWN;
	dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PHYCTRL(1), &val, sizeof(val));
	/* Disable port 2 */
	dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PHYCTRL(2), &val, sizeof(val));
	val |= KSZ9893R_PWR_DOWN;
	dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PHYCTRL(2), &val, sizeof(val));

	if (port1) {
		/* Enable port 1 */
		dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PHYCTRL(1), &val, sizeof(val));
		val &= ~KSZ9893R_PWR_DOWN;
		dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PHYCTRL(1), &val, sizeof(val));
		mdelay(2);
		dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PHYCTRL(1), &val, sizeof(val));
		val |= KSZ9893R_RESTART_AUTONEG;
		dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PHYCTRL(1), &val, sizeof(val));
	}
	if (port2) {
		/* Enable port 2 */
		dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PHYCTRL(2), &val, sizeof(val));
		val &= ~KSZ9893R_PWR_DOWN;
		dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PHYCTRL(2), &val, sizeof(val));
		mdelay(2);
		dm_i2c_read(ksz9893_dev, KSZ9893R_REG_PHYCTRL(2), &val, sizeof(val));
		val |= KSZ9893R_RESTART_AUTONEG;
		dm_i2c_write(ksz9893_dev, KSZ9893R_REG_PHYCTRL(2), &val, sizeof(val));
	}

	return ret;
}
