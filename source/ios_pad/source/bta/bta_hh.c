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
#include <controllers.h>
#include <info_store.h>

tBTA_HH_CB* bta_hh_cb = (tBTA_HH_CB*) 0x1214d718;

void (*const real_bta_hh_event)(uint8_t event, void *p_data) = (void*) 0x11f405ac;
void bta_hh_event(uint8_t event, void *p_data)
{
    DEBUG("bta_hh_event called %u %p\n", event, p_data);

    switch (event) {
    case BTA_HH_OPEN_EVT: {
        tBTA_HH_CONN* conn_data = (tBTA_HH_CONN*) p_data;
        DEBUG("open event for handle %u\n", conn_data->handle);

        if (conn_data->handle != BTA_HH_INVALID_HANDLE) {
            StoredInfo_t* info = store_get_device_info(conn_data->bda);
            DEBUG("found info for device %p\n", info);

            if (!info || initController(conn_data->handle, info->magic, info->vendor_id, info->product_id) != 0) {
                // close connection
                BTA_HhClose(conn_data->handle);
                return;
            }
        }
        break;
    }
    case BTA_HH_CLOSE_EVT: {
        tBTA_HH_CBDATA* cb_data = (tBTA_HH_CBDATA*) p_data;
        DEBUG("close event handle %u status %x\n", cb_data->handle, cb_data->status);

        if (cb_data->handle != BTA_HH_INVALID_HANDLE) {
            Controller_t* controller = &controllers[cb_data->handle];
            if (!controller->isInitialized) {
                return;
            }

            if (controller->deinit) {
                controller->deinit(controller);
            }

            controller->isInitialized = 0;
        }
        break;
    }
    case BTA_HH_VC_UNPLUG_EVT: {
        tBTA_HH_CBDATA* cb_data = (tBTA_HH_CBDATA*) p_data;
        DEBUG("vc unplug %u\n", cb_data->handle);

        // disconnect virtually unplugged devices
        if (cb_data->handle != BTA_HH_INVALID_HANDLE) {
            BTA_HhClose(cb_data->handle);
        }
        break;
    }
    default:
        break;
    }

    real_bta_hh_event(event, p_data);
}

void (*const real_bta_hh_api_disable)(void) = (void*) 0x11f07174;
void bta_hh_api_disable(void)
{
    DEBUG("bta_hh_api_disable\n");

    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (controllers[i].isInitialized) {
            if (controllers[i].deinit) {
                controllers[i].deinit(&controllers[i]);
            }
            controllers[i].isInitialized = 0;
        }
    }

    deinitReportThread();
    stop_info_thread();

    real_bta_hh_api_disable();    
}
