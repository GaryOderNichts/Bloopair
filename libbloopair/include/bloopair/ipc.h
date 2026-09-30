/*
 *   Copyright (C) 2021-2023 GaryOderNichts
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

#define BLOOPAIR_FUNC_GET_VERSION                   0
#define BLOOPAIR_FUNC_READ_CONSOLE_BDADDR           1
#define BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING        2
#define BLOOPAIR_FUNC_GET_COMMIT_HASH               3
#define BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION    4
#define BLOOPAIR_FUNC_READ_RAW_REPORT               5
#define BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG       6
#define BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING      7
#define BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION    8
#define BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG         9
#define BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING        10
#define BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION      11
#define BLOOPAIR_FUNC_GET_CONTROLLER_PAIRING        12
#define BLOOPAIR_FUNC_GET_STORED_SWITCH_PROS         13
#define BLOOPAIR_FUNC_GET_PAIRING_BY_ADDRESS         14
#define BLOOPAIR_FUNC_GET_PAIRING_CHANGE_GENERATION  15

#define BLOOPAIR_MAX_STORED_SWITCH_PROS 10

#define BLOOPAIR_VERSION_MAJOR(v) (((v) >> 16) & 0xff)
#define BLOOPAIR_VERSION_MINOR(v) (((v) >> 8) & 0xff)
#define BLOOPAIR_VERSION_PATCH(v) ((v) & 0xff)
#define BLOOPAIR_VERSION(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

typedef struct __attribute__ ((__packed__)) {
    uint8_t data[4096];
    uint8_t lib;
    uint8_t func;
    uint16_t unk;
    uint32_t unk1;
} BtrmRequest;

typedef struct __attribute__ ((__packed__)) {
    uint8_t data[4096];
    uint8_t unk[12];
} BtrmResponse;

// structure associated with BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING
typedef struct {
    uint8_t bd_address[6];
    uint8_t link_key[16];
    uint8_t name[64];
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairPairingData;

// structure associated with BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION
typedef struct {
    uint8_t controllerType;
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairControllerInformationData;

typedef struct {
    uint8_t bd_address[6];
    /* HCI Link Key Request Reply payload order, not Broadcom internal order. */
    uint8_t hci_link_key[16];
    /* Not exported from the private security-record layout. */
    uint8_t key_type;
    uint8_t controller_type;
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairControllerPairingData;

typedef struct {
    uint8_t bd_address[6];
    uint8_t controller_type;
    uint8_t reserved;
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairStoredSwitchProData;

typedef struct {
    uint8_t count;
    uint8_t reserved[3];
    BloopairStoredSwitchProData controllers[BLOOPAIR_MAX_STORED_SWITCH_PROS];
} BloopairStoredSwitchProList;

typedef struct {
    uint8_t bd_address[6];
} BloopairPairingAddressRequest;

// structure associated with
// - BLOOPAIR_FUNC_READ_RAW_REPORT
// - BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG
// - BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING
// - BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION
typedef struct {
    uint8_t handle;
    uint8_t controllerType;
} BloopairControllerRequestData;

// structure associated with
// - BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG
// - BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING
// - BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION
typedef struct {
    uint8_t controllerType;
    uint8_t bd_address[6];
    uint32_t dataSize;
    uint8_t data[];
} BloopairApplyControllerConfigurationData;
