/*
 * pwm_test.c
 *
 *  Created on: April 29, 2021
 *      Author: developer
 */
#include <common.h>
#include <dm/device.h>
#include <dm/pinctrl.h>
#include <dm.h>
#include <asm/gpio.h>
#include <malloc.h>
#include "selftest.h"
#include "pwm_test.h"


int test_pwm(char *szStrBuffer){

	int ret = 0;
	struct udevice *dev;
	struct uclass *uc;
	struct gpio_desc *test_gpio;
	int i,j;
	int size = 0;
	u32 *freqs;
	u32 *times;
	u32 cycle;
	u32 duty;

	/* Clear reason-string */
	szStrBuffer[0] = '\0';

	if (uclass_get(UCLASS_PWM, &uc))
		return 1;

	uclass_foreach_dev(dev, uc) {

		pinctrl_select_state(dev,"pwmgpio");

		test_gpio = calloc(1, sizeof(struct gpio_desc));
		if (gpio_request_by_name(dev, "gpio", 0, test_gpio, GPIOD_IS_OUT))
			continue;

		size = ofnode_read_size(dev->node, "freqs");
		if (size <= 0)
			continue;
		size = size / sizeof(u32);
		freqs = calloc(size,sizeof(u32));
		ofnode_read_u32_array(dev->node, "freqs", freqs, size);

		size = ofnode_read_size(dev->node, "times");
		if (size <= 0)
			continue;
		size = size / sizeof(u32);
		times = calloc(size,sizeof(u32));
		ofnode_read_u32_array(dev->node, "times", times, size);

		for (i=0; i<size; i++) {
			if (*(freqs+i)) {
				duty = 500000 / *(freqs+i); // usec and half a period
				cycle = (*(times+i) * 1000) / (duty * 2); // msec to usec and divide by cycletime (duty*2)
				for (j = 0; j < cycle; j++) {
					dm_gpio_set_value(test_gpio,0);
					udelay(duty);
					dm_gpio_set_value(test_gpio,1);
					udelay(duty);
				}
			}
			else {
				/* Handle frequency 0 as pause */
				cycle = (*(times+i) * 1000); // msec to usec for delay
				udelay(cycle);
			}
			if (i==0)
				sprintf(szStrBuffer,"Freq%d: %dHz, Time: %dms",i,*(freqs+i),*(times+i));
			else
				sprintf(szStrBuffer + strlen(szStrBuffer),", Freq%d: %dHz, Time: %dms",i,*(freqs+i),*(times+i));
		}

		printf("PWM...................");
		test_OkOrFail(ret, 1, szStrBuffer);
	}

	return 0;
}
