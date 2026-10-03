/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include "bta_api.h"
#include "device_info.h"

void BTA_DmConfirm(uint8_t* bd_addr, uint8_t accept)
{
    tBTA_DM_API_CONFIRM    *p_msg;

    if ((p_msg = (tBTA_DM_API_CONFIRM *) GKI_getbuf(sizeof(tBTA_DM_API_CONFIRM))) != NULL)
    {
        p_msg->hdr.event = 0x114; // BTA_DM_API_CONFIRM_EVT
        bdcpy(p_msg->bd_addr, bd_addr);
        p_msg->accept = accept;
        bta_sys_sendmsg(p_msg);
    }
}

void BTA_DmAddBleDevice(BD_ADDR bd_addr, uint8_t addr_type, uint8_t dev_type)
{
    tBTA_DM_API_ADD_BLE_DEVICE *p_msg;

    if ((p_msg = (tBTA_DM_API_ADD_BLE_DEVICE *) GKI_getbuf(sizeof(tBTA_DM_API_ADD_BLE_DEVICE))) != NULL)
    {
        memset (p_msg, 0, sizeof(tBTA_DM_API_ADD_BLE_DEVICE));

        p_msg->hdr.event = 0x118; //BTA_DM_API_ADD_BLEDEVICE_EVT;
        bdcpy(p_msg->bd_addr, bd_addr);
        p_msg->addr_type = addr_type;
        p_msg->dev_type = dev_type;

        bta_sys_sendmsg(p_msg);
    }
}

void BTA_DmBleSetBgConnType(uint8_t bg_conn_type, tBTA_DM_BLE_SEL_CBACK *p_select_cback)
{
    tBTA_DM_API_BLE_SET_BG_CONN_TYPE    *p_msg;

    if ((p_msg = (tBTA_DM_API_BLE_SET_BG_CONN_TYPE *) GKI_getbuf(sizeof(tBTA_DM_API_BLE_SET_BG_CONN_TYPE))) != NULL)
    {
        memset(p_msg, 0, sizeof(tBTA_DM_API_BLE_SET_BG_CONN_TYPE));

        p_msg->hdr.event        = 0x11B; // BTA_DM_API_BLE_SET_BG_CONN_TYPE;
        p_msg->bg_conn_type     = bg_conn_type;
        p_msg->p_select_cback   = p_select_cback;

        bta_sys_sendmsg(p_msg);
    }
}

void (*const real_BTA_DmAddDevice)(BD_ADDR bd_addr, DEV_CLASS dev_class, LINK_KEY link_key,
                                   uint32_t trusted_mask, uint8_t is_trusted,
                                   uint8_t key_type, uint8_t io_cap) = (void*) DEFINE_REAL(0x11f04d8c, 0xe92d4ff0);
void BTA_DmAddDevice_hook(BD_ADDR bd_addr, DEV_CLASS dev_class, LINK_KEY link_key,
                          uint32_t trusted_mask, uint8_t is_trusted,
                          uint8_t key_type, uint8_t io_cap)
{
    // If this is our BLE device use AddBleDevice instead
    DeviceInfo* info = DeviceInfo_Get(bd_addr);
    if (info) {
        if (info->magic == MAGIC_SWITCH2) {
            DEBUG_PRINT("BTA_DmAddDevice_hook: %s\n", bdaddr_to_string(info->address));
            BTA_DmAddBleDevice(info->address, BLE_ADDR_PUBLIC, BT_DEVICE_TYPE_BLE);
            return;
        } else if (info->magic == MAGIC_BLOOPAIR_BLE) {
            DEBUG_PRINT("BTA_DmAddDevice_hook: %s %d\n", bdaddr_to_string(info->address), info->ble.address_type);
            BTA_DmAddBleDevice(info->address, info->ble.address_type, BT_DEVICE_TYPE_BLE);
            return;
        }
    }

    real_BTA_DmAddDevice(bd_addr, dev_class, link_key, trusted_mask, is_trusted, key_type, io_cap);
}
