/*
 * Copyright (C) 2019-2020 F&S Elektronik Systeme GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <command.h>
#include "check_config.h"
#include "ethernet_test.h"
#ifndef CONFIG_DM_SERIAL
#include "serial_test.h"
#else
#include "serial_test_dm.h"
#endif
#include "audio_test.h"
#include "dram_test.h"
#include "relay_test.h"
#include "led_test.h"
#include "gpio_test.h"
#include "usb_test.h"
#ifdef CONFIG_ENV_IS_IN_NAND
#include "nand_test.h"
#endif
#include "mmc_test.h"
#include "can_spi_test.h"
#include "eeprom_test.h"
#include "sec_test.h"
#include "analog_in_test.h"
#include "rtc_test.h"
#ifdef CONFIG_VIDEO
#include "display_test.h"
#endif
#include "pwm_test.h"
#include "pmic_test.h"
#include "processor_info.h"


static char szStrBuffer[70];

static int selftest_common(enum proto_t, cmd_tbl_t *, int, char * const []);

static int do_selftest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return selftest_common(BOOTP, cmdtp, argc, argv);
}

// U_BOOT_CMD(_name, _maxargs, _rep, _cmd, _usage, _help)

U_BOOT_CMD(
	selftest,	2,	1,	do_selftest,
	"run selftest on F&S boards",
	"[display|none]"
);


void test_OkOrFail(const int result, const int bNewline,
                          const char *pReason)
{
    /* Print result in appropriate color */
    if (result < 0)
    {
        printf("FAILED");
        printf(" ");
    }
    else if (result > 0)
    	printf("skipped");
    else
    {
    	printf("OK");
    	printf("     ");
    }

    /* Print reason, if given */
    if (pReason && *pReason)
    {
    	printf(" (");
    	printf(pReason);
    	printf(")");
    }

    /* Go to next line, if requested */
    if (bNewline)
    	printf("\n");
}

static int selftest_common(enum proto_t proto, cmd_tbl_t *cmdtp, int argc,
		char * const argv[])
{
	int ret = CMD_RET_SUCCESS;

	printf("Selftest running...\n");

	get_processorInfo();

	if (has_feature(FEAT_RTC85263))
		ret = test_rtc_start();

	ret = test_display(szStrBuffer);

	if (has_feature(FEAT_NAND))
		ret = test_nand(szStrBuffer);

	ret = test_mmc(szStrBuffer);

	if (has_feature(FEAT_CAN))
		ret = test_can(szStrBuffer);


	if (has_feature(FEAT_SEC_CHIP))
		ret = test_sec(szStrBuffer);

	ret = test_pwm(szStrBuffer);

	ret = test_analog_in(szStrBuffer);

	ret = test_relay(szStrBuffer);

	ret = test_led(szStrBuffer);

	if (has_feature(FEAT_EEPROM))
		ret = test_eeprom(szStrBuffer);

	ret = test_USBHost(szStrBuffer);

	ret = test_ethernet(szStrBuffer);

	ret = test_serial(szStrBuffer);

	if (has_feature(FEAT_SGTL5000))
		ret = test_audio(szStrBuffer);

	ret = test_gpio(UCLASS_SPI, szStrBuffer);

	ret = test_gpio(UCLASS_I2C, szStrBuffer);

	ret = test_gpio(UCLASS_MMC, szStrBuffer);

	ret = test_gpio(UCLASS_GPIO, szStrBuffer);

	ret = test_pmic(szStrBuffer);

	if (has_feature(FEAT_RTC85263))
		ret = test_rtc_end(szStrBuffer);

	ret = test_ram(szStrBuffer);

	printf("Selftest done!\n");
#if 0
	printf("\n\n");

	switch (argc) {
	case 1:
		return CMD_RET_USAGE;
		break;

	case 2:
		printf("argv[1]: %s\n", argv[1]);
		break;

	default:
		return CMD_RET_SUCCESS;
	}
#endif
	return ret;

}
