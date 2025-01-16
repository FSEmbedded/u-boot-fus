/*
 * rtc.h
 *
 *  Created on: Apr 15, 2020
 *      Author: developer
 */

#ifndef CMD_FUS_SELFTEST_AUDIO_TEST_H_
#define CMD_FUS_SELFTEST_AUDIO_TEST_H_

int test_audio(char *szStrBuffer);

int config_sai(struct udevice *dev);
int config_sgtl(struct udevice *dev);
int run_audioTest(struct udevice *dev, uint8_t *data_Send, uint8_t *data_recv, uint32_t len);

#endif /* CMD_FUS_SELFTEST_AUDIO_TEST_H_ */
