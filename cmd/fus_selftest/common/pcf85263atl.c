#include <common.h>
#include <command.h>
#include <dm.h>
#include <i2c.h>
#include <rtc.h>

#define PCF85263_REG_RTC_SC	0x01	/* Seconds */
#define PCF85263_REG_RTC_SC_OS		BIT(7)	/* Oscilator stopped flag */

#define PCF85263_REG_RTC_MN	0x02	/* Minutes */
#define PCF85263_REG_RTC_HR	0x03	/* Hours */
#define PCF85263_REG_RTC_DT	0x04	/* Day of month 1-31 */
#define PCF85263_REG_RTC_DW	0x05	/* Day of week 0-6 */
#define PCF85263_REG_RTC_MO	0x06	/* Month 1-12 */
#define PCF85263_REG_RTC_YR	0x07	/* Year 0-99 */

#define PCF85263_REG_STOPENABLE 0x2e
#define PCF85263_REG_STOPENABLE_STOP	BIT(0)

#define PCF85263_REG_RESET	0x2f	/* Reset command */
#define PCF85263_REG_RESET_CMD_CPR	0xa4	/* Clear prescaler */

#define PCF85263_MAX_REG 0x2f

static int pcf85263atl_rtc_set(struct udevice *dev, const struct rtc_time *tm)
{
	int ret;
	/*
	 * Before setting time need to stop RTC and disable prescaler
	 * Do this all in a single I2C transaction exploiting wraparound
	 * as described in data sheet.
	 * This means that the array below must be in register order
	 */
	u8 regs[] = {
		PCF85263_REG_STOPENABLE_STOP,	/* STOP */
		PCF85263_REG_RESET_CMD_CPR,	/* Disable prescaler */
		/* Wrap around to register 0 (1/100s) */
		0,				/* 1/100s always zero. */
		bin2bcd(tm->tm_sec),
		bin2bcd(tm->tm_min),
		bin2bcd(tm->tm_hour),		/* 24-hour */
		bin2bcd(tm->tm_mday),
		bin2bcd(tm->tm_wday + 1),
		bin2bcd(tm->tm_mon + 1),
		bin2bcd(tm->tm_year % 100)
	};

	/* write register's data */
	ret = dm_i2c_write(dev, PCF85263_REG_STOPENABLE, regs, sizeof(regs));
	if (ret)
		return ret;

	/* Start it again */
	regs[0] = 0;
	ret = dm_i2c_write(dev, PCF85263_REG_STOPENABLE, regs, 1);

	return ret;
}

static int pcf85263atl_rtc_get(struct udevice *dev, struct rtc_time *tm)
{
	const int first = PCF85263_REG_RTC_SC;
	const int last = PCF85263_REG_RTC_YR;
	const int len = last - first + 1;
	u8 regs[len];
	u8 hr_reg;
	int ret;

	ret = dm_i2c_read(dev, first, regs, len);
	if (ret)
		return ret;

	if (regs[PCF85263_REG_RTC_SC - first] & PCF85263_REG_RTC_SC_OS) {
		dev_warn(dev, "Oscillator stop detected, date/time is not reliable.\n");
		return -EINVAL;
	}

	tm->tm_sec = bcd2bin(regs[PCF85263_REG_RTC_SC - first] & 0x7f);
	tm->tm_min = bcd2bin(regs[PCF85263_REG_RTC_MN - first] & 0x7f);

	hr_reg = regs[PCF85263_REG_RTC_HR - first];
	tm->tm_hour = bcd2bin(hr_reg & 0x3f);

	tm->tm_mday = bcd2bin(regs[PCF85263_REG_RTC_DT - first]);
	tm->tm_wday = bcd2bin(regs[PCF85263_REG_RTC_DW - first]);
	tm->tm_mon  = bcd2bin(regs[PCF85263_REG_RTC_MO - first]) - 1;
	tm->tm_year = bcd2bin(regs[PCF85263_REG_RTC_YR - first]);

	return 0;
}

static int pcf85263atl_rtc_reset(struct udevice *dev)
{
	/*Doing nothing here*/

	return 0;
}


static const struct rtc_ops pcf85263atl_rtc_ops = {
	.get = pcf85263atl_rtc_get,
	.set = pcf85263atl_rtc_set,
	.reset = pcf85263atl_rtc_reset,
};

static const struct udevice_id pcf85263atl_rtc_ids[] = {
	{ .compatible = "nxp,pcf85263" },
	{ }
};

U_BOOT_DRIVER(rtc_pcf85263atl) = {
	.name	= "rtc-pcf85263atl",
	.id	= UCLASS_RTC,
	.of_match = pcf85263atl_rtc_ids,
	.ops	= &pcf85263atl_rtc_ops,
};

