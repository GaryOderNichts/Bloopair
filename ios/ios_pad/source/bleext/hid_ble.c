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
#include "hid_ble.h"
#include "device_info.h"
#include "gatt.h"
#include "main.h"
#include "smp.h"
#include "stack/btm.h"
#include "stack/hid.h"
#include "stack/l2c.h"
#include "uuids.h"

#define HID_BLE_INACTIVITY_TOUT (5 * 1000) // If a device doesn't finish initialization within 5 seconds, we drop it
#define HID_BLE_MAX_REPORTS     8
#define HID_BLE_MAX_REPORT_LEN  128

typedef enum {
    HID_BLE_STATE_INIT,
    HID_BLE_STATE_CONNECT,
    HID_BLE_STATE_WAIT_ENCRYPT,
    HID_BLE_STATE_PNP_ID,
    HID_BLE_STATE_DISCOVER_SERVICES,
    HID_BLE_STATE_DISCOVER_CHARACTERISTICS,
    HID_BLE_STATE_READ_REPORT_MAP,
    HID_BLE_STATE_DISCOVER_DESCRIPTORS,
    HID_BLE_STATE_DISCOVER_REPORT_REFERENCES,
    HID_BLE_STATE_ENABLE_NOTIFICATIONS,
    HID_BLE_STATE_INITIAL_READ,
    HID_BLE_STATE_IDLE,
} HIDBLEState;

typedef struct {
    uint8_t properties;
    uint16_t valueHandle;
    uint16_t endHandle;

    uint16_t configHandle;
    uint16_t referenceHandle;

    uint8_t reportId;
    uint8_t reportType;
} HIDBLEReport;

typedef struct {
    uint8_t inUse;
    HIDBLEState state;
    GATTHandle* gattHandle;
    BD_ADDR addr;
    uint8_t devHandle;
    uint8_t connectFailure;
    TIMER_LIST_ENT timer;

    uint16_t vendorId;
    uint16_t productId;

    uint16_t hidStartingHandle;
    uint16_t hidEndingHandle;

    uint16_t reportMapHandle;

    HIDBLEReport hidReports[HID_BLE_MAX_REPORTS];
    uint8_t hidNumReports;

    uint8_t currentReportToProcess;
} HIDBLEHandle;

static HIDBLECallback hid_callback;
static HIDBLEHandle hid_handles[BTA_HH_MAX_KNOWN];

/*--------------------*/
/* Internal functions */
/*--------------------*/

static HIDBLEHandle* _HID_BLE_AllocateHandle(void)
{
    // Find unused handle
    HIDBLEHandle* handle = NULL;
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (!hid_handles[i].inUse) {
            handle = &hid_handles[i];
            break;
        }
    }

    if (handle) {
        memset(handle, 0, sizeof(HIDBLEHandle));
        handle->inUse = 1;
    }

    return handle;
}

static inline HIDBLEHandle* _HID_BLE_GetHandleFromGATTHandle(GATTHandle* gattHandle)
{
    return (HIDBLEHandle*) GATT_GetHandleParam(gattHandle);
}

static HIDBLEHandle* _HID_BLE_GetHandleFromHidHostHandle(uint8_t devHandle)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        HIDBLEHandle* handle = &hid_handles[i];
        if (handle->inUse && handle->devHandle == devHandle) {
            return handle;
        }
    }

    return NULL;
}

static HIDBLEHandle* _HID_BLE_GetHandleFromBDA(const BD_ADDR bda)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        HIDBLEHandle* handle = &hid_handles[i];
        if (handle->inUse && memcmp(handle->addr, bda, sizeof(handle->addr)) == 0) {
            return handle;
        }
    }

    return NULL;
}

static void _HID_BLE_FreeHandle(HIDBLEHandle* handle)
{
    bta_sys_stop_timer(&handle->timer);
    handle->inUse = 0;
}

static HIDBLEReport* _HID_BLE_GetReport(HIDBLEHandle* handle, uint16_t valueHandle)
{
    for (int i = 0; i < handle->hidNumReports; i++) {
        HIDBLEReport* report = &handle->hidReports[i];
        if (report->valueHandle == valueHandle) {
            return report;
        }
    }

    return NULL;
}

