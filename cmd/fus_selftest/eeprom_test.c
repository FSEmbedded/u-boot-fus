/*
 * eeprom_test.c
 *
 *  Created on: May 19, 2021
 *      Author: developer
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
