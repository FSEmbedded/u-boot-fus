#ifndef CMD_FUS_SELFTEST_SERIAL_TEST_H_
#define CMD_FUS_SELFTEST_SERIAL_TEST_H_

//---Functions for selftest---//

int test_serial(char *szStrBuffer);
int init_uart(void);
void set_loopback(void *dev, int on);
struct udevice * get_debug_dev(void);
void mute_debug_port(int on);

#endif // CMD_FUS_SELFTEST_SERIAL_TEST_H_
