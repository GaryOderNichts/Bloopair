/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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

#include "bta_hh.h"
#include "bt_api.h"

void (*const bta_hh_sm_execute)(tBTA_HH_DEV_CB *p_cb, uint16_t event, void * p_data) = (void*) 0x11f07a88;
void (*const bta_hh_start_sdp)(tBTA_HH_DEV_CB *p_cb, void *p_data) = (void*) 0x11f06d5c;
void (*const bta_hh_sdp_cback)(uint16_t result, uint16_t attr_mask, void *sdp_rec) = (void*) 0x11f06fa4;

void name_read_cback(tBTM_REMOTE_DEV_NAME* name)
{
    DEBUG("got name %s status %d\n", name->remote_bd_name, name->status);

    // getting the sdp record right after the name seems to fail
    // so we wait for a bit
    usleep(7000);

    static int retry = 0;
    if (name->status != 0) {
        if (retry++ < 4) {
            // retry until we succeed
            BTM_ReadRemoteDeviceName(bta_hh_cb->p_cur->addr, name_read_cback);
        }
        else {
            uint8_t status = 7;
            bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_SDP_CMPL_EVT, &status);
        }
        return;
    }

    retry = 0;

    hh_dev_user_data_t* userData = &hh_dev_user_data[bta_hh_cb->p_cur->index];
    _memcpy(userData->device_name, name->remote_bd_name, name->length);

    userData->additional_data_read = 1;

    if (HID_HostGetSDPRecord(bta_hh_cb->p_cur->addr, bta_hh_cb->p_disc_db, sdp_db_size, bta_hh_sdp_cback) != 0) {
        uint8_t status = 7;
        bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_SDP_CMPL_EVT, &status);
    }
}

uint8_t HID_HostGetSDPRecord_hook(uint8_t *addr, tSDP_DISCOVERY_DB *p_db, uint32_t db_len, void* p_cb)
{
    BTM_ReadRemoteDeviceName(bta_hh_cb->p_cur->addr, name_read_cback);

    return 0;
}

void name_read_cback_open(tBTM_REMOTE_DEV_NAME* name)
{
    DEBUG("open: got name %s status %d\n", name->remote_bd_name, name->status);

    usleep(7000);

    if (name->status != 0) {
        return;
    }

    hh_dev_user_data_t* userData = &hh_dev_user_data[bta_hh_cb->p_cur->index];
    _memcpy(userData->device_name, name->remote_bd_name, name->length);

    userData->additional_data_read = 1;

    bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_OPEN_CMPL_EVT, NULL);
}

void bta_hh_open_act(tBTA_HH_DEV_CB *p_cb, void *p_data)
{
    tBTA_HH_API_CONN    conn_data;

    DEBUG("bta_hh_open_act handle %d %d\n", p_cb->hid_handle, p_cb->app_id);

    hh_dev_user_data_t* userData = &hh_dev_user_data[p_cb->index];
    if (userData->additional_data_read && p_cb->app_id) {
        DEBUG("open cmpl\n");
        // if we already read the additional data just finish the open event
        bta_hh_sm_execute(p_cb, BTA_HH_OPEN_CMPL_EVT, p_data);
    }

    /* SDP has been done */
    if (p_cb->app_id != 0) {
        DEBUG("reading name\n");
        bta_hh_cb->p_cur = p_cb;
        BTM_ReadRemoteDeviceName(p_cb->addr, name_read_cback_open);
    }
    else
    /*  app_id == 0 indicates an incoming conenction request arrives without SDP
        performed, do it first */
    {
        p_cb->incoming_conn = 1;

        _memset(&conn_data, 0, sizeof(tBTA_HH_API_CONN));
        bdcpy(conn_data.bd_addr, p_cb->addr);
        bta_hh_start_sdp(p_cb, (void*) &conn_data);
    }
}
