/*
 *   Copyright (C) 2021 GaryOderNichts
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

#include "bta_hh.h"
#include "bt_api.h"
#include <controllers.h>
#include <info_store.h>

void bta_hh_sm_execute(tBTA_HH_DEV_CB *p_cb, uint16_t event, void * p_data);
void bta_hh_start_sdp(tBTA_HH_DEV_CB *p_cb, void *p_data);
void bta_hh_sdp_cback(uint16_t result, uint16_t attr_mask, void *sdp_rec);

void name_read_cback(tBTM_REMOTE_DEV_NAME* name)
{
    DEBUG("got name %s status %d\n", name->remote_bd_name, name->status);

    static int retry = 0;
    if (name->status != 0) {
        if (retry++ < 4) {
            // retry until we succeed
            BTM_ReadRemoteDeviceName(bta_hh_cb->p_cur->addr, name_read_cback);
        }
        else {
            retry = 0;
            utl_freebuf((void **)&bta_hh_cb->p_disc_db);
            uint8_t status = 7;
            bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_SDP_CMPL_EVT, &status);
        }
        return;
    }

    retry = 0;

    StoredInfo_t* info = store_get_device_info(bta_hh_cb->p_cur->addr);
    if (!info) {
        info = store_allocate_device_info(bta_hh_cb->p_cur->addr);
    }

    if (isOfficialName((const char*) name->remote_bd_name)) {
        info->magic = MAGIC_OFFICIAL;
    }
    else {
        info->magic = MAGIC_BLOOPAIR;
    }

    // continue with getting the sdp record
    if (HID_HostGetSDPRecord(bta_hh_cb->p_cur->addr, bta_hh_cb->p_disc_db, sdp_db_size, bta_hh_sdp_cback) != 0) {
        utl_freebuf((void **)&bta_hh_cb->p_disc_db);
        uint8_t status = 7;
        bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_SDP_CMPL_EVT, &status);
    }
}

void bta_hh_di_sdp_callback(uint16_t result)
{
    DEBUG("bta_hh_di_sdp_callback called res: %u\n", result);

    // make sure our info thread is running
    start_info_thread();

    static int retry = 0;
    if (result != 0) {
        if (retry++ < 4) {
            // retry until we succeed
            SDP_DiDiscover(bta_hh_cb->p_cur->addr, bta_hh_cb->p_disc_db, sdp_db_size, bta_hh_di_sdp_callback);
        }
        else {
            retry = 0;
            utl_freebuf((void **)&bta_hh_cb->p_disc_db);
            /* send SDP_CMPL_EVT into state machine */
            uint8_t status = 7;
            bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_SDP_CMPL_EVT, &status);
        }
        return;
    }

    retry = 0;

    ReportMessage_t* message = IOS_Alloc(0xcaff, sizeof(ReportMessage_t) + sdp_db_size);
    if (message) {
        message->type = MESSAGE_TYPE_DI_RECORD;
        // copy bdaddr to the buffer
        memcpy(message->addr, bta_hh_cb->p_cur->addr, BD_ADDR_LEN);
        // copy our db to the buffer
        memcpy(message->data, bta_hh_cb->p_disc_db, sdp_db_size);
        // send the message
        IOS_SendMessage(info_message_queue, (uint32_t) message, 0);
    }

    BTM_ReadRemoteDeviceName(bta_hh_cb->p_cur->addr, name_read_cback);
}

void name_read_cback_open(tBTM_REMOTE_DEV_NAME* name)
{
    DEBUG("open: got name %s status %d\n", name->remote_bd_name, name->status);

    if (name->status == 0) {
        StoredInfo_t* info = store_get_device_info(bta_hh_cb->p_cur->addr);
        if (!info) {
            info = store_allocate_device_info(bta_hh_cb->p_cur->addr);
        }

        if (isOfficialName((const char*) name->remote_bd_name)) {
            info->magic = MAGIC_OFFICIAL;
        }
        else {
            info->magic = MAGIC_BLOOPAIR;
        }
    }

    bta_hh_sm_execute(bta_hh_cb->p_cur, BTA_HH_OPEN_CMPL_EVT, NULL);
}

void bta_hh_open_act(tBTA_HH_DEV_CB *p_cb, void *p_data)
{
    tBTA_HH_API_CONN    conn_data;

    DEBUG("bta_hh_open_act handle %d %d\n", p_cb->hid_handle, p_cb->app_id);

    // make sure our info thread is running
    start_info_thread();

    /* SDP has been done */
    if (p_cb->app_id != 0) {
        StoredInfo_t* info = store_get_device_info(p_cb->addr);

        // controllers paired without bloopair running have an unknown entry in the store
        if (info && info->magic == MAGIC_UNKNOWN) {
            bta_hh_cb->p_cur = p_cb;
            BTM_ReadRemoteDeviceName(p_cb->addr, name_read_cback_open);
        }
        else {
            bta_hh_sm_execute(p_cb, BTA_HH_OPEN_CMPL_EVT, p_data);
        }
    }
    else
    /*  app_id == 0 indicates an incoming conenction request arrives without SDP
        performed, do it first */
    {
        p_cb->incoming_conn = 1;

        memset(&conn_data, 0, sizeof(tBTA_HH_API_CONN));
        bdcpy(conn_data.bd_addr, p_cb->addr);
        bta_hh_start_sdp(p_cb, (void*) &conn_data);
    }
}
