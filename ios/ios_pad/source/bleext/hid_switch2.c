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
#include "hid_switch2.h"
#include "device_info.h"
#include "main.h"
#include "stack/hid.h"
#include "stack/l2c.h"

// Switch 2 "HID" support over GATT, based on
// - https://github.com/ndeadly/switch2_controller_research/blob/master/bluetooth_interface.md

#define HID_SW2_MAX_REPORT_LEN  128

typedef enum {
    HID_SW2_STATE_INIT,
    HID_SW2_STATE_CONNECT,
    HID_SW2_STATE_CONNECTED,
} HIDSW2State;

typedef struct {
    uint8_t inUse;
    HIDSW2State state;
    GATTHandle* gattHandle;
    BD_ADDR addr;
    uint8_t devHandle;
    uint8_t connectFailure;

} HIDSW2Handle;

static HIDSW2Callback hid_callback;
static HIDSW2Handle hid_handles[BTA_HH_MAX_KNOWN];

/*--------------------*/
/* Internal functions */
/*--------------------*/

static HIDSW2Handle* _HID_SW2_AllocateHandle(void)
{
    // Find unused handle
    HIDSW2Handle* handle = NULL;
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (!hid_handles[i].inUse) {
            handle = &hid_handles[i];
            break;
        }
    }

    if (handle) {
        memset(handle, 0, sizeof(HIDSW2Handle));
        handle->inUse = 1;
    }

    return handle;
}

static inline HIDSW2Handle* _HID_SW2_GetHandleFromGATTHandle(GATTHandle* gattHandle)
{
    return (HIDSW2Handle*) GATT_GetHandleParam(gattHandle);
}

static HIDSW2Handle* _HID_SW2_GetHandleFromHidHostHandle(uint8_t devHandle)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        HIDSW2Handle* handle = &hid_handles[i];
        if (handle->inUse && handle->devHandle == devHandle) {
            return handle;
        }
    }

    return NULL;
}

static HIDSW2Handle* _HID_SW2_GetHandleFromBDA(const BD_ADDR bda)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        HIDSW2Handle* handle = &hid_handles[i];
        if (handle->inUse && memcmp(handle->addr, bda, sizeof(handle->addr)) == 0) {
            return handle;
        }
    }

    return NULL;
}

static void _HID_SW2_FreeHandle(HIDSW2Handle* handle)
{
    handle->inUse = 0;
}

static void _HID_SW2_ConnectionComplete(HIDSW2Handle* handle)
{
    // Update conn params to improve latency
    L2CA_UpdateBleConnParams(handle->addr, 6, 6, 0, 100);

    // Notify upper layer
    HIDSW2EventData eventData;
    eventData.connect.status = HID_SW2_STATUS_SUCCESS;
    eventData.connect.handle = handle->devHandle;
    memcpy(eventData.connect.addr, handle->addr, sizeof(BD_ADDR));
    hid_callback(HID_SW2_EVENT_CONNECT, &eventData);
}

/*------------------------*/
/* GATT callback handlers */
/*------------------------*/

static void _HID_SW2_Handle_Connect_Event(HIDSW2Handle* handle, GATTHandle* gattHandle, GATTConnectEventData* data)
{
    DEBUG_PRINT("SW2: Device %s connected!\n", bdaddr_to_string(data->addr));

    if (handle) {
        DEBUG_PRINT("SW2: Handle already exists??\n");
        GATT_Disconnect(gattHandle);
        return;
    }

    uint8_t bgconn = 0;
    handle = _HID_SW2_GetHandleFromBDA(data->addr);
    if (!handle) {
        // If we're here we got a connection without initiating it
        // most likely a background connection
        handle = _HID_SW2_AllocateHandle();
        bgconn = 1;
    }

    if (!handle) {
        DEBUG_PRINT("SW2: Cannot allocate handle\n");
        GATT_Disconnect(gattHandle);
        return;
    }

    GATT_SetHandleParam(gattHandle, handle);
    handle->gattHandle = gattHandle;
    memcpy(handle->addr, data->addr, sizeof(BD_ADDR));
    handle->devHandle = BTA_HH_INVALID_HANDLE;

    // Let's steal a handle from the HID host :p
    uint8_t devHandle;
    if (HID_HostAddDev(handle->addr, 0, &devHandle) != 0) {
        DEBUG_PRINT("HID: HID Host is out of handles!\n");
        GATT_Disconnect(gattHandle);
        return;
    }

    handle->devHandle = devHandle;

    DeviceInfo* info = DeviceInfo_Get(handle->addr);
    if (info) {
        // If this was a background connection, we don't need to start pairing
        info->switch2.pairing_complete = bgconn;
    }

    // Not much more to do here for switch 2
    handle->state = HID_SW2_STATE_CONNECTED;
    _HID_SW2_ConnectionComplete(handle);
}

