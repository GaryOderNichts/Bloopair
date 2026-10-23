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

void reverseBDA(uint8_t* buf, const uint8_t* bda)
{
    buf[0] = bda[5];
    buf[1] = bda[4];
    buf[2] = bda[3];
    buf[3] = bda[2];
    buf[4] = bda[1];
    buf[5] = bda[0];
}

int generateRandom(void* rand, uint32_t size)
{
    void* buffer = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, size, 0x20);
    if (!buffer) {
        return -1;
    }

    if (_ioscOpen() != 0) {
        return -1;
    }

    /* IOSC_GenerateRand */
    int res = IOS_Ioctl(cryptoHandle, 0x15, NULL, 0, buffer, size);
    if (res == 0) {
        memcpy(rand, buffer, size);
    }

    IOS_Free(CROSS_PROCESS_HEAP_ID, buffer);
    return res;
}

int* createIOSCAesKeyHandle(const void* key, uint32_t keySize)
{
    int res;

    int* keyHandle = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, sizeof(int), 0x20);
    if (!keyHandle) {
        return NULL;
    }

    if ((res = IOSC_CreateObject(keyHandle, 0, 0)) != 0) {
        IOS_Free(CROSS_PROCESS_HEAP_ID, keyHandle);
        return NULL;
    }

    void* keyBuffer = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, keySize, 0x20);
    if (!keyBuffer) {
        destroyIOSCAesKeyHandle(keyHandle);
        return NULL;
    }

    memcpy(keyBuffer, key, keySize);
    res = IOSC_ImportSecretKey(*keyHandle, 0, 0, 0, NULL, 0, NULL, 0, keyBuffer, keySize);
    IOS_Free(CROSS_PROCESS_HEAP_ID, keyBuffer);
    if (res != 0) {
        destroyIOSCAesKeyHandle(keyHandle);
        return NULL;
    }

    return keyHandle;
}

void destroyIOSCAesKeyHandle(int* handlePtr)
{
    _ioscOpen(); // Need to call this since IOSC_DeleteObject uses a different handle
    IOSC_DeleteObject(handlePtr);
    IOS_Free(CROSS_PROCESS_HEAP_ID, handlePtr);
}

int aesEcbEncrypt(int* handlePtr, const void* inData, uint32_t inSize, void* outData, uint32_t outSize)
{
    // We need to provide an IV, even if we use ECB ¯\_(ツ)_/¯
    void* iv = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, 0x10, 0x20);
    if (!iv) {
        return -1;
    }

    memset(iv, 0, 0x10);

    void* inDataBuf = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, inSize, 0x20);
    if (!inDataBuf) {
        IOS_Free(CROSS_PROCESS_HEAP_ID, iv);
        return -1;
    }

    memcpy(inDataBuf, inData, inSize);

    void* outDataBuf = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, outSize, 0x20);
    if (!outDataBuf) {
        IOS_Free(CROSS_PROCESS_HEAP_ID, iv);
        IOS_Free(CROSS_PROCESS_HEAP_ID, inDataBuf);
        return -1;
    }

    int ret = IOSC_EncryptBlocks(*handlePtr, IOSC_AES_MODE_ECB, iv, 0x10, inDataBuf, inSize, outDataBuf, outSize);
    if (ret == 0) {
        memcpy(outData, outDataBuf, outSize);
    }

    IOS_Free(CROSS_PROCESS_HEAP_ID, iv);
    IOS_Free(CROSS_PROCESS_HEAP_ID, inDataBuf);
    IOS_Free(CROSS_PROCESS_HEAP_ID, outDataBuf);
    return ret;
}

// https://gist.github.com/ccbrown/9722406
void dumpHex(const void* data, size_t size)
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
