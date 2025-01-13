#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <malloc.h>

static int i2c_generic_probe(struct udevice *dev)
{
	struct gpio_desc *gpio = calloc(1, sizeof(struct gpio_desc));
	char * label = "enable-gpio";
	int err;

	err = gpio_request_by_name(dev, "enable-gpio", 0, gpio, GPIOD_IS_OUT);

	dm_gpio_set_value(gpio,1);
	mdelay(100);
	return  0;
};
static const struct udevice_id i2c_gerneic_ids[] = {
	{ .compatible = "i2c-generic-probe" },
	{ }
};

U_BOOT_DRIVER(i2c_generic_probe) = {
	.name	= "i2c_generic_probe",
	.id	= UCLASS_I2C_GENERIC,
	.of_match = i2c_gerneic_ids,
	.probe  = i2c_generic_probe,
	.ops	= NULL,
};
