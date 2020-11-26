/*
 * pmic_test.c
 *
 *  Created on: May 25, 2020
 *      Author: developer
 */
#include <common.h>
#include <dm/device.h>
#include <dm.h>
#include <i2c.h>
#include "selftest.h"
#include "pmic_test.h"


int test_pmic(char *szStrBuffer){

	int ret = 0;
	static struct udevice *dev;
	static struct udevice *devp = NULL;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';
	/* Set hard by now. Should be done in DTS */

	if (uclass_get_device(UCLASS_PMIC,0,&dev) == 0) {

		printf("PMIC..................");
		struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
		struct udevice *bus = dev_get_parent(dev);

	  	ret = dm_i2c_probe(bus,chip->chip_addr,chip->flags,&devp);
		if (ret){
			sprintf(szStrBuffer, "Device not found");
		}

		test_OkOrFail(ret, 1, szStrBuffer);

		return ret;

	}
	return 0;
}
