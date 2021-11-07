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

#include <imports.h>
#include <bt_api.h>

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

void (*const real_bta_sec_callback)(uint8_t event, void *p_data) = (void*) 0x11f3fd88;
void bta_sec_callback(uint8_t event, void *p_data)
{
    DEBUG("bta_sec_callback called %u %p\n", event, p_data);

    switch (event) {
    case BTA_DM_SP_CFM_REQ_EVT: {
        tBTA_DM_SP_CFM_REQ* req_data = (tBTA_DM_SP_CFM_REQ*) p_data;
        // always confirm ssp requests
        BTA_DmConfirm(req_data->bd_addr, 1);
        return;
    }
    default:
        break;
    }

    real_bta_sec_callback(event, p_data);
}
