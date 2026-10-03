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
#include "gatt.h"
#include "att.h"
#include "stack/btm.h"
#include "stack/l2c.h"
#include "uuids.h"

#define GATT_MAX_HANDLES   16
#define GATT_MAX_CALLBACKS 2 // HID BLE + SW2
// TODO we could probably lower this for our needs
#define GATT_MAX_MTU       517
#define GATT_MAX_VALUE_LEN 600

typedef enum {
    GATT_STATE_INIT,
    GATT_STATE_WAIT_CONNECT,
    GATT_STATE_EXCHANGE_MTU,
    GATT_STATE_IDLE,
    GATT_STATE_DISCOVER_SERVICES,
    GATT_STATE_DISCOVER_CHARS,
    GATT_STATE_DISCOVER_CHAR_DESC,
    GATT_STATE_READ,
    GATT_STATE_WRITE,
} GATTState;

struct GATTHandle {
    uint8_t inUse;
    BD_ADDR addr;
    uint16_t mtu;
    TIMER_LIST_ENT timer;
    GATTCallback callback;
    void* userParam;

    GATTState state;
    union {
        struct {
            uint16_t endingHandle;
        } discoverChars;

        struct {
            uint16_t endingHandle;
        } discoverCharDesc;

        struct {
            uint16_t handle;
            uint16_t offset;
            uint8_t* buffer;
        } read;
    };
};

static int gatt_is_registered = 0;
static GATTAutoConnectionCallback auto_conn_callbacks[GATT_MAX_CALLBACKS];
static GATTHandle gatt_handles[GATT_MAX_HANDLES];

/*--------------------*/
/* Internal functions */
/*--------------------*/

static GATTCallback _GATT_GetCallbackForAutoConn(BD_ADDR addr)
{
    GATTCallback callback = NULL;

    for (int i = 0; i < GATT_MAX_CALLBACKS; i++) {
        if (!auto_conn_callbacks[i]) {
            break;
        }

        callback = auto_conn_callbacks[i](addr);
        if (callback) {
            break;
        }
    }

    return callback;
}

static GATTHandle* _GATT_AllocateHandle(BD_ADDR addr)
{
    // Find unused handle
    GATTHandle* handle = NULL;
    int i;
    for (i = 0; i < GATT_MAX_HANDLES; i++) {
        if (!gatt_handles[i].inUse) {
            handle = &gatt_handles[i];
            break;
        }
    }

    if (handle) {
        memset(handle, 0, sizeof(GATTHandle));
        handle->inUse = 1;
        memcpy(handle->addr, addr, sizeof(BD_ADDR));
    }

    return handle;
}

static GATTHandle* _GATT_GetHandleFromBDA(BD_ADDR addr)
{
    // Find the handle
    for (int i = 0; i < GATT_MAX_HANDLES; i++) {
        GATTHandle* handle = &gatt_handles[i];

        if (handle->inUse && memcmp(handle->addr, addr, sizeof(BD_ADDR)) == 0) {
            return handle;
        }
    }

    return NULL;
}

static int _GATT_CheckHandle(GATTHandle* handle)
{
    if (!handle) {
        return 0;
    }

    if (!handle->inUse) {
        return 0;
    }

    return 1;
}

static void _GATT_FreeHandle(GATTHandle* handle)
{
    // Make sure to free the buffer if a disconnect happened during read
    if (handle->inUse && handle->state == GATT_STATE_READ) {
        if (handle->read.buffer) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, handle->read.buffer);
            handle->read.buffer = NULL;
        }
    }

    bta_sys_stop_timer(&handle->timer);
    handle->inUse = 0;
}

static void _GATT_SendRequest(GATTHandle* handle, BT_HDR* buf)
{
    if (!buf) {
        DEBUG_PRINT("GATT: Trying to send NULL request\n");
        return;
    }

    if (!L2CA_SendFixedChnlData(L2CAP_ATT_CID, handle->addr, buf)) {
        // TODO do we want to disconnect in this case?
        DEBUG_PRINT("GATT: Failed to send request\n");
        // GATT_Disconnect(handle->connHandle);
    }
}

static void _GATT_Start_MTUExchange(GATTHandle* handle)
{
    handle->state = GATT_STATE_EXCHANGE_MTU;
    _GATT_SendRequest(handle, ATT_Build_ExchangeMTU_Request(GATT_MAX_MTU));
}

