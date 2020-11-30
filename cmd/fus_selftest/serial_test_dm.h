#ifndef CMD_FUS_SELFTEST_SERIAL_TEST_H_
#define CMD_FUS_SELFTEST_SERIAL_TEST_H_

//---Functions for selftest---//

int test_serial(char *szStrBuffer);
int init_uart(void);
void set_loopback(int port, int on);
char * get_serial_ports(int port);
int get_debug_port(void);
void mute_debug_port(int on);

#endif // CMD_FUS_SELFTEST_SERIAL_TEST_H_
