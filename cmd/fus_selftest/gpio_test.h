#ifndef CMD_FUS_SELFTEST_GPIO_TEST_H_
#define CMD_FUS_SELFTEST_GPIO_TEST_H_

//---Functions for selftest---//

int test_gpio(int uclass, char *szStrBuffer);
int test_gpio_name(char *device_name, char *szStrBuffer);
int test_gpio_dev(struct udevice *dev, char *device_name, u64 *failmask); // return count of gpio pairs
int skip_node(struct udevice *dev);

#endif // CMD_FUS_SELFTEST_GPIO_TEST_H_
