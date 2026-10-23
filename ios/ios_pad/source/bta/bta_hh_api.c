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
#include "bt_api.h"
#include "device_info.h"
#include "bleext/hid_ble.h"
#include "bleext/hid_switch2.h"
#include "bta/bta_api.h"

static void _HID_BLE_Open_Callback(void* p_param)
{
    uint8_t* addr = (uint8_t*) p_param;

    DeviceInfo* info = DeviceInfo_Get(addr);
    if (info && info->magic == MAGIC_SWITCH2) {
        HID_SW2_Open(addr);
    } else {
        HID_BLE_Open(addr);
    }

    GKI_freebuf(addr);
}


void (*const real_BTA_HhOpen)(BD_ADDR dev_bda, uint8_t mode, uint8_t sec_mask) = (void*) DEFINE_REAL(0x11f077f8, 0xe92d40f0);
void BTA_HhOpen_hook(BD_ADDR dev_bda, uint8_t mode, uint8_t sec_mask)
{
    uint8_t devType;
    uint8_t addrType;
    BTM_ReadDevInfo(dev_bda, &devType, &addrType);

    // If this is a BLE device, pass on to our HID over GATT implementation
    if (devType == BT_DEVICE_TYPE_BLE) {
        // Let's be safe and dispatch this from the btm context, like the hid host would
        void* addr = GKI_getbuf(sizeof(BD_ADDR));
        memcpy(addr, dev_bda, sizeof(BD_ADDR));
        bta_dmexecutecallback(_HID_BLE_Open_Callback, addr);
        return;
    }

    real_BTA_HhOpen(dev_bda, mode, sec_mask);
}

static void _HID_BLE_Close_Callback(void* p_param)
{
    uint8_t dev_handle = (uint8_t) (uint32_t) p_param;

    if (HID_SW2_IsActiveHandle(dev_handle)) {
        HID_SW2_Close(dev_handle);
    } else {
        HID_BLE_Close(dev_handle);
    }
}

void (*const real_BTA_HhClose)(uint8_t dev_handle) = (void*) DEFINE_REAL(0x11f07638, 0xe92d4030);
void BTA_HhClose_hook(uint8_t dev_handle)
{
    // If this is a BLE handle, pass on to our impl
    if (HID_BLE_IsActiveHandle(dev_handle) || HID_SW2_IsActiveHandle(dev_handle)) {
        // Let's be safe and dispatch this from the btm context, like the hid host would
        bta_dmexecutecallback(_HID_BLE_Close_Callback, (void*) (uint32_t) dev_handle);
        return;
    }

    real_BTA_HhClose(dev_handle);
}

void (*const real_BTA_HhAddDev)(uint8_t* bda, uint16_t attr_mask, uint8_t sub_class, uint8_t app_id, uint32_t dl_len, void *dsc_list) = (void*) DEFINE_REAL(0x11f0767c, 0xe92d4ff0);
void BTA_HhAddDev_hook(uint8_t* bda, uint16_t attr_mask, uint8_t sub_class, uint8_t app_id, uint32_t dl_len, void *dsc_list)
{
    // If this is our BLE device don't register with BTA_Hh
    DeviceInfo* info = DeviceInfo_Get(bda);
    if (info && (info->magic == MAGIC_SWITCH2 || info->magic == MAGIC_BLOOPAIR_BLE)) {
        return;
    }

    real_BTA_HhAddDev(bda, attr_mask, sub_class, app_id, dl_len, dsc_list);
}

void (*const real_BTA_HhRemoveDev)(uint8_t dev_handle) = (void*) DEFINE_REAL(0x11f075ec, 0xe92d4030);
void BTA_HhRemoveDev_hook(uint8_t dev_handle)
{
    // If this is a BLE handle, pass on to our impl
    if (HID_BLE_IsActiveHandle(dev_handle) || HID_SW2_IsActiveHandle(dev_handle)) {
        // Eh we ended up hooking WUDiRemoveDev instead, so nothing to do here
        return;
    }

    real_BTA_HhRemoveDev(dev_handle);
}
