/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include "imports.h"

#define CRCPOLY 0xedb88320

uint32_t crc32(uint32_t seed, const void* data, size_t len)
{
    uint32_t crc = seed;
    const uint8_t* src = data;
    uint32_t mult;
    int i;

    while (len--) {
        crc ^= *src++;
        for (i = 0; i < 8; i++) {
            mult = (crc & 1) ? CRCPOLY : 0;
            crc = (crc >> 1) ^ mult;
        }
    }

    return crc;
}

// https://gist.github.com/ccbrown/9722406
void dumpHex(const void *data, size_t size)
{
#ifndef NDEBUG
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    // DEBUG_PRINT("0x%08X (0x0000): ", data);
    for (i = 0; i < size; ++i) {
        DEBUG_PRINT("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size) {
            DEBUG_PRINT(" ");
            if ((i + 1) % 16 == 0) {
                DEBUG_PRINT("|  %s \n", ascii);
                if (i + 1 < size) {
                    // DEBUG_PRINT("0x%08X (0x%04X); ", data + i + 1, i + 1);
                }
            } else if (i + 1 == size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    DEBUG_PRINT(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    DEBUG_PRINT("   ");
                }
                DEBUG_PRINT("|  %s \n", ascii);
            }
        }
    }
#endif
}
