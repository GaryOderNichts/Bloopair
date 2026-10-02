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
#include "wud.h"
#include "bt_api.h"
#include "device_info.h"
#include "stack/btm.h"
#include "bta/bta_api.h"

void (*real_write_link_key)(void* p1) = (void*) 0x11f41b48;
void write_link_key_hook(void* p1)
{
    const uint8_t* addr = (const uint8_t*) p1;

    // Don't write link keys for our own devices,
    // they're stored on the SD Card instead
    DeviceInfo* info = DeviceInfo_Get(addr);
    if (info && (info->magic != MAGIC_OFFICIAL && info->magic != MAGIC_EMPTY)) {
        gWBC.linkKeyState = 0;
        GKI_freebuf(p1);
        return;
    }

    real_write_link_key(p1);
}

static void _RemoveDev_Callback(void* p_param)
{
    DeviceInfo* info = (DeviceInfo*) p_param;

    // Remove device from whitelist
    if (btm_ble_find_dev_in_whitelist(info->address)) {
        BTM_BleUpdateBgConnDev(0, info->address);
    }

    // Disconnect
    if (BTM_IsAclConnectionUp(info->address)) {
        btm_remove_acl(info->address);
    }

    // Wipe from BTM
    deleteDevice(info->address);
}

void WUDiRemoveDevice_hook(uint8_t* addr)
{
    DEBUG_PRINT("WUDiRemoveDevice_hook %s\n", bdaddr_to_string(addr));

    WUDDevInfo* info = WUDiGetDevInfo(addr);
    if (!info) {
        return;
    }

    IOS_WaitSemaphore(gWUDContext.semaphoreHandle, 0);

    if (memcmp(info->name, "Nintendo RVL-CNT", 0x10) == 0 ||
        (memcmp(info->name, "Nintendo RVL-WBC", 0x10) == 0 && WUDIsLinkedWBC())) {

        // Since our devices bypass BTA_Hh, remove them ourselves here
        // (this is the only reason we're replacing this function)
        DeviceInfo* dinfo = DeviceInfo_Get(info->address);
        if (dinfo && (dinfo->magic == MAGIC_SWITCH2 || dinfo->magic == MAGIC_BLOOPAIR_BLE)) {
            bta_dmexecutecallback(_RemoveDev_Callback, dinfo);
        } else {
            BTA_HhRemoveDev(info->handle);

            deleteDevice(info->address);
        }
    } else {
        deleteDevice(info->address);
    }

    if (info->unk0x5b == 2 || info->unk0x5b == 0) {
        gWBC.devNums = gWBC.devNums - 1;
        bt_rm_ppc_response_DevNum(gWBC.devNums);
    }

    memset(info, 0, sizeof(WUDDevInfo));

    IOS_SignalSemaphore(gWUDContext.semaphoreHandle);
}

// This is the init sequence usually done in bta_hh_event/WUDiHidHostEventCallback
uint8_t WUD_ConnectDevice(uint8_t handle, const uint8_t* addr, uint8_t is_error)
{
    WUDDevInfo* devInfo;

    if (is_error) {
        if (gWBC.syncState == WUD_SYNC_STATE_INIT) {
            // TODO this is technically only done if not auth failure
            if (WUDiGetDevInfo(addr) /* && !auth failure */) {
                devInfo = WUDiGetDevInfo(addr);
                WUDiMoveBottomStdDevInfoPtr(devInfo);
                WUDiRemoveDevice(addr);
                gWBC.linkNums--;
            }
        } else {
            devInfo = WUDiGetDiscoverDevice();
            if (memcmp(addr, devInfo->address, sizeof(BD_ADDR)) == 0 &&
                devInfo->status == WUD_DEVICE_STATUS_REGISTERING) {

                // TODO this is technically only done on auth failure
                if (WUDiGetDevInfo(addr) /* && auth failure */) {
                    WUDiRemoveDevice(addr);
                    gWBC.linkNums--;
                }

                gWBC.syncState = WUD_SYNC_STATE_ERROR;
                IOS_SendMessage(gWUDContext.queueHandle, WUD_MESSAGE_SYNC_ERROR, 1);
            }
        }

        return 1;
    }

    devInfo = WUDiGetDiscoverDevice();
    if (memcmp(devInfo->address, addr, sizeof(BD_ADDR)) != 0) {
        devInfo = WUDiGetDevInfo(addr);
    }
    if (!devInfo) {
        DEBUG_PRINT("Unknown connecting device\n");
        return 0;
    }

    if (devInfo->status == WUD_DEVICE_STATUS_REGISTERING) {
        gWBC.syncState = WUD_SYNC_STATE_COMPLETE;
    } else if (devInfo->status == WUD_DEVICE_STATUS_AUTH_COMPLETE) {
        gWBC.syncState = WUD_SYNC_STATE_REGISTER_DEVICE;
    }

    devInfo->status = WUD_DEVICE_STATUS_CONNECTED;
    devInfo->handle = handle;
    gWBC.connNums++;

    // Not sure why we're doing this again
    devInfo = WUDiGetDevInfo(addr);
    if (!devInfo) {
        devInfo = WUDiGetDiscoverDevice();
    }

    WUDiSetDevAddrForHandle(handle, devInfo->address);
    WUDiSetQueueSizeForHandle(handle, 0);

    // This is usually skipped for balance board, but there are no BLE balance boards :p
    WUDiMoveTopStdDevInfoPtr(devInfo);

    if (gWBC.maxDevices < gWBC.connNums) {
        return 0;
    }

    // Notify PPC side of connected device
    bt_rm_ppc_response_hid_event(BTA_HH_OPEN_EVT, devInfo, 1, gWBC.connNums);

    if (gWBC.syncState == WUD_SYNC_STATE_REGISTER_DEVICE) {
        // Tell WUD to register this device
        IOS_SendMessage(gWUDContext.queueHandle, WUD_MESSAGE_SYNC_REGISTER_DEVICE, 1);
    } else if (gWBC.syncState == WUD_SYNC_STATE_COMPLETE) {
        gWBC.syncState = WUD_SYNC_STATE_STORED_DEV_INFO_TO_NAND;
        IOS_SendMessage(gWUDContext.queueHandle, WUD_MESSAGE_SYNC_COMPLETE, 1);
    }

    return 1;
}

// This is the deinit sequence usually done in bta_hh_event
void WUD_DisconnectDevice(uint8_t handle, const uint8_t* addr)
{
    gWBC.connNums--;

    // This is usually skipped for balance board, but there are no BLE balance boards :p
    WUDDevInfo* devInfo = WUDiGetDevInfo(addr);
    if (devInfo) {
        WUDiMoveTopOfDisconnectedStdDevice(devInfo);
    }

    WUDiSetDevAddrForHandle(handle, NULL);
    WUDiSetQueueSizeForHandle(handle, 0);

    // Notify PPC side of disconnected device
    bt_rm_ppc_response_hid_event(BTA_HH_CLOSE_EVT, devInfo, 0, gWBC.connNums);
}
