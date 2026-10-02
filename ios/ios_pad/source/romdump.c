/*
 *   Copyright (C) 2026 GaryOderNichts
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
#include "romdump.h"
#include "bt_api.h"
#include "fsa.h"
#include "main.h"

// Can technically read in 251 byte chunks, but let's do 0x80 so it aligns
#define READ_CHUNK_SIZE 0x80

// >The microprocessor also includes 384 KB of ROM
#define ROM_READ_END_ADDRESS (384 * 1024)

static int fileHandle = -1;
static void* writeBuffer = NULL;

static uint32_t currentAddress;
static uint32_t endAddress;

static uint8_t BCM_VSC_Read(uint32_t address, uint8_t size, tBTM_VSC_CMPL_CB* cb)
{
    uint8_t params[5];
    params[0] = (uint8_t) address;
    params[1] = (uint8_t) (address >> 8);
    params[2] = (uint8_t) (address >> 16);
    params[3] = (uint8_t) (address >> 24);
    params[4] = size;
    return BTM_VendorSpecificCommand(0xfc4d, sizeof(params), params, cb);
}

static void read_callback(tBTM_VSC_CMPL* p1)
{
    if (p1->p_param_buf[0] != 0) {
        DEBUG_PRINT("romdump: Read failed: %x (current_address %08lx)\n", p1->p_param_buf[0], currentAddress);
        return;
    }

    if (p1->param_len != READ_CHUNK_SIZE + 1) {
        DEBUG_PRINT("romdump: Size mismatch %d vs %d\n", p1->param_len, READ_CHUNK_SIZE + 1);
        return;
    }

    memcpy(writeBuffer, p1->p_param_buf + 1, READ_CHUNK_SIZE);
    FSA_WriteFile(gFsaHandle, writeBuffer, 1, READ_CHUNK_SIZE, fileHandle, 0);
    FSA_FlushFile(gFsaHandle, fileHandle);

    // Increment address
    currentAddress += READ_CHUNK_SIZE;

    // Print status
    DEBUG_PRINT("romdump: %lx/%lx\n", currentAddress, endAddress);

    // If we reached the end address we're done
    if (currentAddress == endAddress) {
        FSA_CloseFile(gFsaHandle, fileHandle);
        IOS_Free(CROSS_PROCESS_HEAP_ID, writeBuffer);

        DEBUG_PRINT("romdump: Dump complete!\n");
        return;
    }

    if (!BCM_VSC_Read(currentAddress, READ_CHUNK_SIZE, read_callback)) {
        DEBUG_PRINT("romdump: Failed to start read\n");
    }
}

int romdump_dump(void)
{
    int res = FSA_OpenFile(gFsaHandle, BLOOPAIR_MOUNT_PATH "/wiiu/bloopair/romdump.bin", "w", &fileHandle);
    if (res < 0) {
        printf("romdump: Failed to create file\n");
        return -1;
    }

    writeBuffer = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, READ_CHUNK_SIZE, 0x40);
    if (!writeBuffer) {
        printf("romdump: Failed to allocate write buffer\n");
        return -1;
    }

    // Start dumping at address 0
    currentAddress = 0;
    endAddress = ROM_READ_END_ADDRESS;
    if (!BCM_VSC_Read(currentAddress, READ_CHUNK_SIZE, read_callback)) {
        DEBUG_PRINT("romdump: Failed to start read\n");
    }

    return 0;
}
