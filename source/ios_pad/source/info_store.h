/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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
#include "bta_hooks/bt_api.h"

typedef struct {
    uint32_t __version;
    uint8_t is_used;
    BD_ADDR address;
    uint8_t name[64];
    uint16_t vendor_id;
    uint16_t product_id;
} StoredInfo_t;

int store_update_device_info(int write);

void store_clear_removed_devices(void);

int store_add_name(uint8_t* address, uint8_t* name);

int store_add_vid_pid(uint8_t* address, uint16_t vendor_id, uint16_t product_id);

StoredInfo_t* store_get_device_info(uint8_t* address);

extern int info_message_queue;

enum {
    MESSAGE_TYPE_DI_RECORD,
    MESSAGE_TYPE_SAVE_STORE,
};

typedef struct {
    uint32_t type;
    BD_ADDR addr;
    uint8_t __padding[2];
    uint8_t data[0];
} ReportMessage_t;

void start_info_thread(void);

void stop_info_thread(void);