static HIDBLEReport* _HID_BLE_GetReportFromId(HIDBLEHandle* handle, uint8_t reportId, uint8_t reportType)
{
    for (int i = 0; i < handle->hidNumReports; i++) {
        HIDBLEReport* report = &handle->hidReports[i];
        if (report->reportId == reportId && report->reportType == reportType) {
            return report;
        }
    }

    return NULL;
}

static void _HID_BLE_ConnectionComplete(HIDBLEHandle* handle)
{
    // Stop inactivity timer, now that we're done
    bta_sys_stop_timer(&handle->timer);

    // Update conn params to improve latency
    L2CA_UpdateBleConnParams(handle->addr, 6, 8, 0, 100);

    // Restart background connections here now, since we nop'ed out the immediate restart in btm_acl_created
    btm_ble_resume_bg_conn(NULL, 1);

    // Add vendor and product id to info store
    // TODO this could be moved to upper layer
    DeviceInfo* info = DeviceInfo_GetOrAllocate(handle->addr);
    if (info && info->magic != MAGIC_SWITCH2) {
        info->magic = MAGIC_BLOOPAIR_BLE;
        info->vendor_id = handle->vendorId;
        info->product_id = handle->productId;
    }

    // Notify upper layer
    HIDBLEEventData eventData;
    eventData.connect.status = HID_BLE_STATUS_SUCCESS;
    eventData.connect.handle = handle->devHandle;
    memcpy(eventData.connect.addr, handle->addr, sizeof(BD_ADDR));
    hid_callback(HID_BLE_EVENT_CONNECT, &eventData);
}

static void _HID_BLE_ConnectFailure(HIDBLEHandle* handle)
{
    // Notify upper layer of failed connect
    HIDBLEEventData eventData;
    eventData.connect.status = HID_BLE_STATUS_FAILURE;
    eventData.connect.handle = handle->devHandle;
    memcpy(eventData.connect.addr, handle->addr, sizeof(BD_ADDR));
    hid_callback(HID_BLE_EVENT_CONNECT, &eventData);

    // Disconnect from device
    handle->connectFailure = 1;
    GATT_Disconnect(handle->gattHandle);
}

static void _HID_BLE_Timer_Cback(TIMER_LIST_ENT* p_tle)
{
    HIDBLEHandle* handle = (HIDBLEHandle*) p_tle->param;

    DEBUG_PRINT("HID: Connection timed out: %s\n", bdaddr_to_string(handle->addr));

    _HID_BLE_ConnectFailure(handle);
}

static void _HID_BLE_HID_Data(HIDBLEHandle* handle, uint16_t valueHandle, const void* value, uint16_t length)
{
    uint8_t buf[HID_BLE_MAX_REPORT_LEN];

    // Lookup report information
    HIDBLEReport* report = _HID_BLE_GetReport(handle, valueHandle);
    if (!report) {
        DEBUG_PRINT("HID: Notification for unknown report\n");
        return;
    }

    if (length > HID_BLE_MAX_REPORT_LEN - 1) {
        DEBUG_PRINT("HID: Notification too long\n");
        return;
    }

    // Prepend report id to buffer
    buf[0] = report->reportId;
    memcpy(buf + 1, value, length);

    // Call callback
    bta_hh_co_data(handle->devHandle, buf, length + 1, 0, 4, 0, NULL, 3);
}

/*-----------------------*/
/* SMP callback handlers */
/*-----------------------*/

static void _HID_BLE_SMPCallback(BD_ADDR addr, SMPEvent event, SMPBondingData* bondingData, void* userPointer)
{
    HIDBLEHandle* handle = _HID_BLE_GetHandleFromBDA(addr);
    if (!handle) {
        DEBUG_PRINT("HID: Cannot find handle for SMP callback\n");
        btm_remove_acl(addr);
        return;
    }

    switch (event) {
    case SMP_EVENT_ENCRYPT:
        if (handle->state == HID_BLE_STATE_WAIT_ENCRYPT) {
            // Start GATT HID procedure, now that the link is encrypted
            handle->state = HID_BLE_STATE_PNP_ID;

            tBT_UUID uuid;
            uuid.len = LEN_UUID_16;
            uuid.uu.uuid16 = UUID_PNP_ID;
            GATT_ReadByUUID(handle->gattHandle, uuid);
        }
        break;
    case SMP_EVENT_BOND: {
        // Updating existing or add new device
        DeviceInfo* info = DeviceInfo_GetOrAllocate(addr);
        if (!info) {
            return;
        }

        tBTM_SEC_DEV_REC* dev = btm_find_dev(handle->addr);
        if (!dev) {
            DEBUG_PRINT("HID: unknown dev\n");
            return;
        }

        // Add bonding information to device
        info->magic = MAGIC_BLOOPAIR_BLE;
        info->flush = 1;
        info->ble.address_type = dev->ble.ble_addr_type;
        memcpy(info->ble.ltk, bondingData->ltk, sizeof(bondingData->ltk));
        memcpy(info->ble.rand, bondingData->rand, sizeof(bondingData->rand));
        info->ble.ediv = bondingData->ediv;

        DeviceInfo_AddToBgConn(info);
        break;
    }
    case SMP_EVENT_FAILURE:
        _HID_BLE_ConnectFailure(handle);
        break;

    default:
        break;
    }
}