static void _GATT_Start_Callback(TIMER_LIST_ENT* p_tle)
{
    GATTHandle* handle = (GATTHandle*) p_tle->param;

    // Handle connected, proceed with MTU exchange
    _GATT_Start_MTUExchange(handle);
}

/*----------------------*/
/* L2C connect callback */
/*----------------------*/
static void _GATT_Connect_Callback(BD_ADDR addr, uint8_t connected, uint16_t reason)
{
    DEBUG_PRINT("_GATT_Connect_Callback %s %d %x\n", bdaddr_to_string(addr), connected, reason);

    GATTHandle* handle = _GATT_GetHandleFromBDA(addr);

    if (connected) {
        if (handle) {
            if (handle->state != GATT_STATE_WAIT_CONNECT) {
                DEBUG_PRINT("GATT: Invalid state\n");
                return;
            }
        } else {
            // Connection without an active handle, means this is a bg connection
            // Create a new handle before starting MTU exchange
            handle = _GATT_AllocateHandle(addr);
            if (!handle) {
                DEBUG_PRINT("GATT: Failed to allocate handle\n");
                return;
            }

            handle->callback = _GATT_GetCallbackForAutoConn(addr);
            if (!handle->callback) {
                // No one wants this device :(
                _GATT_FreeHandle(handle);
                btm_remove_acl(addr);
                return;
            }
        }

        // FIXME workaround for issue when issuing PDU's to quickly after establishing a connection
        // Start the pairing process delayed
        handle->timer.param = (uint32_t) handle;
        handle->timer.p_cback = (TIMER_CBACK*) _GATT_Start_Callback;
        bta_sys_start_timer(&handle->timer, 0, 250);
    } else {
        if (handle) {
            // Notify client of disconnect
            GATTEventData eventData;
            eventData.disconnect.status = GATT_STATUS_SUCCESS;
            handle->callback(handle, GATT_EVENT_DISCONNECT, &eventData);

            // Free handle
            _GATT_FreeHandle(handle);
        } else {
            // TODO a disconnect can happen before being connected on L2C timeout
        }
    }
}

/*------------------------*/
/* GATT response handlers */
/*------------------------*/

static void _GATT_Handle_Error_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (len != 4) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint8_t opcode = p[0];
    uint16_t attributeHandle = p[1] | (p[2] << 8);
    uint8_t error = p[3];

    GATTEventData eventData;
    switch (handle->state) {
    case GATT_STATE_DISCOVER_SERVICES: {
        if (opcode != ATT_READ_BY_GROUP_TYPE_REQ) {
            DEBUG_PRINT("GATT: Unsupported error response\n");
            return;
        }

        handle->state = GATT_STATE_IDLE;

        // If no more attributes found, success
        eventData.servicesComplete.status =
            (error == ATT_ERROR_ATTRIBUTE_NOT_FOUND) ? GATT_STATUS_SUCCESS : GATT_STATUS_FAILED;
        handle->callback(handle, GATT_EVENT_DISCOVER_SERVICES_COMPLETE, &eventData);
        break;
    }
    case GATT_STATE_DISCOVER_CHARS: {
        if (opcode != ATT_READ_BY_TYPE_REQ) {
            DEBUG_PRINT("GATT: Unsupported error response\n");
            return;
        }

        handle->state = GATT_STATE_IDLE;

        // If no more attributes found, success
        eventData.charsComplete.status =
            (error == ATT_ERROR_ATTRIBUTE_NOT_FOUND) ? GATT_STATUS_SUCCESS : GATT_STATUS_FAILED;
        handle->callback(handle, GATT_EVENT_DISCOVER_CHARS_COMPLETE, &eventData);
        break;
    }
    case GATT_STATE_DISCOVER_CHAR_DESC: {
        if (opcode != ATT_FIND_INFORMATION_REQ) {
            DEBUG_PRINT("GATT: Unsupported error response\n");
            return;
        }

        handle->state = GATT_STATE_IDLE;

        // If no more attributes found, success
        eventData.charDescComplete.status =
            (error == ATT_ERROR_ATTRIBUTE_NOT_FOUND) ? GATT_STATUS_SUCCESS : GATT_STATUS_FAILED;
        handle->callback(handle, GATT_EVENT_DISCOVER_CHAR_DESC_COMPLETE, &eventData);
        break;
    }
    case GATT_STATE_READ: {
        // If we did a blob read of a not long attribute the attribute did perfectly fit into the MTU
        if (opcode == ATT_READ_BLOB_REQ && error == ATT_ERROR_ATTRIBUTE_NOT_LONG) {
            eventData.read.status = GATT_STATUS_SUCCESS;
            eventData.read.handle = handle->read.handle;
            eventData.read.length = handle->read.offset;
            eventData.read.value = handle->read.buffer;

            // Get a copy of the buffer pointer to free, since the read state might be modified in the callback
            void* toFree = handle->read.buffer;
            handle->state = GATT_STATE_IDLE;

            handle->callback(handle, GATT_EVENT_READ, &eventData);

            if (toFree) {
                IOS_Free(LOCAL_PROCESS_HEAP_ID, toFree);
                handle->read.buffer = NULL;
            }
            return;
        }

        // Any other error is failure
        if (handle->read.buffer) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, handle->read.buffer);
            handle->read.buffer = NULL;
        }

        handle->state = GATT_STATE_IDLE;

        eventData.read.status = GATT_STATUS_FAILED;
        handle->callback(handle, GATT_EVENT_READ, &eventData);
        break;
    }
    case GATT_STATE_WRITE: {
        if (opcode != ATT_WRITE_REQ) {
            DEBUG_PRINT("GATT: Unsupported error response\n");
            return;
        }

        handle->state = GATT_STATE_IDLE;

        eventData.write.status = GATT_STATUS_FAILED;
        handle->callback(handle, GATT_EVENT_WRITE, &eventData);
        break;
    }
    default:
        DEBUG_PRINT("GATT: Unhandled error for op %d with error %d\n", opcode, error);
        break;
    };
}

