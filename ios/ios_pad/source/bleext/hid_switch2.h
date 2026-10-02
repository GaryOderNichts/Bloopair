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
#include "gatt.h"

typedef enum {
    HID_SW2_STATUS_SUCCESS,
    HID_SW2_STATUS_FAILURE,
} HIDSW2Status;

typedef struct {
    HIDSW2Status status;
    uint8_t handle;
    BD_ADDR addr;
} HIDSW2ConnectEventData;

typedef struct {
    uint8_t handle;
    BD_ADDR addr;
} HIDSW2DisconnectEventData;

typedef union {
    HIDSW2ConnectEventData connect;
    HIDSW2DisconnectEventData disconnect;
} HIDSW2EventData;

typedef enum {
    HID_SW2_EVENT_CONNECT,
    HID_SW2_EVENT_DISCONNECT,
} HIDSW2Event;

typedef void (*HIDSW2Callback)(HIDSW2Event event, HIDSW2EventData* eventData);

void HID_SW2_Init(HIDSW2Callback callback);

uint8_t HID_SW2_Open(BD_ADDR addr);

uint8_t HID_SW2_Close(uint8_t devHandle);

uint8_t HID_SW2_IsActiveHandle(uint8_t devHandle);

uint8_t HID_SW2_SendData(uint8_t devHandle, uint16_t charHandle, const void* data, uint16_t len);

uint8_t HID_SW2_EnableReport(uint8_t devHandle, uint16_t configHandle);

uint8_t HID_SW2_DisableReport(uint8_t devHandle, uint16_t configHandle);