/*------------------------*/
/* GATT callback handlers */
/*------------------------*/

static void _HID_BLE_Handle_Connect_Event(HIDBLEHandle* handle, GATTHandle* gattHandle, GATTConnectEventData* data)
{
    DEBUG_PRINT("HID: Device %s connected!\n", bdaddr_to_string(data->addr));

    if (handle) {
        DEBUG_PRINT("HID: Handle already exists??\n");
        GATT_Disconnect(gattHandle);
        return;
    }

    uint8_t bgconn = 0;
    handle = _HID_BLE_GetHandleFromBDA(data->addr);
    if (!handle) {
        // If we're here we got a connection without initiating it
        // most likely a background connection
        handle = _HID_BLE_AllocateHandle();
        bgconn = 1;
    }

    if (!handle) {
        DEBUG_PRINT("HID: Cannot allocate handle\n");
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

    // Start inactivity timer
    handle->timer.param = (uint32_t) handle;
    handle->timer.p_cback = (TIMER_CBACK*) _HID_BLE_Timer_Cback;
    bta_sys_start_timer(&handle->timer, 0, HID_BLE_INACTIVITY_TOUT);

    handle->state = HID_BLE_STATE_WAIT_ENCRYPT;

    // If this was not a background connection, we need to start pairing
    if (!bgconn) {
        SMP_StartPairing(handle->addr, _HID_BLE_SMPCallback, NULL);
        return;
    }

    // For background connection, get bonding information for device and start encryption
    DeviceInfo* info = DeviceInfo_Get(handle->addr);
    if (!info || info->magic != MAGIC_BLOOPAIR_BLE) {
        DEBUG_PRINT("HID: Failed to find device info for auto connection\n");
        return;
    }

    // Copy BLE information from device info
    SMPBondingData bonding;
    memcpy(bonding.ltk, info->ble.ltk, sizeof(bonding.ltk));
    memcpy(bonding.rand, info->ble.rand, sizeof(bonding.rand));
    bonding.ediv = info->ble.ediv;

    // Start encryption
    SMP_StartEncrypt(handle->addr, _HID_BLE_SMPCallback, &bonding, NULL);
}

static void _HID_BLE_Handle_Disconnect_Event(HIDBLEHandle* handle, GATTDisconnectEventData* data)
{
    // If connect failed, don't bother sending a disconnect, a connect with failure was already sent
    if (!handle->connectFailure) {
        // If we disconnected before reaching idle state without error, treat as connect failure
        if (handle->state < HID_BLE_STATE_INITIAL_READ) {
            HIDBLEEventData eventData;
            eventData.connect.status = HID_BLE_STATUS_FAILURE;
            eventData.connect.handle = BTA_HH_INVALID_HANDLE;
            memcpy(eventData.connect.addr, handle->addr, sizeof(BD_ADDR));
            hid_callback(HID_BLE_EVENT_CONNECT, &eventData);
        } else {
            HIDBLEEventData eventData;
            memcpy(eventData.disconnect.addr, handle->addr, sizeof(BD_ADDR));
            eventData.disconnect.handle = handle->devHandle;
            hid_callback(HID_BLE_EVENT_DISCONNECT, &eventData);
        }
    }

    HID_HostRemoveDevNoClose(handle->devHandle);
    _HID_BLE_FreeHandle(handle);
}

static void _HID_BLE_Handle_DiscoverServicesResult_Event(HIDBLEHandle* handle,
                                                         GATTDiscoverServicesResultEventData* data)
{
    // Look for the HID service
    if (data->uuid.len == LEN_UUID_16 && data->uuid.uu.uuid16 == UUID_HUMAN_INTERFACE_DEVICE) {
        DEBUG_PRINT("HID: Found HID service %x - %x\n", data->startingHandle, data->endingHandle);

        handle->hidStartingHandle = data->startingHandle;
        handle->hidEndingHandle = data->endingHandle;

        // We found the HID service, can stop now
        data->stopDiscovery = 1;
    }
}

static void _HID_BLE_Handle_DiscoverServicesComplete_Event(HIDBLEHandle* handle,
                                                           GATTDiscoverServicesCompleteEventData* data)
{
    // Make sure success and HID service was actually found
    if (data->status != GATT_STATUS_SUCCESS || handle->hidEndingHandle == 0) {
        DEBUG_PRINT("HID: Did not find HID service\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    // Continue with discovering HID characteristics
    handle->state = HID_BLE_STATE_DISCOVER_CHARACTERISTICS;
    GATT_DiscoverChars(handle->gattHandle, handle->hidStartingHandle, handle->hidEndingHandle);
}

static void _HID_BLE_Handle_DiscoverCharsResult_Event(HIDBLEHandle* handle, GATTDiscoverCharsResultEventData* data)
{
    // Set end of last report
    if (handle->hidNumReports > 0) {
        HIDBLEReport* report = &handle->hidReports[handle->hidNumReports - 1];
        if (report->endHandle == handle->hidEndingHandle) {
            report->endHandle = data->handle - 1;
        }
    }

    // Keep track of HID reports
    if (data->uuid.len == LEN_UUID_16) {
        if (data->uuid.uu.uuid16 == UUID_REPORT) {
            DEBUG_PRINT("Report %d: properties %x handle %x\n", handle->hidNumReports, data->properties,
                        data->valueHandle);

            if (handle->hidNumReports < HID_BLE_MAX_REPORTS) {
                HIDBLEReport* report = &handle->hidReports[handle->hidNumReports];
                report->properties = data->properties;
                report->valueHandle = data->valueHandle;
                report->endHandle = handle->hidEndingHandle;
                handle->hidNumReports++;
            }
        } else if (data->uuid.uu.uuid16 == UUID_REPORT_MAP) {
            DEBUG_PRINT("Report map: properties %x handle %x\n", data->properties, data->valueHandle);

            handle->reportMapHandle = data->valueHandle;
        }
    }
}

static void _HID_BLE_Handle_DiscoverCharsComplete_Event(HIDBLEHandle* handle, GATTDiscoverCharsCompleteEventData* data)
{
    if (data->status != GATT_STATUS_SUCCESS || handle->hidNumReports == 0) {
        DEBUG_PRINT("HID: Did not find HID reports\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    // Reading the report map is necessary for some controllers
    // e.g. Xbox Controller doesn't leave pairing mode until the report map is read
    handle->state = HID_BLE_STATE_READ_REPORT_MAP;
    GATT_Read(handle->gattHandle, handle->reportMapHandle);
}

static void _HID_BLE_Handle_DiscoverCharDescResult_Event(HIDBLEHandle* handle,
                                                         GATTDiscoverCharDescResultEventData* data)
{
    HIDBLEReport* report = &handle->hidReports[handle->currentReportToProcess];

    if (data->uuid.len == LEN_UUID_16) {
        switch (data->uuid.uu.uuid16) {
        case UUID_CLIENT_CHARACTERISTIC_CONFIGURATION:
            DEBUG_PRINT("UUID_CLIENT_CHARACTERISTIC_CONFIGURATION: %x\n", data->handle);
            report->configHandle = data->handle;
            break;
        case UUID_REPORT_REFERENCE:
            DEBUG_PRINT("UUID_REPORT_REFERENCE: %x\n", data->handle);
            report->referenceHandle = data->handle;
            break;
        default:
            break;
        }
    }
}

static void _HID_BLE_Handle_DiscoverCharDescComplete_Event(HIDBLEHandle* handle,
                                                           GATTDiscoverCharDescCompleteEventData* data)
{
    // TODO should this error out?
    if (data->status != GATT_STATUS_SUCCESS) {
        DEBUG_PRINT("HID: Failed to discover characteristic descriptors\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    // Discover the next report descriptors or move on with references
    if (handle->currentReportToProcess + 1 < handle->hidNumReports) {
        handle->currentReportToProcess++;

        HIDBLEReport* report = &handle->hidReports[handle->currentReportToProcess];
        GATT_DiscoverCharDesc(handle->gattHandle, report->valueHandle + 1, report->endHandle);
    } else {
        handle->state = HID_BLE_STATE_DISCOVER_REPORT_REFERENCES;
        handle->currentReportToProcess = 0;
        GATT_Read(handle->gattHandle, handle->hidReports[0].referenceHandle);
    }
}

static void _HID_BLE_Process_PNP_ID(HIDBLEHandle* handle, GATTStatus status, const uint8_t* p, uint16_t len)
{
    if (status != GATT_STATUS_SUCCESS || len != 7) {
        DEBUG_PRINT("HID: Invalid read\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    // uint8_t vendorIdSource = p[0];
    handle->vendorId = p[1] | (p[2] << 8);
    handle->productId = p[3] | (p[4] << 8);
    // uint16_t version = p[5] | (p[6] << 8);

    DEBUG_PRINT("HID: vid %x pid %x\n", handle->vendorId, handle->productId);

    // Continue with service discovery
    handle->state = HID_BLE_STATE_DISCOVER_SERVICES;
    GATT_DiscoverAllPrimaryServices(handle->gattHandle);
}

static void _HID_BLE_Process_ReportReference(HIDBLEHandle* handle, GATTStatus status, const uint8_t* p, uint16_t len)
{
    if (status != GATT_STATUS_SUCCESS || len != 2) {
        DEBUG_PRINT("HID: Invalid read\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    HIDBLEReport* report = &handle->hidReports[handle->currentReportToProcess];
    report->reportId = p[0];
    report->reportType = p[1];

    DEBUG_PRINT("Report %d: id %x type %x\n", handle->currentReportToProcess, report->reportId, report->reportType);

    // Read the next reference or move on with enabling notifications
    if (handle->currentReportToProcess + 1 < handle->hidNumReports) {
        handle->currentReportToProcess++;

        report = &handle->hidReports[handle->currentReportToProcess];
        GATT_Read(handle->gattHandle, report->referenceHandle);
    } else {
        handle->state = HID_BLE_STATE_ENABLE_NOTIFICATIONS;

        for (int i = 0; i < handle->hidNumReports; i++) {
            HIDBLEReport* report = &handle->hidReports[i];
            if (report->properties & GATT_CHAR_PROP_NOTIFY) {
                handle->currentReportToProcess = i;

                uint16_t value = bswap16(GATT_CHAR_CONF_NOTIFICATION); // Set notification bit
                GATT_Write(handle->gattHandle, report->configHandle, &value, sizeof(value));
                return;
            }
        }

        DEBUG_PRINT("HID: No notifiable characteristics found\n");
        GATT_Disconnect(handle->gattHandle);
    }
}

static void _HID_BLE_Process_ReportMap(HIDBLEHandle* handle, GATTStatus status, const uint8_t* p, uint16_t len)
{
    if (status != GATT_STATUS_SUCCESS) {
        DEBUG_PRINT("HID: Invalid read\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    // We don't do anything with the report map yet
    DEBUG_PRINT("Read report map len %d\n", len);
    dumpHex(p, len);

    // Discover descriptors for every report, starting at the first one
    handle->state = HID_BLE_STATE_DISCOVER_DESCRIPTORS;
    handle->currentReportToProcess = 0;
    GATT_DiscoverCharDesc(handle->gattHandle, handle->hidReports[0].valueHandle + 1, handle->hidReports[0].endHandle);
}

static void _HID_BLE_Process_InitialRead(HIDBLEHandle* handle, GATTStatus status, const uint8_t* p, uint16_t len)
{
    // Process data
    if (status == GATT_STATUS_SUCCESS && len > 0) {
        _HID_BLE_HID_Data(handle, handle->hidReports[handle->currentReportToProcess].valueHandle, p, len);
    }

    // Find more potential reports that can be read
    for (int i = handle->currentReportToProcess + 1; i < handle->hidNumReports; i++) {
        HIDBLEReport* report = &handle->hidReports[i];
        if (report->properties & GATT_CHAR_PROP_READ && report->reportType == HID_REPORT_TYPE_INPUT_REPORT) {
            handle->currentReportToProcess = i;

            GATT_Read(handle->gattHandle, report->valueHandle);
            return;
        }
    }

    handle->state = HID_BLE_STATE_IDLE;
}

static void _HID_BLE_Handle_Read_Event(HIDBLEHandle* handle, GATTReadEventData* data)
{
    if (handle->state == HID_BLE_STATE_PNP_ID) {
        _HID_BLE_Process_PNP_ID(handle, data->status, data->value, data->length);
    } else if (handle->state == HID_BLE_STATE_DISCOVER_REPORT_REFERENCES) {
        _HID_BLE_Process_ReportReference(handle, data->status, data->value, data->length);
    } else if (handle->state == HID_BLE_STATE_READ_REPORT_MAP) {
        _HID_BLE_Process_ReportMap(handle, data->status, data->value, data->length);
    } else if (handle->state == HID_BLE_STATE_INITIAL_READ) {
        _HID_BLE_Process_InitialRead(handle, data->status, data->value, data->length);
    }
}

static void _HID_BLE_Process_EnableNotif(HIDBLEHandle* handle, GATTStatus status)
{
    if (status != GATT_STATUS_SUCCESS) {
        DEBUG_PRINT("HID: Invalid write\n");
        _HID_BLE_ConnectFailure(handle);
        return;
    }

    // Find more potential reports that can be notified
    for (int i = handle->currentReportToProcess + 1; i < handle->hidNumReports; i++) {
        HIDBLEReport* report = &handle->hidReports[i];
        if (report->properties & GATT_CHAR_PROP_NOTIFY) {
            handle->currentReportToProcess = i;

            uint16_t value = bswap16(GATT_CHAR_CONF_NOTIFICATION); // Set notification bit
            GATT_Write(handle->gattHandle, report->configHandle, &value, sizeof(value));
            return;
        }
    }

    // We're done now, notify client
    _HID_BLE_ConnectionComplete(handle);

    // Start reading an initial report value from each report
    // This seems to work really well for the xbox controller, but unfortunately not
    // the stadia controller, where this just reads 0 sized reports?
    handle->state = HID_BLE_STATE_INITIAL_READ;

    for (int i = 0; i < handle->hidNumReports; i++) {
        HIDBLEReport* report = &handle->hidReports[i];
        if (report->properties & GATT_CHAR_PROP_READ && report->reportType == HID_REPORT_TYPE_INPUT_REPORT) {
            handle->currentReportToProcess = i;

            GATT_Read(handle->gattHandle, report->valueHandle);
            return;
        }
    }

    // No initial value to read, we're done
    handle->state = HID_BLE_STATE_IDLE;
}

static void _HID_BLE_Handle_Write_Event(HIDBLEHandle* handle, GATTWriteEventData* data)
{
    if (handle->state == HID_BLE_STATE_ENABLE_NOTIFICATIONS) {
        _HID_BLE_Process_EnableNotif(handle, data->status);
    }
}

static void _HID_BLE_Handle_Notification_Event(HIDBLEHandle* handle, GATTNotificationEventData* data)
{
    if (handle->state == HID_BLE_STATE_IDLE) {
        _HID_BLE_HID_Data(handle, data->handle, data->value, data->length);
    }
}

static void _HID_BLE_GATT_Callback(GATTHandle* gattHandle, GATTEvent event, GATTEventData* data)
{
    HIDBLEHandle* handle = _HID_BLE_GetHandleFromGATTHandle(gattHandle);
    if (event != GATT_EVENT_CONNECT && !handle) {
        DEBUG_PRINT("HID: No handle for event %d?\n", event);
        return;
    }

    switch (event) {
    case GATT_EVENT_CONNECT:
        _HID_BLE_Handle_Connect_Event(handle, gattHandle, &data->connect);
        break;
    case GATT_EVENT_DISCONNECT:
        _HID_BLE_Handle_Disconnect_Event(handle, &data->disconnect);
        break;
    case GATT_EVENT_DISCOVER_SERVICES_RESULT:
        _HID_BLE_Handle_DiscoverServicesResult_Event(handle, &data->servicesResult);
        break;
    case GATT_EVENT_DISCOVER_SERVICES_COMPLETE:
        _HID_BLE_Handle_DiscoverServicesComplete_Event(handle, &data->servicesComplete);
        break;
    case GATT_EVENT_DISCOVER_CHARS_RESULT:
        _HID_BLE_Handle_DiscoverCharsResult_Event(handle, &data->charsResult);
        break;
    case GATT_EVENT_DISCOVER_CHARS_COMPLETE:
        _HID_BLE_Handle_DiscoverCharsComplete_Event(handle, &data->charsComplete);
        break;
    case GATT_EVENT_DISCOVER_CHAR_DESC_RESULT:
        _HID_BLE_Handle_DiscoverCharDescResult_Event(handle, &data->charDescResult);
        break;
    case GATT_EVENT_DISCOVER_CHAR_DESC_COMPLETE:
        _HID_BLE_Handle_DiscoverCharDescComplete_Event(handle, &data->charDescComplete);
        break;
    case GATT_EVENT_READ:
        _HID_BLE_Handle_Read_Event(handle, &data->read);
        break;
    case GATT_EVENT_WRITE:
        _HID_BLE_Handle_Write_Event(handle, &data->write);
        break;
    case GATT_EVENT_NOTIFICATION:
        _HID_BLE_Handle_Notification_Event(handle, &data->notif);
        break;
    default:
        DEBUG_PRINT("HID: Unhandled GATT event\n");
        break;
    }
}

static GATTCallback _HID_BLE_GATT_AutoConnectionHandler(BD_ADDR addr)
{
    DeviceInfo* info = DeviceInfo_Get(addr);
    if (!info) {
        return NULL;
    }

    if (info->magic != MAGIC_BLOOPAIR_BLE) {
        return NULL;
    }

    return &_HID_BLE_GATT_Callback;
}

/*---------------------------*/
/* Public facing HID BLE API */
/*---------------------------*/

void HID_BLE_Init(HIDBLECallback callback)
{
    GATT_Init();
    SMP_Init();

    // Handle auto connections
    GATT_RegisterAutoConnectionHandler(&_HID_BLE_GATT_AutoConnectionHandler);

    hid_callback = callback;
}

uint8_t HID_BLE_Open(BD_ADDR addr)
{
    DEBUG_PRINT("HID_BLE_Open: %s\n", bdaddr_to_string(addr));

    HIDBLEHandle* handle = _HID_BLE_GetHandleFromBDA(addr);
    if (handle) {
        DEBUG_PRINT("HID: Error attempting to open with active handle\n");
        // TODO there isn't really a way to recover from this?
        // WUD may call this before the disconnect by WUDiRemoveDevice has fully completed
        return 0;
    }

    // Create a handle here already
    handle = _HID_BLE_AllocateHandle();
    if (!handle) {
        DEBUG_PRINT("HID: Cannot allocate handle\n");
        return 0;
    }

    handle->devHandle = BTA_HH_INVALID_HANDLE;
    handle->gattHandle = NULL;
    memcpy(handle->addr, addr, sizeof(handle->addr));

    handle->state = HID_BLE_STATE_CONNECT;
    return GATT_Connect(handle->addr, &_HID_BLE_GATT_Callback);
}

uint8_t HID_BLE_Close(uint8_t devHandle)
{
    HIDBLEHandle* handle = _HID_BLE_GetHandleFromHidHostHandle(devHandle);
    if (!handle) {
        return 0;
    }

    GATT_Disconnect(handle->gattHandle);
    return 1;
}

uint8_t HID_BLE_IsActiveHandle(uint8_t devHandle)
{
    return _HID_BLE_GetHandleFromHidHostHandle(devHandle) != NULL;
}

uint8_t HID_BLE_SendData(uint8_t devHandle, const void* data, uint16_t len)
{
    HIDBLEHandle* handle = _HID_BLE_GetHandleFromHidHostHandle(devHandle);
    if (!handle) {
        return 0;
    }

    const uint8_t* reportData = (const uint8_t*) data;
    HIDBLEReport* report = _HID_BLE_GetReportFromId(handle, reportData[0], HID_REPORT_TYPE_OUTPUT_REPORT);
    if (!report) {
        DEBUG_PRINT("HID_BLE_SendData: Unknown report %x\n", reportData[0]);
        return 0;
    }

    return GATT_WriteNoRsp(handle->gattHandle, report->valueHandle, reportData + 1, len - 1);
}
