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

#include <imports.h>
#include "bt_api.h"

enum {
    MAGIC_EMPTY    = 0,
    MAGIC_OFFICIAL = 0xB0,
    MAGIC_BLOOPAIR = 0xB1,
    MAGIC_SWITCH   = 0xB2,
    MAGIC_UNKNOWN  = 0xFF,
};

typedef struct PACKED {
    BD_ADDR address;
    //uint8_t name[64];

/*
    Bloopair specific:
    We use the last few bytes of the name to store additional data,
    that way we don't have to create a custom userconfig entry and don't leave back
    any traces on the console
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

typedef struct {
    uint8_t magic;
    BD_ADDR address;
    uint16_t vendor_id;
    uint16_t product_id;
} StoredInfo;

// read the device info and add it to the store
void store_read_device_info(void);

// get the info for the specified address
StoredInfo* store_get_device_info(uint8_t* address);

// allocate a new info for the specified address
StoredInfo* store_allocate_device_info(uint8_t* address);

// read and store info from the DI record for the specified device
void store_read_DI_record(uint8_t* bda, tSDP_DISCOVERY_DB* db);