static void _GATT_Handle_ExchangeMTU_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != GATT_STATE_EXCHANGE_MTU) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    if (len != 2) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint16_t mtu = p[0] | (p[1] << 8);
    if (mtu > GATT_MAX_MTU) {
        mtu = GATT_MAX_MTU;
    }

    DEBUG_PRINT("GATT: Exchanged MTU: %d\n", mtu);

    handle->mtu = mtu;

    // Handle is now idle and ready to accept commands
    handle->state = GATT_STATE_IDLE;

    // Call connect callback, now that the MTU is exchanged
    GATTEventData eventData;
    memcpy(eventData.connect.addr, handle->addr, sizeof(BD_ADDR));
    handle->callback(handle, GATT_EVENT_CONNECT, &eventData);
}

static void _GATT_Handle_FindInformation_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != GATT_STATE_DISCOVER_CHAR_DESC) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    if (len <= 1) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint8_t uuidFormat = *p++;
    len--;

    if (uuidFormat != 1 && uuidFormat != 2) {
        DEBUG_PRINT("GATT: Unsupported UUID length\n");
        return;
    }

    uint8_t valueLength = (uuidFormat == 1) ? 4 : 18;
    uint16_t attHandle = 0xffff;
    GATTEventData eventData;
    while (len >= valueLength) {
        attHandle = p[0] | (p[1] << 8);
        len -= 2;
        p += 2;

        eventData.charDescResult.handle = attHandle;
        if (uuidFormat == 1) {
            eventData.charDescResult.uuid.len = LEN_UUID_16;
            eventData.charDescResult.uuid.uu.uuid16 = p[0] | (p[1] << 8);
            len -= LEN_UUID_16;
            p += LEN_UUID_16;
        } else if (uuidFormat == 2) {
            eventData.charDescResult.uuid.len = LEN_UUID_128;
            memcpy(eventData.charDescResult.uuid.uu.uuid128, p, LEN_UUID_128);
            len -= LEN_UUID_128;
            p += LEN_UUID_128;
        }

        handle->callback(handle, GATT_EVENT_DISCOVER_CHAR_DESC_RESULT, &eventData);
    }

    // Already done?
    if (attHandle >= handle->discoverCharDesc.endingHandle) {
        handle->state = GATT_STATE_IDLE;

        eventData.servicesComplete.status = GATT_STATUS_SUCCESS;
        handle->callback(handle, GATT_EVENT_DISCOVER_CHAR_DESC_COMPLETE, &eventData);
        return;
    }

    // Read the next info starting at one past the last handle
    _GATT_SendRequest(handle, ATT_Build_FindInformation_Request(attHandle + 1, handle->discoverCharDesc.endingHandle));
}

