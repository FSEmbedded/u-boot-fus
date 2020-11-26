/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <dm/device.h>
#include <dm.h>
#include "audio_test.h"
#include <i2c.h>
#include "selftest.h"

const int16_t TestData[] =
{
	0x7ff8, 0x7fff, // 32767
    0x754e, 0x7ee7, // 32487
    0x6aa4, 0x7ba2, // 31650
    0x5ffa, 0x7641, // 30273
    0x5550, 0x6ed9, // 28377
    0x4aa6, 0x658c, // 25996
    0x3ffc, 0x5a82, // 23170
    0x3552, 0x4deb, // 19947
    0x2aa8, 0x4000, // 16384
    0x1ffe, 0x30fb, // 12539
    0x1554, 0x2121, // 8481
    0x0aaa, 0x10b5, // 4277
    0x0000, 0x0000, // 0
    0xf556, 0xef4b, // -4277
    0xeAAC, 0xdedf, // -8481
    0xe002, 0xcf05, // -12539
    0xd558, 0xc000, // -16384
    0xcaae, 0xb215, // -19947
    0xc004, 0xa57e, // -23170
    0xb55a, 0x9a74, // -25996
    0xaab0, 0x9127, // -28377
    0xa006, 0x89bf, // -30273
    0x955C, 0x845e, // -31650
    0x8ab2, 0x8119, // -32487
    0x8008, 0x8001, // -32767
    0x8ab2, 0x8119, // -32487
    0x955C, 0x845e, // -31650
    0xa006, 0x89bf, // -30273
    0xaab0, 0x9127, // -28377
    0xb55a, 0x9a74, // -25996
    0xc004, 0xa57e, // -23170
    0xcaae, 0xb215, // -19947
    0xd558, 0xc000, // -16384
    0xe002, 0xcf05, // -12539
    0xeAAC, 0xdedf, // -8481
    0xf556, 0xef4b, // -4277

    0x0000, 0x0000, // 0,
    0x0aaa, 0x10b5, // 4277
    0x1554, 0x2121, // 8481
    0x1ffe, 0x30fb, // 12539
    0x2aa8, 0x4000, // 16384
    0x3552, 0x4deb, // 19947
    0x3ffc, 0x5a82, // 23170
    0x4aa6, 0x658c, // 25996
    0x5550, 0x6ed9, // 28377
    0x5ffa, 0x7641, // 30273
    0x6aa4, 0x7ba2, // 31650
    0x754e, 0x7ee7, // 32487
    0x7ff8, 0x7fff, // 32767
    0x754e, 0x7ee7, // 32487
    0x6aa4, 0x7ba2, // 31650
    0x5ffa, 0x7641, // 30273
    0x5550, 0x6ed9, // 28377
    0x4aa6, 0x658c, // 25996
    0x3ffc, 0x5a82, // 23170
    0x3552, 0x4deb, // 19947
    0x2aa8, 0x4000, // 16384
    0x1ffe, 0x30fb, // 12539
    0x1554, 0x2121, // 8481
    0x0aaa, 0x10b5, // 4277
    0x0000, 0x0000, // 0
    0xf556, 0xef4b, // -4277
    0xeAAC, 0xdedf, // -8481
    0xe002, 0xcf05, // -12539
    0xd558, 0xc000, // -16384
    0xcaae, 0xb215, // -19947
    0xc004, 0xa57e, // -23170
    0xb55a, 0x9a74, // -25996
    0xaab0, 0x9127, // -28377
    0xa006, 0x89bf, // -30273
    0x955C, 0x845e, // -31650
    0x8ab2, 0x8119, // -32487
    0x8008, 0x8001, // -32767
    0x8ab2, 0x8119, // -32487
    0x955C, 0x845e, // -31650
    0xa006, 0x89bf, // -30273
    0xaab0, 0x9127, // -28377
    0xb55a, 0x9a74, // -25996
    0xc004, 0xa57e, // -23170
    0xcaae, 0xb215, // -19947
    0xd558, 0xc000, // -16384
    0xe002, 0xcf05, // -12539
    0xeAAC, 0xdedf, // -8481
    0xf556, 0xef4b, // -4277

    0x0000, 0x0000, // 0,
    0x0aaa, 0x10b5, // 4277
    0x1554, 0x2121, // 8481
    0x1ffe, 0x30fb, // 12539
    0x2aa8, 0x4000, // 16384
    0x3552, 0x4deb, // 19947
    0x3ffc, 0x5a82, // 23170
    0x4aa6, 0x658c, // 25996
    0x5550, 0x6ed9, // 28377
    0x5ffa, 0x7641, // 30273
    0x6aa4, 0x7ba2, // 31650
    0x754e, 0x7ee7, // 32487

    //0x0000, 0x0000, // 0
    //0x0000, 0x0000, // 0
    //0x0000, 0x0000, // 0
    //0x0000, 0x0000, // 0
    //0x0000, 0x0000, // 0
    //0x0000, 0x0000, // 0
    //0x0000, 0x0000, // 0
	0x0000, // 0
};

