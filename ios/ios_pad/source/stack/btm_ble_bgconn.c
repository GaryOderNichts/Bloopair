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
#include "btm.h"
#include "hci.h"

uint8_t BTM_BleUpdateBgConnDev(uint8_t add_remove, BD_ADDR remote_bda)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    tBTM_BLE_SEL_CBACK   *p_select_cback;
    uint8_t ret = 1;

    // BTM_TRACE_EVENT0 (" BTM_BleUpdateBgConnDev");

    /* if auto connection is active */
    if (p_cb->bg_conn_state == BLE_BG_CONN_ACTIVE)
    {
        if (p_cb->bg_conn_type == BTM_BLE_CONN_AUTO)
        {
            /* terminate auto connection first */
            ret = btm_ble_start_auto_conn(0);
        }
        else if (p_cb->bg_conn_type == BTM_BLE_CONN_SELECTIVE)
        {
            p_select_cback = btm_cb.ble_ctr_cb.p_select_cback;
            ret = btm_ble_start_select_conn(0, NULL);
        }
    }
    if (ret)
    {
        /* update white list */
        ret = btm_update_bg_conn_list(add_remove, remote_bda);
        btm_update_dev_to_white_list(add_remove, remote_bda, BLE_ADDR_PUBLIC);
    }

    if (ret && p_cb->bg_conn_state == BLE_BG_CONN_IDLE)
    {
        if (p_cb->bg_conn_type == BTM_BLE_CONN_AUTO)
        {
            /* restart auto connection */
            btm_ble_start_auto_conn(1);
        }
        else if (p_cb->bg_conn_type == BTM_BLE_CONN_SELECTIVE)
        {
            p_select_cback = btm_cb.ble_ctr_cb.p_select_cback;
            btm_ble_start_select_conn(1, p_select_cback);
        }
    }
    return ret;
}

uint8_t btm_update_bg_conn_list(uint8_t to_add, BD_ADDR bd_addr)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    uint8_t i;
    BD_ADDR dummy_bda = {0};
    // BTM_TRACE_EVENT0 ("btm_update_bg_conn_list");
    if ((to_add && (p_cb->bg_conn_dev_num == BTM_BLE_MAX_BG_CONN_DEV_NUM || p_cb->num_empty_filter == 0)) ||
        (!to_add && p_cb->num_empty_filter == p_cb->max_filter_entries))
    {
        // BTM_TRACE_DEBUG1("num_empty_filter = %d", p_cb->num_empty_filter);
        return 0;
    }

    for (i = 0; i < BTM_BLE_MAX_BG_CONN_DEV_NUM && i < p_cb->max_filter_entries; i ++)
    {
        /* to add */
        if (memcmp(p_cb->bg_conn_dev_list[i], dummy_bda, BD_ADDR_LEN) == 0 && to_add)
        {
            memcpy(p_cb->bg_conn_dev_list[i], bd_addr, BD_ADDR_LEN);
            p_cb->bg_conn_dev_num ++;
            return 1;
        }
        /* to remove */
        if (!to_add && memcmp(p_cb->bg_conn_dev_list[i], bd_addr, BD_ADDR_LEN) == 0)
        {
            memset(p_cb->bg_conn_dev_list[i], 0, BD_ADDR_LEN);
            p_cb->bg_conn_dev_num --;
            return 1;
        }
    }
    return 0;
}

uint8_t btm_ble_find_dev_in_whitelist(BD_ADDR bd_addr)
{
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    uint8_t i;

    // BTM_TRACE_EVENT0 ("btm_ble_find_dev_in_whitelist");

    /* empty wl */
    if (p_cb->num_empty_filter == p_cb->max_filter_entries)
    {
        // BTM_TRACE_DEBUG0("white list empty");
        return 0;
    }

    for (i = 0; i < BTM_BLE_MAX_BG_CONN_DEV_NUM && i < p_cb->max_filter_entries; i ++)
    {
        if (memcmp(p_cb->bg_conn_dev_list[i], bd_addr, BD_ADDR_LEN) == 0)
            return 1;
    }
    return 0;
}

uint8_t btm_update_dev_to_white_list(uint8_t to_add, BD_ADDR bd_addr, uint8_t addr_type)
{
    /* look up the sec device record, and find the address */
    tBTM_BLE_CB *p_cb = &btm_cb.ble_ctr_cb;
    tBTM_SEC_DEV_REC    *p_dev_rec;
    BD_ADDR             dummy_bda = {0};
    uint8_t             started = 0, suspend = 0;

    if (btm_cb.btm_inq_vars.inq_active)
    {
        suspend = 1;
        btsnd_hcic_ble_set_scan_enable (BTM_BLE_SCAN_DISABLE, BTM_BLE_DUPLICATE_ENABLE);
    }

    if ((to_add && p_cb->num_empty_filter == 0) ||
        (!to_add && p_cb->num_empty_filter == p_cb->max_filter_entries))
    {
        // BTM_TRACE_ERROR1("num_entry available in controller: %d", p_cb->num_empty_filter);
        return started;
    }

    if ((p_dev_rec = btm_find_dev (bd_addr)) != NULL &&
        p_dev_rec->device_type == BT_DEVICE_TYPE_BLE)
    {
        // BTM_TRACE_DEBUG0("btm_update_dev_to_white_list 1");

        if ( p_dev_rec->ble.ble_addr_type == BLE_ADDR_PUBLIC)
        {
            if (to_add)
                started = btsnd_hcic_ble_add_white_list (BLE_ADDR_PUBLIC, bd_addr);
            else
                started = btsnd_hcic_ble_remove_from_white_list (BLE_ADDR_PUBLIC, bd_addr);
        }
        else
        {
            if (BLE_ADDR_IS_STATIC(bd_addr))
            {
                if (to_add)
                    started = btsnd_hcic_ble_add_white_list (BLE_ADDR_RANDOM, bd_addr);
                else
                    started = btsnd_hcic_ble_remove_from_white_list (BLE_ADDR_RANDOM, bd_addr);

            }
            if (memcmp(p_dev_rec->ble.reconn_addr, dummy_bda, BD_ADDR_LEN) != 0)
            {
                if (to_add)
                    started = btsnd_hcic_ble_add_white_list (BLE_ADDR_RANDOM, p_dev_rec->ble.reconn_addr);
                else
                    started = btsnd_hcic_ble_remove_from_white_list (BLE_ADDR_RANDOM, p_dev_rec->ble.reconn_addr);
            }
        }
    }
    /* if not a known device, shall we add it? */
    else
    {
        if (to_add)
            started = btsnd_hcic_ble_add_white_list (addr_type, bd_addr);
        else
            started = btsnd_hcic_ble_remove_from_white_list (addr_type, bd_addr);
    }

    if (suspend)
    {
        btsnd_hcic_ble_set_scan_enable (BTM_BLE_SCAN_ENABLE, BTM_BLE_DUPLICATE_DISABLE);
    }

    return started;
}
