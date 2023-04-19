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

#include "ipc.h"
#include "info_store.h"
#include <bloopair/ipc.h>

static int bloopairFunc(BtrmRequest* request, BtrmResponse* response)
{
    switch (request->func) {
    case BLOOPAIR_FUNC_GET_VERSION:
        DEBUG("BLOOPAIR_FUNC_GET_VERSION\n");
        return BLOOPAIR_VERSION(0, 6, 2);

    case BLOOPAIR_FUNC_READ_DEVICE_BDADDR: {
        DEBUG("BLOOPAIR_FUNC_READ_DEVICE_BDADDR\n");
        memcpy(response->data, local_device_bdaddr, 6);
        return 0;
    }
    
    case BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING: {
        DEBUG("BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING\n");

        BloopairPairingData* data = (BloopairPairingData*) request->data;

        // register the device pairing
        registerNewDevice(data->bd_address, data->link_key, data->name);

        // store the link key so the pairing doesn't get removed for not having one
        BTM_WriteStoredLinkKey(1, data->bd_address, data->link_key, NULL);

        // add the info to our store
        StoredInfo* info = store_get_device_info(data->bd_address);
        if (!info) {
            info = store_allocate_device_info(data->bd_address);
            if (!info) {
                return -1;
            }
        }

        info->magic = MAGIC_BLOOPAIR;
        info->product_id = data->product_id;
        info->vendor_id = data->vendor_id;
        
        return 0;
    }
    }

    return -1;
}

int btrmCustomLibHook(BtrmRequest* request, BtrmResponse* response)
{
    return bloopairFunc(request, response);
}

// return non-0 if btrmCustomLibHook should be called for this lib
int btrmCheckCustomLib(uint8_t lib)
{
    if (lib == BLOOPAIR_LIB) {
        return 1;
    }

    return 0;
}