static void _HID_SW2_Handle_Disconnect_Event(HIDSW2Handle* handle, GATTDisconnectEventData* data)
{
    // If connect failed, don't bother sending a disconnect, a connect with failure was already sent
    if (!handle->connectFailure) {
        // If we disconnected before reaching connected state without error, treat as connect failure
        if (handle->state != HID_SW2_STATE_CONNECTED) {
            HIDSW2EventData eventData;
            eventData.connect.status = HID_SW2_STATUS_FAILURE;
            eventData.connect.handle = BTA_HH_INVALID_HANDLE;
            memcpy(eventData.connect.addr, handle->addr, sizeof(BD_ADDR));
            hid_callback(HID_SW2_EVENT_CONNECT, &eventData);
        } else {
            HIDSW2EventData eventData;
            memcpy(eventData.disconnect.addr, handle->addr, sizeof(BD_ADDR));
            eventData.disconnect.handle = handle->devHandle;
            hid_callback(HID_SW2_EVENT_DISCONNECT, &eventData);
        }
    }

    HID_HostRemoveDevNoClose(handle->devHandle);
    _HID_SW2_FreeHandle(handle);
}

static void _HID_SW2_Handle_Write_Event(HIDSW2Handle* handle, GATTWriteEventData* data)
{
    if (data->status != GATT_STATUS_SUCCESS) {
        DEBUG_PRINT("SW2: Write failed\n");
    }
}

static void _HID_SW2_Handle_Notification_Event(HIDSW2Handle* handle, GATTNotificationEventData* data)
{
    uint8_t buf[HID_SW2_MAX_REPORT_LEN];

    if (data->length > HID_SW2_MAX_REPORT_LEN - 2) {
        DEBUG_PRINT("SW2: Notification too long\n");
        return;
    }

    // Prepend handle value to buffer
    buf[0] = data->handle >> 8;
    buf[1] = data->handle & 0xff;
    memcpy(buf + 2, data->value, data->length);

    // Call callback
    bta_hh_co_data(handle->devHandle, buf, data->length + 2, 0, 4, 0, NULL, 3);
}

static void _HID_SW2_GATT_Callback(GATTHandle* gattHandle, GATTEvent event, GATTEventData* data)
{
    HIDSW2Handle* handle = _HID_SW2_GetHandleFromGATTHandle(gattHandle);
    if (event != GATT_EVENT_CONNECT && !handle) {
        DEBUG_PRINT("SW2: No handle for event %d?\n", event);
        return;
    }

    switch (event) {
    case GATT_EVENT_CONNECT:
        _HID_SW2_Handle_Connect_Event(handle, gattHandle, &data->connect);
        break;
    case GATT_EVENT_DISCONNECT:
        _HID_SW2_Handle_Disconnect_Event(handle, &data->disconnect);
        break;
    case GATT_EVENT_DISCOVER_SERVICES_RESULT:
        break;
    case GATT_EVENT_DISCOVER_SERVICES_COMPLETE:
        break;
    case GATT_EVENT_DISCOVER_CHARS_RESULT:
        break;
    case GATT_EVENT_DISCOVER_CHARS_COMPLETE:
        break;
    case GATT_EVENT_DISCOVER_CHAR_DESC_RESULT:
        break;
    case GATT_EVENT_DISCOVER_CHAR_DESC_COMPLETE:
        break;
    case GATT_EVENT_READ:
        break;
    case GATT_EVENT_WRITE:
        _HID_SW2_Handle_Write_Event(handle, &data->write);
        break;
    case GATT_EVENT_NOTIFICATION:
        _HID_SW2_Handle_Notification_Event(handle, &data->notif);
        break;
    default:
        DEBUG_PRINT("SW2: Unhandled GATT event\n");
        break;
    }
}

