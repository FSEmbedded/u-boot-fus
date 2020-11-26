#ifndef CMD_FUS_SELFTEST_GPIO_TEST_H_
#define CMD_FUS_SELFTEST_GPIO_TEST_H_

//---Functions for selftest---//

int test_gpio(int uclass, char *szStrBuffer);
int init_gpio(void);
int get_array_size(int input);
char * get_gpio_name(int input, int element);
int get_gpio_val(int element);
int set_gpio_val(int element, int val);
//int set_active_gpio_element(int element);

#endif // CMD_FUS_SELFTEST_GPIO_TEST_H_
