/*
 *   Copyright (C) 2021-2023 GaryOderNichts
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

#include "bloopair.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <coreinit/ipcbufpool.h>

typedef struct {
    IOSVec vecs[2];
    uint8_t padding[104];
    BtrmRequest request;
    uint8_t padding1[120];
    BtrmResponse response;
    uint8_t padding2[116];
} BtrmIoctlv;

static BtrmIoctlv* allocBtrmIoctlv(uint8_t lib, uint8_t func)
{
    BtrmIoctlv* ioctlv = (BtrmIoctlv*) memalign(0x40, sizeof(BtrmIoctlv));
    if (!ioctlv) {
        return NULL;
    }

    memset(ioctlv->request.data, 0, sizeof(ioctlv->request.data));
    ioctlv->request.lib = lib;
    ioctlv->request.func = func;
    ioctlv->request.unk = 0;
    ioctlv->request.unk1 = 0;

    ioctlv->vecs[0].vaddr = &ioctlv->request;
    ioctlv->vecs[0].len = sizeof(BtrmRequest);
    ioctlv->vecs[1].vaddr = &ioctlv->response;
    ioctlv->vecs[1].len = sizeof(BtrmResponse);

    return ioctlv;
}

static void freeBtrmIoctlv(BtrmIoctlv* ioctlv)
{
    free(ioctlv);
}

static IOSError executeBtrmIoctlv(IOSHandle handle, BtrmIoctlv* ioctlv)
{
    return IOS_Ioctlv(handle, 0, 1, 1, ioctlv->vecs);
}

IOSHandle Bloopair_Open(void)
{
    return IOS_Open("/dev/usb/btrm", (IOSOpenMode) 0);
}

IOSError Bloopair_Close(IOSHandle handle)
{
    return IOS_Close(handle);
}

BOOL Bloopair_IsActive(IOSHandle handle)
{
    return Bloopair_GetVersion(handle) >= 0;
}

int32_t Bloopair_GetVersion(IOSHandle handle)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_GET_VERSION);
    if (!ioctlv) {
        return IOS_ERROR_INVALID;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    freeBtrmIoctlv(ioctlv);

    return (int32_t) res;
}

// reads the bluetooth device address of the local bluetooth controller
IOSError Bloopair_ReadControllerBDA(IOSHandle handle, uint8_t* outBDA)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_READ_DEVICE_BDADDR);
    if (!ioctlv) {
        return IOS_ERROR_INVALID;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        memcpy(outBDA, ioctlv->response.data, 6);
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

// manually adds a controller pairing
IOSError Bloopair_AddControllerPairing(IOSHandle handle, uint8_t* bda, uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING);
    if (!ioctlv) {
        return IOS_ERROR_INVALID;
    }

    BloopairPairingData* pairingData = (BloopairPairingData*) ioctlv->request.data;
    memcpy(pairingData->bd_address, bda, 6);
    memcpy(pairingData->link_key, link_key, 16);
    strncpy((char*) pairingData->name, name, 63);
    pairingData->vendor_id = vid;
    pairingData->product_id = pid;

    IOSError res = executeBtrmIoctlv(handle, ioctlv);

    freeBtrmIoctlv(ioctlv);

    return res;
}
