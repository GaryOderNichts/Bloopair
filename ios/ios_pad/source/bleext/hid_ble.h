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
#pragma once

#include "bt_api.h"

// HID Service Specification Table 2.7
enum {
    HID_REPORT_TYPE_RESERVED       = 0x00,
    HID_REPORT_TYPE_INPUT_REPORT   = 0x01,
    HID_REPORT_TYPE_OUTPUT_REPORT  = 0x02,
    HID_REPORT_TYPE_FEATURE_REPORT = 0x03,
};

typedef enum {
    HID_BLE_STATUS_SUCCESS,
    HID_BLE_STATUS_FAILURE,
} HIDBLEStatus;

typedef struct {
    HIDBLEStatus status;
    uint8_t handle;
    BD_ADDR addr;
} HIDBLEConnectEventData;

typedef struct {
    uint8_t handle;
    BD_ADDR addr;
} HIDBLEDisconnectEventData;

typedef union {
    HIDBLEConnectEventData connect;
    HIDBLEDisconnectEventData disconnect;
} HIDBLEEventData;

typedef enum {
    HID_BLE_EVENT_CONNECT,
    HID_BLE_EVENT_DISCONNECT,
} HIDBLEEvent;

typedef void (*HIDBLECallback)(HIDBLEEvent event, HIDBLEEventData* eventData);

void HID_BLE_Init(HIDBLECallback callback);

uint8_t HID_BLE_Open(BD_ADDR addr);

uint8_t HID_BLE_Close(uint8_t devHandle);

uint8_t HID_BLE_IsActiveHandle(uint8_t devHandle);

uint8_t HID_BLE_SendData(uint8_t devHandle, const void* data, uint16_t len);
