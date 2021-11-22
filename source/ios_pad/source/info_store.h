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

typedef struct __attribute__ ((__packed__)) {
    BD_ADDR address;
    //uint8_t name[64];

/*
    Bloopair specific:
    We use the last few bytes of the name to store additional data,
    that way we don't have to create a custom userconfig entry and don't leave back
    any traces on the console
*/
    uint8_t name[59];
    uint8_t magic;
    uint16_t vendor_id;
    uint16_t product_id;
} bt_devInfo_entry_t;

typedef struct __attribute__ ((__packed__)) {
    uint8_t num_entries;
    bt_devInfo_entry_t entries[10];
    bt_devInfo_entry_t controller_order[4];
    uint8_t wbc_pairing[42]; // wii balance board pairing information?
    uint8_t unk[98]; // unused
} bt_devInfo_t;

typedef struct {
    uint8_t magic;
    BD_ADDR address;
    uint16_t vendor_id;
    uint16_t product_id;
} StoredInfo_t;

StoredInfo_t* store_get_device_info(uint8_t* address);

StoredInfo_t* store_allocate_device_info(uint8_t* address);

extern int info_message_queue;

enum {
    MESSAGE_TYPE_DI_RECORD,
};

typedef struct {
    uint32_t type;
    BD_ADDR addr;
    uint8_t __padding[2];
    uint8_t data[0];
} ReportMessage_t;

void start_info_thread(void);

void stop_info_thread(void);
