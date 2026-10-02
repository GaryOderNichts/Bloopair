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

typedef struct GATTHandle GATTHandle;

// Table 3.5: Characteristic Properties bit field
enum {
    GATT_CHAR_PROP_BROADCAST                   = 0x01,
    GATT_CHAR_PROP_READ                        = 0x02,
    GATT_CHAR_PROP_WRITE_WITHOUT_RESPONSE      = 0x04,
    GATT_CHAR_PROP_WRITE                       = 0x08,
    GATT_CHAR_PROP_NOTIFY                      = 0x10,
    GATT_CHAR_PROP_INDICATE                    = 0x20,
    GATT_CHAR_PROP_AUTHENTICATED_SIGNED_WRITES = 0x40,
    GATT_CHAR_PROP_EXTENDED_PROPERTIES         = 0x80,
};

// Table 3.11: Client Characteristic Configuration bit field definition
enum {
    GATT_CHAR_CONF_NOTIFICATION = 0x0001,
    GATT_CHAR_CONF_INDICATION   = 0x0002,
};

typedef enum {
    GATT_STATUS_SUCCESS,
    GATT_STATUS_FAILED,
} GATTStatus;

typedef struct {
    BD_ADDR addr;
} GATTConnectEventData;

typedef struct {
    GATTStatus status;
} GATTDisconnectEventData;

typedef struct {
    uint8_t stopDiscovery;
    uint16_t startingHandle;
    uint16_t endingHandle;
    tBT_UUID uuid;
} GATTDiscoverServicesResultEventData;

typedef struct {
    GATTStatus status;
} GATTDiscoverServicesCompleteEventData;

typedef struct {
    uint8_t stopDiscovery;
    uint16_t handle;
    uint8_t properties;
    uint16_t valueHandle;
    tBT_UUID uuid;
} GATTDiscoverCharsResultEventData;

typedef struct {
    GATTStatus status;
} GATTDiscoverCharsCompleteEventData;

typedef struct {
    uint16_t handle;
    tBT_UUID uuid;
} GATTDiscoverCharDescResultEventData;

typedef struct {
    GATTStatus status;
} GATTDiscoverCharDescCompleteEventData;

typedef struct {
    GATTStatus status;
} GATTWriteEventData;

typedef struct {
    GATTStatus status;
    uint16_t handle;
    uint16_t length;
    const uint8_t* value;
} GATTReadEventData;

typedef struct {
    uint16_t handle;
    uint16_t length;
    const uint8_t* value;
} GATTNotificationEventData;

typedef union {
    GATTConnectEventData connect;
    GATTDisconnectEventData disconnect;
    GATTDiscoverServicesResultEventData servicesResult;
    GATTDiscoverServicesCompleteEventData servicesComplete;
    GATTDiscoverCharsResultEventData charsResult;
    GATTDiscoverCharsCompleteEventData charsComplete;
    GATTDiscoverCharDescResultEventData charDescResult;
    GATTDiscoverCharDescCompleteEventData charDescComplete;
    GATTWriteEventData write;
    GATTReadEventData read;
    GATTNotificationEventData notif;
} GATTEventData;

typedef enum {
    GATT_EVENT_CONNECT,
    GATT_EVENT_DISCONNECT,
    GATT_EVENT_DISCOVER_SERVICES_RESULT,
    GATT_EVENT_DISCOVER_SERVICES_COMPLETE,
    GATT_EVENT_DISCOVER_CHARS_RESULT,
    GATT_EVENT_DISCOVER_CHARS_COMPLETE,
    GATT_EVENT_DISCOVER_CHAR_DESC_RESULT,
    GATT_EVENT_DISCOVER_CHAR_DESC_COMPLETE,
    GATT_EVENT_WRITE,
    GATT_EVENT_READ,
    GATT_EVENT_NOTIFICATION,
} GATTEvent;

typedef void (*GATTCallback)(GATTHandle* handle, GATTEvent event, GATTEventData* data);
typedef GATTCallback (*GATTAutoConnectionCallback)(BD_ADDR addr);

void GATT_Init(void);

void GATT_RegisterAutoConnectionHandler(GATTAutoConnectionCallback callback);

uint8_t GATT_Connect(BD_ADDR addr, GATTCallback callback);

uint8_t GATT_CancelConnect(BD_ADDR addr);

uint8_t GATT_Disconnect(GATTHandle* handle);

void GATT_SetHandleParam(GATTHandle* handle, void* param);

void* GATT_GetHandleParam(GATTHandle* handle);

// Discover All Primary Services
uint8_t GATT_DiscoverAllPrimaryServices(GATTHandle* handle);

// Discover All Characteristics of a Service
uint8_t GATT_DiscoverChars(GATTHandle* handle, uint16_t startingHandle, uint16_t endingHandle);

// Discover All Characteristic Descriptors
uint8_t GATT_DiscoverCharDesc(GATTHandle* handle, uint16_t startingHandle, uint16_t endingHandle);

// Read Characteristic Value / Read Long Characteristic Values
uint8_t GATT_Read(GATTHandle* handle, uint16_t characteristicHandle);

// Read Using Characteristic UUID
uint8_t GATT_ReadByUUID(GATTHandle* handle, tBT_UUID characteristicUUID);

// Write Characteristic Value
uint8_t GATT_Write(GATTHandle* handle, uint16_t characteristicHandle, const void* data, uint16_t len);

// Write Without Response
uint8_t GATT_WriteNoRsp(GATTHandle* handle, uint16_t characteristicHandle, const void* data, uint16_t len);
