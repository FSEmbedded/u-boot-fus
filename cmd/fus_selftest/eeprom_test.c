/*
 * eeprom_test.c
 *
 *  Created on: May 19, 2021
 *	Author: developer
 */
#include <common.h>
#include <dm/device.h>
#include <dm.h>
#include <i2c.h>
#include "selftest.h"
#include "eeprom_test.h"


int test_eeprom(char *szStrBuffer){

	int ret = 0;
	struct udevice *dev;
	struct udevice *devp = NULL;
	ofnode subnode;
	u32 reg = 0;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	uclass_foreach_dev_probe(UCLASS_I2C, dev) {

		subnode = dev_read_subnode(dev, "eeprom");
		if(!ofnode_valid(subnode))
			continue;
		ofnode_read_u32(subnode, "reg", &reg);
	  	ret = dm_i2c_probe(dev,reg,0,&devp);
		printf("EEPROM................");
		test_OkOrFail(ret, 1, szStrBuffer);

		return ret;
	}

	return 0;
}

/* alternative OSM test if epromm is not equipped */
int test_osm_i2c_a(char *szStrBuffer, uint8_t chip_addr) {
	struct udevice *bus;
	struct udevice *chip;
	uint8_t reg_val[2];
	const char *path;
	int ret;

	memset(szStrBuffer, 0, 256);

	printf("I2C_A.................");
	path = fdt_get_symbol(gd->fdt_blob, "osm_i2c_a");
	ret = uclass_get_device_by_of_path(UCLASS_I2C, path, &bus);
	if (ret) {
		snprintf(szStrBuffer, 256, "I2C_A bus not found: %d", ret);
		test_OkOrFail(ret, 1, szStrBuffer);
		return ret;
	}

	ret = dm_i2c_probe(bus, chip_addr, 0, &chip);
	if (ret) {
		snprintf(szStrBuffer, 256, "failed to Probe 0x%02x: %d", chip_addr, ret);
		test_OkOrFail(ret, 1, szStrBuffer);
		return ret;
	}

	ret = dm_i2c_read(chip, 0x01, reg_val, 2);
	if (ret) {
		snprintf(szStrBuffer, 256, "failed to read 0x%02x: %d", chip_addr, ret);
		test_OkOrFail(ret, 1, szStrBuffer);
		return ret;
	}

	snprintf(szStrBuffer, 256, "chip 0x%02x, val=0x%04x", chip_addr, reg_val[0] << 8 | reg_val[1]);
	/* Check Value against reset default */
	if (reg_val[0] == 0x85 && reg_val[1] == 0x83)
		ret = 0;
	else
		ret = -1;
	test_OkOrFail(ret, 1, szStrBuffer);

	return 0;
}