static void _GATT_DiscoverChars_Handle_ReadByType_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != GATT_STATE_DISCOVER_CHARS) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    if (len <= 1) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint8_t valueLength = *p++;
    len--;

    if (valueLength < 5) {
        DEBUG_PRINT("GATT: Unsupported value length\n");
        return;
    }

    uint8_t uuidLength = valueLength - 5;
    if (uuidLength != LEN_UUID_16 && uuidLength != LEN_UUID_128) {
        DEBUG_PRINT("GATT: Unsupported UUID length\n");
        return;
    }

    // Parse results
    uint16_t attHandle = 0xffff;
    uint8_t properties;
    uint16_t valueHandle;
    GATTEventData eventData;
    while (len >= valueLength) {
        attHandle = p[0] | (p[1] << 8);
        properties = p[2];
        valueHandle = p[3] | (p[4] << 8);
        len -= 5;
        p += 5;

        eventData.charsResult.stopDiscovery = 0;
        eventData.charsResult.handle = attHandle;
        eventData.charsResult.properties = properties;
        eventData.charsResult.valueHandle = valueHandle;

        if (uuidLength == LEN_UUID_16) {
            eventData.charsResult.uuid.len = LEN_UUID_16;
            eventData.charsResult.uuid.uu.uuid16 = p[0] | (p[1] << 8);
            len -= LEN_UUID_16;
            p += LEN_UUID_16;
        } else if (uuidLength == LEN_UUID_128) {
            eventData.charsResult.uuid.len = LEN_UUID_128;
            memcpy(eventData.charsResult.uuid.uu.uuid128, p, LEN_UUID_128);
            len -= LEN_UUID_128;
            p += LEN_UUID_128;
        }

        handle->callback(handle, GATT_EVENT_DISCOVER_CHARS_RESULT, &eventData);

        // Does the event handler need to continue discovering services?
        if (eventData.charsResult.stopDiscovery) {
            attHandle = 0xffff;
            break;
        }
    }

    // Already done?
    if (attHandle >= handle->discoverChars.endingHandle) {
        handle->state = GATT_STATE_IDLE;

        eventData.charsComplete.status = GATT_STATUS_SUCCESS;
        handle->callback(handle, GATT_EVENT_DISCOVER_CHARS_COMPLETE, &eventData);
        return;
    }

    // Read the next data starting at one past the last handle
    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_GATT_CHARACTERISTIC;
    _GATT_SendRequest(handle, ATT_Build_ReadByType_Request(attHandle + 1, handle->discoverChars.endingHandle, uuid));
}

static void _GATT_ReadByUUID_Handle_ReadByType_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != GATT_STATE_READ) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    if (len < 3) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint16_t fullLength = len;
    uint8_t valueLength = *p++;
    len--;

    if (valueLength < 2) {
        DEBUG_PRINT("GATT: Unsupported value length\n");
        return;
    }

    // We start reading the handle from the first value here
    uint16_t attHandle = p[0] | (p[1] << 8);
    len -= 2;
    p += 2;

    // If the value didn't exceed the maximum and fit into the MTU we don't need to start an actual read
    if (valueLength < 255 && fullLength < handle->mtu - 1) {
        GATTEventData eventData;
        eventData.read.status = GATT_STATUS_SUCCESS;
        eventData.read.handle = attHandle;
        eventData.read.length = valueLength - 2;
        eventData.read.value = p;

        handle->state = GATT_STATE_IDLE;

        handle->callback(handle, GATT_EVENT_READ, &eventData);
        return;
    }

    // Just start a full read from the first discovered handle now
    // We could immediately start with a blob read here i guess?
    handle->read.handle = attHandle;
    handle->read.offset = 0;
    handle->read.buffer = NULL;
    _GATT_SendRequest(handle, ATT_Build_Read_Request(attHandle));
}

static void _GATT_Handle_ReadByType_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state == GATT_STATE_DISCOVER_CHARS) {
        _GATT_DiscoverChars_Handle_ReadByType_Response(handle, p, len);
    } else if (handle->state == GATT_STATE_READ) {
        _GATT_ReadByUUID_Handle_ReadByType_Response(handle, p, len);
    } else {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
    }
}

