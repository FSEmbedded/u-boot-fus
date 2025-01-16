// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/rtc/rtc-pcf85363.c
 *
 * Driver for NXP PCF85363 real-time clock.
 *
 * Copyright (C) 2017 Eric Nelson
 */
#include <common.h>
#include <command.h>
#include <dm.h>
#include <log.h>
#include <rtc.h>
#include <i2c.h>

/*
 * Date/Time registers
 */
#define DT_100THS	0x00
#define DT_SECS		0x01
#define DT_MINUTES	0x02
#define DT_HOURS	0x03
#define DT_DAYS		0x04
#define DT_WEEKDAYS	0x05
#define DT_MONTHS	0x06
#define DT_YEARS	0x07

/*
 * Alarm registers
 */
#define DT_SECOND_ALM1	0x08
#define DT_MINUTE_ALM1	0x09
#define DT_HOUR_ALM1	0x0a
#define DT_DAY_ALM1	0x0b
#define DT_MONTH_ALM1	0x0c
#define DT_MINUTE_ALM2	0x0d
#define DT_HOUR_ALM2	0x0e
#define DT_WEEKDAY_ALM2	0x0f
#define DT_ALARM_EN	0x10

/*
 * Time stamp registers
 */
#define DT_TIMESTAMP1	0x11
#define DT_TIMESTAMP2	0x17
#define DT_TIMESTAMP3	0x1d
#define DT_TS_MODE	0x23

/*
 * control registers
 */
#define CTRL_OFFSET	0x24
#define CTRL_OSCILLATOR	0x25
#define CTRL_BATTERY	0x26
#define CTRL_PIN_IO	0x27
#define CTRL_FUNCTION	0x28
#define CTRL_INTA_EN	0x29
#define CTRL_INTB_EN	0x2a
#define CTRL_FLAGS	0x2b
#define CTRL_RAMBYTE	0x2c
#define CTRL_WDOG	0x2d
#define CTRL_STOP_EN	0x2e
#define CTRL_RESETS	0x2f
#define CTRL_RAM	0x40

#define ALRM_SEC_A1E	BIT(0)
#define ALRM_MIN_A1E	BIT(1)
#define ALRM_HR_A1E	BIT(2)
#define ALRM_DAY_A1E	BIT(3)
#define ALRM_MON_A1E	BIT(4)
#define ALRM_MIN_A2E	BIT(5)
#define ALRM_HR_A2E	BIT(6)
#define ALRM_DAY_A2E	BIT(7)

#define INT_WDIE	BIT(0)
#define INT_BSIE	BIT(1)
#define INT_TSRIE	BIT(2)
#define INT_A2IE	BIT(3)
#define INT_A1IE	BIT(4)
#define INT_OIE		BIT(5)
#define INT_PIE		BIT(6)
#define INT_ILP		BIT(7)

#define FLAGS_TSR1F	BIT(0)
#define FLAGS_TSR2F	BIT(1)
#define FLAGS_TSR3F	BIT(2)
#define FLAGS_BSF	BIT(3)
#define FLAGS_WDF	BIT(4)
#define FLAGS_A1F	BIT(5)
#define FLAGS_A2F	BIT(6)
#define FLAGS_PIF	BIT(7)

#define PIN_IO_INTAPM	GENMASK(1, 0)
#define PIN_IO_INTA_CLK	0
#define PIN_IO_INTA_BAT	1
#define PIN_IO_INTA_OUT	2
#define PIN_IO_INTA_HIZ	3

#define OSC_CAP_SEL		GENMASK(1, 0)
#define OSC_CAP_6000		0x01
#define OSC_CAP_12500		0x02

#define OSC_DRIVE_CNTRL 	GENMASK(3,2)
#define OSC_DRIVE_SHIFT		0x2
#define OSC_DRIVE_NORMAL	0x0
#define OSC_DRIVE_LOW		0x1
#define OSC_DRIVE_HIGH 		0x2


