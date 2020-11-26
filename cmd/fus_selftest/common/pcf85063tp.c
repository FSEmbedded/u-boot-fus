#include <common.h>
#include <command.h>
#include <dm.h>
#include <i2c.h>
#include <rtc.h>

#define PCF85063_REG_CTRL1		0x00 /* status */
#define PCF85063_REG_CTRL1_STOP		BIT(5)
#define PCF85063_REG_CTRL2		0x01

#define PCF85063_REG_SC			0x04 /* datetime */
#define PCF85063_REG_SC_OS		0x80
#define PCF85063_REG_MN			0x05
#define PCF85063_REG_HR			0x06
#define PCF85063_REG_DM			0x07
#define PCF85063_REG_DW			0x08
#define PCF85063_REG_MO			0x09
#define PCF85063_REG_YR			0x0A

static int pcf85063tp_rtc_set(struct udevice *dev, const struct rtc_time *tm)
{

	int ret;
	u8 regs[7];

	/* hours, minutes and seconds */
	regs[0] = bin2bcd(tm->tm_sec) & 0x7F; /* clear OS flag */
	regs[1] = bin2bcd(tm->tm_min);
	regs[2] = bin2bcd(tm->tm_hour);
	/* Day of month, 1 - 31 */
	regs[3] = bin2bcd(tm->tm_mday);
	/* Day, 0 - 6 */
	regs[4] = tm->tm_wday & 0x07;
	/* month, 1 - 12 */
	regs[5] = bin2bcd(tm->tm_mon + 1);
	/* year and century */
	regs[6] = bin2bcd(tm->tm_year - 100);

	/* write register's data */
	ret = dm_i2c_write(dev, PCF85063_REG_SC, regs, sizeof(regs));

	return ret;
}

static int pcf85063tp_rtc_get(struct udevice *dev, struct rtc_time *tm)
{
	int ret = 0;
	u8 regs[7];

	/*
	 * while reading, the time/date registers are blocked and not updated
	 * anymore until the access is finished. To not lose a second
	 * event, the access must be finished within one second. So, read all
	 * time/date registers in one turn.
	 */

	ret = dm_i2c_read(dev, PCF85063_REG_SC, regs, sizeof(regs));
	if (ret < 0)
		return ret;

	/* if the clock has lost its power it makes no sense to use its time */
	if (regs[0] & PCF85063_REG_SC_OS) {
		dev_warn(&client->dev, "Power loss detected, invalid time\n");
		return -1;
	}

	tm->tm_sec = bcd2bin(regs[0] & 0x7F);
	tm->tm_min = bcd2bin(regs[1] & 0x7F);
	tm->tm_hour = bcd2bin(regs[2] & 0x3F); /* rtc hr 0-23 */
	tm->tm_mday = bcd2bin(regs[3] & 0x3F);
	tm->tm_wday = regs[4] & 0x07;
	tm->tm_mon = bcd2bin(regs[5] & 0x1F) - 1; /* rtc mn 1-12 */
	tm->tm_year = bcd2bin(regs[6]);
	tm->tm_year += 100;

	return ret;
}

static int pcf85063tp_rtc_reset(struct udevice *dev)
{
	/*Doing nothing here*/

	return 0;
}


static const struct rtc_ops pcf85063tp_rtc_ops = {
	.get = pcf85063tp_rtc_get,
	.set = pcf85063tp_rtc_set,
	.reset = pcf85063tp_rtc_reset,
};

static const struct udevice_id pcf85063tp_rtc_ids[] = {
	{ .compatible = "nxp,pcf85063" },
	{ }
};

U_BOOT_DRIVER(rtc_pcf85063tp) = {
	.name	= "rtc-pcf85063tp",
	.id	= UCLASS_RTC,
	.of_match = pcf85063tp_rtc_ids,
	.ops	= &pcf85063tp_rtc_ops,
};