static void _GATT_Handle_Read_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    GATTEventData eventData;

    if (handle->state != GATT_STATE_READ) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    // Oh we ran out of buffer
    if (handle->read.offset + len > GATT_MAX_VALUE_LEN) {
        if (handle->read.buffer) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, handle->read.buffer);
            handle->read.buffer = NULL;
        }

        handle->state = GATT_STATE_IDLE;

        eventData.read.status = GATT_STATUS_FAILED;
        handle->callback(handle, GATT_EVENT_READ, &eventData);
        return;
    }

    // If value did fit into MTU, we're done
    if (len < handle->mtu - 1) {
        eventData.read.status = GATT_STATUS_SUCCESS;
        eventData.read.handle = handle->read.handle;

        // Shortcut, if this wasn't a long read
        if (!handle->read.buffer) {
            eventData.read.length = len;
            eventData.read.value = p;
        } else {
            memcpy(handle->read.buffer + handle->read.offset, p, len);
            handle->read.offset += len;

            eventData.read.length = handle->read.offset;
            eventData.read.value = handle->read.buffer;
        }

        // Get a copy of the potential buffer to free, since the read state might be modified in the callback
        void* toFree = handle->read.buffer;
        handle->state = GATT_STATE_IDLE;

        handle->callback(handle, GATT_EVENT_READ, &eventData);

        if (toFree) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, toFree);
            handle->read.buffer = NULL;
        }
        return;
    }

    if (!handle->read.buffer) {
        handle->read.buffer = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, GATT_MAX_VALUE_LEN);
    }

    memcpy(handle->read.buffer + handle->read.offset, p, len);
    handle->read.offset += len;

    // Continue reading at offset
    // If the offset is equal to length a zero length value response will be returned
    _GATT_SendRequest(handle, ATT_Build_ReadBlob_Request(handle->read.handle, handle->read.offset));
}

static void _GATT_Handle_ReadByGroupType_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != GATT_STATE_DISCOVER_SERVICES) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    if (len <= 1) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint8_t valueLength = *p++;
    len--;

    if (valueLength < 4) {
        DEBUG_PRINT("GATT: Unsupported value length\n");
        return;
    }

    uint8_t uuidLength = valueLength - 4;
    if (uuidLength != LEN_UUID_16 && uuidLength != LEN_UUID_128) {
        DEBUG_PRINT("GATT: Unsupported UUID length\n");
        return;
    }

    // Parse results
    uint16_t startingHandle = 0;
    uint16_t endingHandle = 0xffff;
    GATTEventData eventData;
    while (len >= valueLength) {
        startingHandle = p[0] | (p[1] << 8);
        endingHandle = p[2] | (p[3] << 8);
        len -= 4;
        p += 4;

        eventData.servicesResult.stopDiscovery = 0;
        eventData.servicesResult.startingHandle = startingHandle;
        eventData.servicesResult.endingHandle = endingHandle;

        if (uuidLength == LEN_UUID_16) {
            eventData.servicesResult.uuid.len = LEN_UUID_16;
            eventData.servicesResult.uuid.uu.uuid16 = p[0] | (p[1] << 8);
            len -= LEN_UUID_16;
            p += LEN_UUID_16;
        } else if (uuidLength == LEN_UUID_128) {
            eventData.servicesResult.uuid.len = LEN_UUID_128;
            memcpy(eventData.servicesResult.uuid.uu.uuid128, p, LEN_UUID_128);
            len -= LEN_UUID_128;
            p += LEN_UUID_128;
        }

        handle->callback(handle, GATT_EVENT_DISCOVER_SERVICES_RESULT, &eventData);

        // Does the event handler need to continue discovering services?
        if (eventData.servicesResult.stopDiscovery) {
            endingHandle = 0xffff;
            break;
        }
    }

    // Already done?
    if (endingHandle == 0xffff) {
        handle->state = GATT_STATE_IDLE;

        eventData.servicesComplete.status = GATT_STATUS_SUCCESS;
        handle->callback(handle, GATT_EVENT_DISCOVER_SERVICES_COMPLETE, &eventData);
        return;
    }

    // Read the next data starting at one past the ending handle
    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_GATT_PRIMARY_SERVICE;
    _GATT_SendRequest(handle, ATT_Build_ReadByGroupType_Request(endingHandle + 1, 0xffff, uuid));
}

static void _GATT_Handle_Write_Response(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != GATT_STATE_WRITE) {
        DEBUG_PRINT("GATT: Received response for invalid state\n");
        return;
    }

    handle->state = GATT_STATE_IDLE;

    GATTEventData eventData;
    eventData.write.status = GATT_STATUS_SUCCESS;
    handle->callback(handle, GATT_EVENT_WRITE, &eventData);
}

