/*
 * eeprom_test.h
 *
 *  Created on: May 19, 2021
 *      Author: developer
 */

#ifndef CMD_FUS_SELFTEST_EEPROM_TEST_H_
#define CMD_FUS_SELFTEST_EEPROM_TEST_H_

int test_eeprom(char *szStrBuffer);
int test_osm_i2c_a(char *szStrBuffer, uint8_t chip_addr);

#endif /* CMD_FUS_EEPROM_TEST_H_ */
