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

#include <stdint.h>

#define BLOOPAIR_LIB 0x10

#define BLOOPAIR_FUNC_IS_ACTIVE              0
#define BLOOPAIR_FUNC_READ_DEVICE_BDADDR     1
#define BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING 2

typedef struct __attribute__ ((__packed__)) {
    uint8_t data[4096];
    uint8_t lib;
    uint8_t func;
    uint8_t unk[6];
} BtrmRequest_t;

typedef struct __attribute__ ((__packed__)) {
    uint8_t data[4108];
} BtrmResponse_t;

// structure associated with BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING
typedef struct {
    uint8_t bd_address[6];
    uint8_t link_key[16];
    uint8_t name[64];
    uint16_t vendor_id;
    uint16_t product_id;
} PairingData_t;
