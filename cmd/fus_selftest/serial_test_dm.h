#ifndef CMD_FUS_SELFTEST_SERIAL_FUS_H_
#define CMD_FUS_SELFTEST_SERIAL_FUS_H_

#define TIMEOUT 1000 // ms (1 sec)

#define OUTPUT 0
#define INPUT 1

#define BUFFERSIZE 128

static char tx_buffer[BUFFERSIZE];

//---Functions for selftest---//

int test_serial(void);
int init_uart(void);
void set_loopback(int port, int on);
char * get_serial_ports(int port);
int get_debug_port(void);
void mute_debug_port(int on);

#endif // CMD_FUS_SELFTEST_SERIAL_FUS_H_
