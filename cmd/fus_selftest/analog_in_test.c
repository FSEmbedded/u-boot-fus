/*
 * analog_in_test.c
 *
 *  Created on: May 22, 2021
 *      Author: developer
 */
#include <common.h>
#include <dm/device.h>
#include <dm.h>
#include <i2c.h>
#include "selftest.h"
#include "analog_in_test.h"

// Bitmaps

#define OP_STAT 0x8000

#define AIN0_GND 0x4000
#define AIN1_GND 0x5000
#define AIN2_GND 0x6000
#define AIN3_GND 0x7000

#define CLR_MUX  0x8FFF

#define FSR_6V   0x0000
#define FSR_4V   0x0200
#define FSR_2V   0x0400
#define FSR_1V   0x0600
#define FSR_0V5  0x0800
#define FSR_0V25 0x0A00

#define CLR_FSR  0xF1FF

struct analog_in_data
{
	u16 sel;
	u16 volt;
	u16 tol;
	u16 fact;
	u16 div;
	u16 val;
};

struct analog_in_data ain[] = {
	/*     sel,  volt,  tol, fact, div, val */
	{ AIN0_GND, 12000, 1200,   11,   1,   0 },
	{ AIN1_GND,  5000,  500,   53,  33,   0 },
	{ AIN2_GND,  3300,  330,   11,  10,   0 },
	{ AIN3_GND,     0,  500,   53,  33,   0 },
};

#if 0
// Voltages for comparison

#define VOLT_12V 12000
#define VOLT_5V 5000
#define VOLT_3V3 3300

// +/- 10% Tolerance for comparison

#define TOL_12V 1200
#define TOL_5V 500
#define TOL_3V3 330

// Factors and dividers of the voltage dividers

// 12V fact: (11 / 1)
#define FACT_12V 11
#define DIV_12V 1
// 5V fact: (26.5 / 16.5)
#define FACT_5V 53
#define DIV_5V 33
// 3V3 fact: (11 / 10)
#define FACT_3V3 11
#define DIV_3V3 10
#endif

// I2C address is set only one time
u32 addr = 0;


int ads1015_read(struct udevice *dev, u8 reg, u16 *val){
	struct i2c_msg msgs[2];
	u8 buf[2];
	int ret = 0;

	/* Set register address */
	msgs[0].addr = addr;
	msgs[0].flags = I2C_M_STOP;
	msgs[0].len = 1;
	msgs[0].buf = &reg;
	/* Get register value */
	msgs[1].addr = addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = buf;
	/* Start xfer */
	ret = dm_i2c_xfer(dev, msgs, 2);

	*val = (buf[0] << 8) | buf[1];

	return ret;
}

int ads1015_write(struct udevice *dev, u8 reg, u16 val){
	struct i2c_msg msgs[2];
	u8 buf[2];
	int ret = 0;

	buf[0] = (val & 0xFF00) >> 8;
	buf[1] =  val & 0x00FF;

	/* Set register address */
	msgs[0].addr = addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg;
	/* Get register value */
	msgs[1].addr = addr;
	msgs[1].flags = I2C_M_STOP;
	msgs[1].len = 2;
	msgs[1].buf = buf;
	/* Start xfer */
	ret = dm_i2c_xfer(dev, msgs, 2);

	return ret;
}

u16 ain_read(struct udevice *dev, u16 ain_mux){
	u16 buf = 0;

	/* Set AIN_MUX */
	ads1015_read(dev, 0x1, &buf);
	buf = (buf & CLR_MUX) | ain_mux;
	ads1015_write(dev, 0x1, buf | OP_STAT);

	/* Wait for conversion */
	ads1015_read(dev, 0x1, &buf);
	while (!(buf >> 15))
		ads1015_read(dev, 0x1, &buf);

	/* Read Conv Reg */
	ads1015_read(dev, 0x0, &buf);

	/* Because of FSR_4V we have 2mV for one bit,
	 * so multiply by two through leaving one reserved bit at LSB */
	return buf >> 3;
}

bool ain_compare(u16 val, u16 target, u16 tol){
	return ((val < (target - tol)) || ((target + tol) < val));
}

int test_analog_in(char *szStrBuffer){

	int ret = 0;
	struct udevice *dev;
	struct udevice *devp = NULL;
	struct uclass *uc;
	ofnode subnode;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	if (uclass_get(UCLASS_I2C, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {

		subnode = dev_read_subnode(dev, "ads1015");

		if (subnode.of_offset < 0)
			continue;

		printf("ANALOG_IN.............");
		ofnode_read_u32(subnode, "reg", &addr);

	  	ret = dm_i2c_probe(dev,addr,0x0,&devp);
		if (ret){
			sprintf(szStrBuffer, "Device not found");
		}
		else
		{
			u16 i = 0;
			u16 cfg = 0;
			u16 size = sizeof(ain) / sizeof(struct analog_in_data);

			/* Set Config Reg */
			ads1015_read(devp, 0x1, &cfg);
			cfg = (cfg & CLR_FSR) | FSR_4V;
			ads1015_write(devp, 0x1, cfg);

			/* Read AINs */
			for (i=0;i<size;i++)
				ain[i].val = ain_read(devp, ain[i].sel);

			/* Voltage divider */
			for (i=0;i<size;i++)
				ain[i].val = (ain[i].val * ain[i].fact) / ain[i].div;

			/* Comparison */
			for (i=0;i<size;i++) {
				if (ain_compare(ain[i].val, ain[i].volt, ain[i].tol)) {
					if (ret)
						sprintf(szStrBuffer + strlen(szStrBuffer),", ");
					else
						ret = 1;

					sprintf(szStrBuffer + strlen(szStrBuffer),"AIN%d: %dmV != %dmV+-%dmV", i, ain[i].val, ain[i].volt, ain[i].tol);
				}
			}
		}

		if (ret)
			test_OkOrFail(-1, 1, szStrBuffer);
		else
			test_OkOrFail(ret, 1, szStrBuffer);
	}

	return ret;
}