static void _GATT_Handle_HandleValue_Notification(GATTHandle* handle, uint8_t* p, uint16_t len)
{
    if (len < 2) {
        DEBUG_PRINT("GATT: Unsupported packet length\n");
        return;
    }

    uint16_t attHandle = p[0] | (p[1] << 8);
    len -= 2;
    p += 2;

    GATTEventData eventData;
    eventData.notif.handle = attHandle;
    eventData.notif.length = len;
    eventData.notif.value = p;
    handle->callback(handle, GATT_EVENT_NOTIFICATION, &eventData);
}

/*-------------------*/
/* L2C data callback */
/*-------------------*/
static void _GATT_Data_Callback(BD_ADDR addr, BT_HDR* buf)
{
    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;
    uint16_t len = buf->len;

    if (len == 0) {
        GKI_freebuf(buf);
        return;
    }

    uint8_t op = *p++;
    len--;

    // DEBUG_PRINT("_GATT_Data_Callback %s %d op 0x%02x\n", bdaddr_to_string(addr), buf->len, op);

    GATTHandle* handle = _GATT_GetHandleFromBDA(addr);
    if (!handle) {
        DEBUG_PRINT("GATT: No handle for data\n");
        // TODO check this actually works? There was a weird recursive spiral?
        L2CA_RemoveFixedChnl(L2CAP_ATT_CID, addr);
        GKI_freebuf(buf);
        return;
    }

    switch (op) {
    case ATT_ERROR_RSP:
        _GATT_Handle_Error_Response(handle, p, len);
        break;
    case ATT_EXCHANGE_MTU_RSP:
        _GATT_Handle_ExchangeMTU_Response(handle, p, len);
        break;
    case ATT_FIND_INFORMATION_RSP:
        _GATT_Handle_FindInformation_Response(handle, p, len);
        break;
    case ATT_READ_BY_TYPE_RSP:
        _GATT_Handle_ReadByType_Response(handle, p, len);
        break;
    case ATT_READ_RSP:
    case ATT_READ_BLOB_RSP:
        _GATT_Handle_Read_Response(handle, p, len);
        break;
    case ATT_READ_BY_GROUP_TYPE_RSP:
        _GATT_Handle_ReadByGroupType_Response(handle, p, len);
        break;
    case ATT_WRITE_RSP:
        _GATT_Handle_Write_Response(handle, p, len);
        break;
    case ATT_HANDLE_VALUE_NTF:
        _GATT_Handle_HandleValue_Notification(handle, p, len);
        break;
    default:
        DEBUG_PRINT("GATT: Unhandled ATT response 0x%x\n", op);
        break;
    }

    GKI_freebuf(buf);
}

static void _GATT_Register()
{
    if (gatt_is_registered) {
        return;
    }

    gatt_is_registered = 1;

    tL2CAP_FIXED_CHNL_REG fixed_reg;
    fixed_reg.fixed_chnl_opts.mode = L2CAP_FCR_BASIC_MODE;
    fixed_reg.fixed_chnl_opts.max_transmit = 0xFF;
    fixed_reg.fixed_chnl_opts.rtrans_tout = 2000;
    fixed_reg.fixed_chnl_opts.mon_tout = 12000;
    fixed_reg.fixed_chnl_opts.mps = 670;
    fixed_reg.fixed_chnl_opts.tx_win_sz = 1;
    fixed_reg.pL2CA_FixedConn_Cb = _GATT_Connect_Callback;
    fixed_reg.pL2CA_FixedData_Cb = _GATT_Data_Callback;
    fixed_reg.default_idle_tout = 0xffff;
    L2CA_RegisterFixedChannel(L2CAP_ATT_CID, &fixed_reg);
}

/*------------------------*/
/* Public facing GATT API */
/*------------------------*/

void GATT_Init(void)
{
    _GATT_Register();
}

void GATT_RegisterAutoConnectionHandler(GATTAutoConnectionCallback callback)
{
    for (int i = 0; i < GATT_MAX_CALLBACKS; i++) {
        if (auto_conn_callbacks[i] == NULL) {
            auto_conn_callbacks[i] = callback;
            break;
        }
    }
}

