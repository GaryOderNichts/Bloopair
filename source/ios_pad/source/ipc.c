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

#include "ipc.h"
#include "info_store.h"
#include "../../bloopair_ipc.h"

static int bloopairFunc(BtrmRequest_t* request, BtrmResponse_t* response)
{
    switch (request->func) {
    case BLOOPAIR_FUNC_IS_ACTIVE:
        DEBUG("BLOOPAIR_FUNC_IS_ACTIVE\n");
        return 0;

    case BLOOPAIR_FUNC_READ_DEVICE_BDADDR: {
        DEBUG("BLOOPAIR_FUNC_READ_DEVICE_BDADDR\n");
        _memcpy(response->data, local_device_bdaddr, 6);
        return 0;
    }
    
    case BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING: {
        DEBUG("BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING\n");

        PairingData_t* data = (PairingData_t*) request->data;

        // register the device pairing
        registerNewDevice(data->bd_address, data->link_key, data->name);

        // store the link key so the pairing doesn't get removed for not having one
        BTM_WriteStoredLinkKey(1, data->bd_address, data->link_key, NULL);

        // add the info to our store
        StoredInfo_t* info = store_get_device_info(data->bd_address);
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

int btrm_receive_message_hook(int queueid, IPCMessage_t **p_message, uint32_t flags)
{
    int res = IOS_ReceiveMessage(queueid, (uint32_t*) p_message, flags);
    if (res != 0) {
        return res;
    }

    IPCMessage_t* message = *p_message;

    // there are some event messages which aren't ipcmessage pointers
    if (message < (IPCMessage_t*) 0x1000) {
        return res;
    }

    if (message->command == IOS_IOCTLV && message->ioctlv.command == 0 &&
        message->ioctlv.num_in == 1 && message->ioctlv.num_out == 1 &&
        message->ioctlv.vecs[0].len == sizeof(BtrmRequest_t) &&
        message->ioctlv.vecs[1].len == sizeof(BtrmResponse_t)) {

        BtrmRequest_t* request = (BtrmRequest_t*) message->ioctlv.vecs[0].ptr;
        BtrmResponse_t* response = (BtrmResponse_t*) message->ioctlv.vecs[1].ptr;

        if (request->lib == BLOOPAIR_LIB) {
            // if this was a bloopair command reply and wait for the next message
            IOS_ResourceReply(message, bloopairFunc(request, response));
            return btrm_receive_message_hook(queueid, p_message, flags);
        }
    }

    return res;
}
