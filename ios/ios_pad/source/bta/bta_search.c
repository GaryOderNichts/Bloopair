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
#include <bta/bta_api.h>
#include "controllers.h"
#include "scan.h"
#include "audio.h"

#define PRO_CONTROLLER_NAME "Nintendo RVL-CNT-01-UC"

void (*const real_bta_search_callback)(uint8_t event, void *p_data) = (void*) 0x11f401a8;
void bta_search_callback(uint8_t event, void *p_data)
{
    DEBUG_PRINT("bta_search_callback called %u %p\n", event, p_data);

    switch (event) {
    case BTA_DM_INQ_RES_EVT: {
        tBTA_DM_INQ_RES* res = (tBTA_DM_INQ_RES*) p_data;

        // If this is a BLE device don't do remote name lookup (not implemented)
        if (res->device_type == BT_DEVICE_TYPE_BLE) {
            res->remt_name_not_required = 1;
        }

        if (scanIsEnabled()) {
            scanOnInquiryResult(res);

            // devices other than peripherals are only recorded,
            // the original callback should only ever see controllers
            if ((res->dev_class[1] & BTM_COD_MAJOR_CLASS_MASK) != BTM_COD_MAJOR_PERIPHERAL) {
                return;
            }
        }
        break;
    }

    case BTA_DM_DISC_RES_EVT: {
        tBTA_DM_DISC_RES* res = (tBTA_DM_DISC_RES*) p_data;

        if (scanIsEnabled()) {
            scanOnName(res->bd_addr, res->bd_name);

            // known non-peripheral devices are not passed on
            if (scanDeviceKind(res->bd_addr) == 0) {
                return;
            }
        }

        if (res->result == 0 && !isOfficialName((const char*) res->bd_name)) {
            DEBUG_PRINT("[%s] \"%s\" is non official, replacing name...\n",
                bdaddr_to_string(res->bd_addr), res->bd_name);
            // replace device name
            memcpy(res->bd_name, PRO_CONTROLLER_NAME, sizeof(PRO_CONTROLLER_NAME));
        }
        break;
    }
    default:
        break;
    }

    // lets the audio test start its connection once the search is over
    audioOnSearchEvent(event);

    real_bta_search_callback(event, p_data);
}

static uint8_t current_inq_mode = BTM_LIMITED_INQUIRY;

void (*const real_BTA_DmSearch)(tBTA_DM_INQ *p_dm_inq, uint32_t services, void *p_cback) = (void*) 0x11f04c7c;
void BTA_DmSearch_hook(tBTA_DM_INQ *p_dm_inq, uint32_t services, void *p_cback)
{
    DEBUG_PRINT("BTA_DmSearch_hook %ds (%s)\n", p_dm_inq->duration,
        (current_inq_mode == BTM_LIMITED_INQUIRY) ? "BTM_LIMITED_INQUIRY" : "BTM_GENERAL_INQUIRY");

    audioOnSearchStart(services);

    // no new search while the audio test pairs and connects
    if (audioBlockSearch()) {
        return;
    }

    if (scanIsEnabled()) {
        // scan mode: look for every discoverable BR/EDR device, not only controllers (audio devices are BR/EDR)
        p_dm_inq->mode = BTM_GENERAL_INQUIRY;
        p_dm_inq->filter_type = BTM_CLR_INQUIRY_FILTER;
        p_dm_inq->max_resps = 0;
        real_BTA_DmSearch(p_dm_inq, services, p_cback);
        return;
    }

    // switch between limited and general inquiry to make sure all BR/EDR devices are covered
    // BLE is always enabled
    p_dm_inq->mode = current_inq_mode | BTM_BLE_GENERAL_INQUIRY;
    current_inq_mode = (current_inq_mode == BTM_LIMITED_INQUIRY) ? BTM_GENERAL_INQUIRY : BTM_LIMITED_INQUIRY;

    // only search for peripherals
    // BLE filtering happens in `btm_ble_is_discoverable_hook`
    p_dm_inq->filter_type = BTM_FILTER_COND_DEVICE_CLASS;
    p_dm_inq->filter_cond.dev_class_cond.dev_class_mask[0] = 0;
    p_dm_inq->filter_cond.dev_class_cond.dev_class_mask[1] = BTM_COD_MAJOR_CLASS_MASK;
    p_dm_inq->filter_cond.dev_class_cond.dev_class_mask[2] = 0;
    p_dm_inq->filter_cond.dev_class_cond.dev_class[0] = 0;
    p_dm_inq->filter_cond.dev_class_cond.dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
    p_dm_inq->filter_cond.dev_class_cond.dev_class[2] = 0;

    real_BTA_DmSearch(p_dm_inq, services, p_cback);
}