uint8_t GATT_Connect(BD_ADDR addr, GATTCallback callback)
{
    GATTHandle* handle = _GATT_GetHandleFromBDA(addr);
    if (handle) {
        DEBUG_PRINT("GATT: Error attempting to connect with active handle\n");
        return 0;
    }

    handle = _GATT_AllocateHandle(addr);
    if (!handle) {
        DEBUG_PRINT("GATT: Failed to allocate handle\n");
        return 0;
    }

    // Wait for connection
    handle->state = GATT_STATE_WAIT_CONNECT;
    handle->callback = callback;

    return L2CA_ConnectFixedChnl(L2CAP_ATT_CID, addr);
}

uint8_t GATT_CancelConnect(BD_ADDR addr)
{
    GATTHandle* handle = _GATT_GetHandleFromBDA(addr);
    if (!handle) {
        DEBUG_PRINT("GATT: Failed to find handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_WAIT_CONNECT) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    _GATT_FreeHandle(handle);
    return L2CA_CancelBleConnectReq(addr);
}

uint8_t GATT_Disconnect(GATTHandle* handle)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    // return L2CA_RemoveFixedChnl(L2CAP_ATT_CID, handle->addr);
    // TODO we just use btm to completely disconnect the device for now
    btm_remove_acl(handle->addr);
    return 1;
}

void GATT_SetHandleParam(GATTHandle* handle, void* param)
{
    handle->userParam = param;
}

void* GATT_GetHandleParam(GATTHandle* handle)
{
    return handle->userParam;
}

uint8_t GATT_DiscoverAllPrimaryServices(GATTHandle* handle)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_IDLE) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    // Read all services with primary group type
    handle->state = GATT_STATE_DISCOVER_SERVICES;

    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_GATT_PRIMARY_SERVICE;
    _GATT_SendRequest(handle, ATT_Build_ReadByGroupType_Request(0x0001, 0xffff, uuid));
    return 1;
}

uint8_t GATT_DiscoverChars(GATTHandle* handle, uint16_t startingHandle, uint16_t endingHandle)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_IDLE) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    handle->state = GATT_STATE_DISCOVER_CHARS;
    handle->discoverChars.endingHandle = endingHandle;

    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_GATT_CHARACTERISTIC;
    _GATT_SendRequest(handle, ATT_Build_ReadByType_Request(startingHandle, endingHandle, uuid));
    return 1;
}

uint8_t GATT_DiscoverCharDesc(GATTHandle* handle, uint16_t startingHandle, uint16_t endingHandle)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_IDLE) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    handle->state = GATT_STATE_DISCOVER_CHAR_DESC;
    handle->discoverCharDesc.endingHandle = endingHandle;

    _GATT_SendRequest(handle, ATT_Build_FindInformation_Request(startingHandle, endingHandle));
    return 1;
}

uint8_t GATT_Read(GATTHandle* handle, uint16_t characteristicHandle)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_IDLE) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    handle->state = GATT_STATE_READ;
    handle->read.handle = characteristicHandle;
    handle->read.offset = 0;
    handle->read.buffer = NULL;

    _GATT_SendRequest(handle, ATT_Build_Read_Request(characteristicHandle));
    return 1;
}

uint8_t GATT_ReadByUUID(GATTHandle* handle, tBT_UUID characteristicUUID)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_IDLE) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    handle->state = GATT_STATE_READ;
    handle->read.handle = 0;
    handle->read.offset = 0;
    handle->read.buffer = NULL;

    _GATT_SendRequest(handle, ATT_Build_ReadByType_Request(0x0001, 0xffff, characteristicUUID));
    return 1;
}

uint8_t GATT_Write(GATTHandle* handle, uint16_t characteristicHandle, const void* data, uint16_t len)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    if (handle->state != GATT_STATE_IDLE) {
        DEBUG_PRINT("GATT: Invalid state\n");
        return 0;
    }

    handle->state = GATT_STATE_WRITE;

    _GATT_SendRequest(handle, ATT_Build_Write_Request(characteristicHandle, data, len));
    return 1;
}

uint8_t GATT_WriteNoRsp(GATTHandle* handle, uint16_t characteristicHandle, const void* data, uint16_t len)
{
    if (!_GATT_CheckHandle(handle)) {
        DEBUG_PRINT("GATT: Invalid handle\n");
        return 0;
    }

    // No need to mess with state here, we don't get a response for the command
    _GATT_SendRequest(handle, ATT_Build_Write_Command(characteristicHandle, data, len));
    return 1;
}