#define TEST_SIZE_BYTE ( sizeof(TestData)/sizeof(uint8_t))
#define TEST_SIZE ( sizeof(TestData)/sizeof(TestData[0])))
#define THRESHOLD 0x2000	// was 0x3000
#define NUM_ALLOWED_ERRORS 20

// main functions

int test_audio(char *szStrBuffer)
{
	struct udevice *dev;
	struct udevice *devp = NULL;
	int ret;
	int nStartLeft = 0;
	int nStartRight = 0;
	int uiErrorCounterL = 0;
	int uiErrorCounterR = 0;

	int16_t ReadData[TEST_SIZE_BYTE*2];
	int16_t *pReadData;
	memset(ReadData, 0x0, (TEST_SIZE_BYTE*2));

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	/* Set SAI clocks and registers */
	config_sai();

	/* Init codec */
	printf("I2S I2C interface.....");
	ret = uclass_get_device_by_name(UCLASS_I2C_GENERIC,"sgtl5000@0a",&dev);

	if (ret){
		/* Test skipped */
		return 1;
	}

	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
	struct udevice *bus = dev_get_parent(dev);


	ret = dm_i2c_probe(bus,chip->chip_addr,chip->flags,&devp);
	if (ret){
        sprintf(szStrBuffer, "SGTL5000 not found");
        test_OkOrFail(-1, 1, szStrBuffer);
		return -1;
	}

	ret = config_sgtl(dev);
	if (ret){
        sprintf(szStrBuffer, "Failed write codec Register");
        test_OkOrFail(-1, 1, szStrBuffer);
		return -1;
	}

	test_OkOrFail(0, 1, szStrBuffer);


	/* Test Audio */
	printf("Audio.................");

	run_audioTest((uint8_t*) TestData,(uint8_t*) ReadData,TEST_SIZE_BYTE);

	pReadData = ReadData;

	/* Search for start of signal in left channel data */
	for (nStartLeft=0; nStartLeft<TEST_SIZE_BYTE; nStartLeft+=2)
	{
		if (pReadData[nStartLeft] > 25000)
			break;
	}

	if (nStartLeft == TEST_SIZE_BYTE)
	{
		/* No data found in left channel or both channels */
        sprintf(szStrBuffer, "No data found in left channel or both channels");
        test_OkOrFail(-1, 1, szStrBuffer);
		return -1;
	}

	nStartRight = nStartLeft;

	/* Check if all values are within limits */
	for (int i=0; i<96; i+=2)
	{
		if ((TestData[i]+THRESHOLD < pReadData[i+nStartLeft])
			|| (TestData[i]-THRESHOLD > pReadData[i+nStartLeft]))
		{
			uiErrorCounterL++;
		}
		if ((TestData[i+1]+THRESHOLD < pReadData[i+nStartRight+1])
			|| (TestData[i+1]-THRESHOLD > pReadData[i+nStartRight+1]))
		{
			uiErrorCounterR++;
		}
	}

	if(uiErrorCounterL > NUM_ALLOWED_ERRORS)
	{
		sprintf(szStrBuffer, "Left signal differs too much");
		ret =-1;

	}

	if(uiErrorCounterR > NUM_ALLOWED_ERRORS)
	{
		sprintf(szStrBuffer, "Right signal differs too much");
		ret =-1;
	}

	test_OkOrFail(ret, 1, szStrBuffer);

	return ret;

}
