/*
 * sec_test.c
 *
 *  Created on: May 22, 2021
 *      Author: developer
 */
#include <common.h>
#include <dm/device.h>
#include <dm.h>
#include <i2c.h>
#include "selftest.h"
#include "sec_test.h"


int test_sec(char *szStrBuffer){

	int ret = 0;
	struct udevice *dev;
	struct udevice *devp = NULL;
	struct uclass *uc;
	ofnode subnode;
	u32 reg = 0;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	if (uclass_get(UCLASS_I2C, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {
		subnode = dev_read_subnode(dev, "sec050");

		if (subnode.of_offset < 0)
			continue;

		printf("SEC_CHIP..............");

		ofnode_read_u32(subnode, "reg", &reg);

		/* Probe chip to set enable gpio */
		ret = i2c_get_chip(dev, reg, 1, &devp);

	  	ret = dm_i2c_probe(dev,reg,0x0,&devp);

		if (ret){
			sprintf(szStrBuffer, "Device not found");
		}

		test_OkOrFail(ret, 1, szStrBuffer);

		return ret;
	}

	return 0;
}