static GATTCallback _HID_SW2_GATT_AutoConnectionHandler(BD_ADDR addr)
{
    DeviceInfo* info = DeviceInfo_Get(addr);
    if (!info) {
        return NULL;
    }

    if (info->magic != MAGIC_SWITCH2) {
        return NULL;
    }

    return &_HID_SW2_GATT_Callback;
}

/*-------------------*/
/* Public facing API */
/*-------------------*/

void HID_SW2_Init(HIDSW2Callback callback)
{
    GATT_Init();

    // Handle auto connections
    GATT_RegisterAutoConnectionHandler(&_HID_SW2_GATT_AutoConnectionHandler);

    hid_callback = callback;
}

uint8_t HID_SW2_Open(BD_ADDR addr)
{
    DEBUG_PRINT("HID_SW2_Open: %s\n", bdaddr_to_string(addr));

    HIDSW2Handle* handle = _HID_SW2_GetHandleFromBDA(addr);
    if (handle) {
        // TODO this doesn't really work, we should prevent this from happening in the first place
        DEBUG_PRINT("SW2: Error attempting to open with active handle\n");
        return 0;
    }

    handle = _HID_SW2_AllocateHandle();
    if (!handle) {
        DEBUG_PRINT("SW2: Cannot allocate handle\n");
        return 0;
    }

    handle->gattHandle = NULL;
    memcpy(handle->addr, addr, sizeof(handle->addr));

    handle->state = HID_SW2_STATE_CONNECT;
    return GATT_Connect(handle->addr, &_HID_SW2_GATT_Callback);
}

uint8_t HID_SW2_Close(uint8_t devHandle)
{
    HIDSW2Handle* handle = _HID_SW2_GetHandleFromHidHostHandle(devHandle);
    if (!handle) {
        return 0;
    }

    GATT_Disconnect(handle->gattHandle);
    return 1;
}

uint8_t HID_SW2_IsActiveHandle(uint8_t devHandle)
{
    return _HID_SW2_GetHandleFromHidHostHandle(devHandle) != NULL;
}

uint8_t HID_SW2_SendData(uint8_t devHandle, uint16_t charHandle, const void* data, uint16_t len)
{
    HIDSW2Handle* handle = _HID_SW2_GetHandleFromHidHostHandle(devHandle);
    if (!handle) {
        return 0;
    }

    GATT_WriteNoRsp(handle->gattHandle, charHandle, data, len);
    return 1;
}

uint8_t HID_SW2_EnableReport(uint8_t devHandle, uint16_t configHandle)
{
    HIDSW2Handle* handle = _HID_SW2_GetHandleFromHidHostHandle(devHandle);
    if (!handle) {
        return 0;
    }

    uint16_t value = bswap16(GATT_CHAR_CONF_NOTIFICATION); // Set notification bit
    GATT_Write(handle->gattHandle, configHandle, &value, sizeof(value));
    return 1;
}

uint8_t HID_SW2_DisableReport(uint8_t devHandle, uint16_t configHandle)
{
    HIDSW2Handle* handle = _HID_SW2_GetHandleFromHidHostHandle(devHandle);
    if (!handle) {
        return 0;
    }

    uint16_t value = 0;
    GATT_Write(handle->gattHandle, configHandle, &value, sizeof(value));
    return 1;
}
