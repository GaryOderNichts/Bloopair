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
#include "controllers.h"

#define PRO_CONTROLLER_NAME "Nintendo RVL-CNT-01-UC"

void (*const real_bta_search_callback)(uint8_t event, void *p_data) = (void*) 0x11f401a8;
void bta_search_callback(uint8_t event, void *p_data)
{
    DEBUG_PRINT("bta_search_callback called %u %p\n", event, p_data);

    switch (event) {
    case BTA_DM_DISC_RES_EVT: {
        tBTA_DM_DISC_RES* res = (tBTA_DM_DISC_RES*) p_data;
        if (res->result == 0 && !isOfficialName((const char*) res->bd_name)) {
            DEBUG_PRINT("%s is non official, replacing name...\n", res->bd_name);
            // replace device name
            memcpy(res->bd_name, PRO_CONTROLLER_NAME, sizeof(PRO_CONTROLLER_NAME));
        }
        break;
    }
    default:
        break;
    }

    real_bta_search_callback(event, p_data);
}

static uint8_t current_inq_mode = BTM_LIMITED_INQUIRY;

void (*const real_BTA_DmSearch)(tBTA_DM_INQ *p_dm_inq, uint32_t services, void *p_cback) = (void*) 0x11f04c7c;
void BTA_DmSearch_hook(tBTA_DM_INQ *p_dm_inq, uint32_t services, void *p_cback)
{
    DEBUG_PRINT("BTA_DmSearch_hook %ds (%s)\n", p_dm_inq->duration,
        (current_inq_mode == BTM_LIMITED_INQUIRY) ? "BTM_LIMITED_INQUIRY" : "BTM_GENERAL_INQUIRY");

    // switch between limited and general inquiry to make sure all devices are covered
    p_dm_inq->mode = current_inq_mode;
    current_inq_mode = (current_inq_mode == BTM_LIMITED_INQUIRY) ? BTM_GENERAL_INQUIRY : BTM_LIMITED_INQUIRY;

    // only search for peripherals
    p_dm_inq->filter_type = BTM_FILTER_COND_DEVICE_CLASS;
    p_dm_inq->filter_cond.dev_class_cond.dev_class_mask[0] = 0;
    p_dm_inq->filter_cond.dev_class_cond.dev_class_mask[1] = BTM_COD_MAJOR_CLASS_MASK;
    p_dm_inq->filter_cond.dev_class_cond.dev_class_mask[2] = 0;
    p_dm_inq->filter_cond.dev_class_cond.dev_class[0] = 0;
    p_dm_inq->filter_cond.dev_class_cond.dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
    p_dm_inq->filter_cond.dev_class_cond.dev_class[2] = 0;

    real_BTA_DmSearch(p_dm_inq, services, p_cback);
}