#define COF_MASK	GENMASK(2,0)
#define COF_32768Hz	0x0	/* clock output 32,768 kHz */
#define COF_16384Hz	0x1	/* clock output 16,384 kHz */
#define COF_8192Hz	0x2	/* clock output 8,192 kHz */
#define COF_4096Hz	0x3	/* clock output 4,096 kHz */
#define COF_2048Hz	0x4	/* clock output 2,048 kHz */
#define COF_1024Hz	0x5	/* clock output 1,024 kHz */
#define COF_1Hz		0x6	/* clock output 1 Hz */
#define COF_OFF		0x7	/* No clock output */

#define STOP_EN_STOP	BIT(0)

#define RESET_CPR	0xa4

#define NVRAM_SIZE	0x40

struct pcf85363 {
	struct udevice *dev;
    struct udevice *bus;
};

static int pcf85363_rtc_read_time(struct udevice *dev, struct rtc_time *tm)
{
	struct pcf85363 *pcf85363 = dev_get_plat(dev);
	unsigned char buf[DT_YEARS + 1];
	int ret, len = sizeof(buf);

	/* read the RTC date and time registers all at once */
	ret = dm_i2c_read(pcf85363->dev, DT_100THS, buf, len);
	if (ret) {
		printf("%s: error %d\n", __func__, ret);
		return ret;
	}

	tm->tm_year = bcd2bin(buf[DT_YEARS]);
	/* adjust for 1900 base of rtc_time */
	tm->tm_year += 100;

	tm->tm_wday = buf[DT_WEEKDAYS] & 7;
	buf[DT_SECS] &= 0x7F;
	tm->tm_sec = bcd2bin(buf[DT_SECS]);
	buf[DT_MINUTES] &= 0x7F;
	tm->tm_min = bcd2bin(buf[DT_MINUTES]);
	tm->tm_hour = bcd2bin(buf[DT_HOURS]);
	tm->tm_mday = bcd2bin(buf[DT_DAYS]);
	tm->tm_mon = bcd2bin(buf[DT_MONTHS]) - 1;

	return 0;
}

static int pcf85363_rtc_set_time(struct udevice *dev, const struct rtc_time *tm)
{
	struct pcf85363 *pcf85363 = dev_get_plat(dev);
	unsigned char tmp[11];
	unsigned char *buf = &tmp[2];
	int ret;

	tmp[0] = STOP_EN_STOP;
	tmp[1] = RESET_CPR;

	buf[DT_100THS] = 0;
	buf[DT_SECS] = bin2bcd(tm->tm_sec);
	buf[DT_MINUTES] = bin2bcd(tm->tm_min);
	buf[DT_HOURS] = bin2bcd(tm->tm_hour);
	buf[DT_DAYS] = bin2bcd(tm->tm_mday);
	buf[DT_WEEKDAYS] = tm->tm_wday;
	buf[DT_MONTHS] = bin2bcd(tm->tm_mon + 1);
	buf[DT_YEARS] = bin2bcd(tm->tm_year % 100);

	ret = dm_i2c_write(pcf85363->dev, CTRL_STOP_EN,
				tmp, 2);
	if (ret)
		return ret;

	ret = dm_i2c_write(pcf85363->dev, DT_100THS,
				buf, sizeof(tmp) - 2);
	if (ret)
		return ret;

	return dm_i2c_reg_write(pcf85363->dev, CTRL_STOP_EN, 0);
}

static int pcf85363_probe(struct udevice *dev)
{
    struct pcf85363 *pcf85363 = dev_get_plat(dev);
    
    pcf85363->dev = dev;
    pcf85363->bus = dev_get_parent(dev);

    return 0;
}

static const struct rtc_ops pcf85363_rtc_ops = {
	.get = pcf85363_rtc_read_time,
	.set = pcf85363_rtc_set_time,
};

static const struct udevice_id pcf85363_rtc_ids[] = {
	{ .compatible = "nxp,pcf85263", },
	{ .compatible = "nxp,pcf85363", },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(rtc_pcf85363) = {
	.name   = "rtc-pcf85363",
	.id     = UCLASS_RTC,
	.of_match = pcf85363_rtc_ids,
	.ops    = &pcf85363_rtc_ops,
    .probe  = pcf85363_probe,
    .plat_auto	= sizeof(struct pcf85363),
};