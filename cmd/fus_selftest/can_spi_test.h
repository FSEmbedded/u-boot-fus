/*
 * can_test.h
 *
 *  Created on: February 22, 2021
 *      Author: developer
 */

#ifndef CMD_FUS_SELFTEST_CAN_TEST_H_
#define CMD_FUS_SELFTEST_CAN_TEST_H_

/* SPI command field for MCP2518FD */
#define MCP_RESET 0x0
#define MCP_WRITE 0x2
#define MCP_READ 0x3

/* MCP registers */
#define MCP_DEVID 0xE14

struct spi_mcp_message {
	u8 addr[2];
	u8 data[4];
};


int test_can(char * szStrBuffer);


#endif /* CMD_FUS_SELFTEST_CAN_TEST_H_ */
