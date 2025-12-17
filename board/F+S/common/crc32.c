#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

uint32_t crc32(uint32_t crc, const unsigned char *p, unsigned len) {
    const uint32_t poly = 0xEDB88320;

    crc = crc ^0xFFFFFFFF;

    for(unsigned int i = 0; i < len; ++i) {
        crc ^= p[i];
        for (int j = 0; j<8; j++) {
            if(crc & 1)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}