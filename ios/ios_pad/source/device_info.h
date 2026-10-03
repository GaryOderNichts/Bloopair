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

#pragma once

#include "bt_api.h"
#include <imports.h>

enum {
    MAGIC_EMPTY        = 0,
    MAGIC_OFFICIAL     = 0xB0,
    MAGIC_BLOOPAIR     = 0xB1,
    MAGIC_SWITCH       = 0xB2,
    MAGIC_SWITCH2      = 0xB3,
    MAGIC_BLOOPAIR_BLE = 0xB4,
    MAGIC_UNKNOWN      = 0xFF,
};

typedef struct PACKED {
    BD_ADDR address;
    // uint8_t name[64];

    /*
        Bloopair specific:
        We used to store additional information at the end of the name field.
        Everything is stored on the SD Card now, but we migrate existing pairings for now.
    */
    uint8_t name[56];
    uint8_t reserved[3];
    uint8_t magic;
    uint16_t vendor_id;
    uint16_t product_id;
} BT_DevInfo_Entry;
CHECK_SIZE(BT_DevInfo_Entry, 0x46);

typedef struct PACKED {
    BD_ADDR address;
    uint8_t name[20];
    uint8_t link_key[16];
} BT_DevInfo_WBC_Entry;
CHECK_SIZE(BT_DevInfo_WBC_Entry, 0x2a);

typedef struct PACKED {
    uint8_t num_entries;
    BT_DevInfo_Entry entries[10];
    BT_DevInfo_Entry controller_order[4];
    BT_DevInfo_WBC_Entry wbc;
    uint8_t unk[98]; // unused
} BT_DevInfo;
CHECK_SIZE(BT_DevInfo, 0x461);

typedef struct PACKED {
    uint8_t file_version;
    uint8_t device_magic;
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairStoredDeviceHeader;
CHECK_SIZE(BloopairStoredDeviceHeader, 0x6);

typedef struct PACKED {
    uint8_t address_type;
    uint8_t ltk[16];
    uint8_t rand[8];
    uint16_t ediv;
} BloopairStoredDeviceBLEData;
CHECK_SIZE(BloopairStoredDeviceBLEData, 0x1B);

typedef struct {
    uint8_t magic;
    BD_ADDR address;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t flush;

    union {
        // BR/EDR data
        struct {
            uint8_t link_key[16];
        } classic;

        // Switch 2 data
        struct {
            uint8_t pairing_complete;
            uint8_t ltk[16];
        } switch2;

        // BLE data
        struct {
            uint8_t address_type;
            uint8_t ltk[16];
            uint8_t rand[8];
            uint16_t ediv;
        } ble;
    };
} DeviceInfo;

// read the device info and add it to the store
void DeviceInfo_Init(void);

// get the info for the specified address
DeviceInfo* DeviceInfo_Get(const uint8_t* address);

// allocate a new info for the specified address
DeviceInfo* DeviceInfo_Allocate(const uint8_t* address);

DeviceInfo* DeviceInfo_GetOrAllocate(const uint8_t* address);

void DeviceInfo_AddToBgConn(DeviceInfo* info);

// read and store info from the DI record for the specified device
void DeviceInfo_ParseDIRecord(const uint8_t* bda, tSDP_DISCOVERY_DB* db);
