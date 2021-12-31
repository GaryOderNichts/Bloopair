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

#include "ipc.hpp"
#include "bloopair_ipc.h"

#include "malloc.h"
#include <cstring>
#include <coreinit/cache.h>

struct BtrmIoctlv {
    IOSVec vecs[2];
    BtrmRequest_t request;
    BtrmResponse_t response;
};

static void initBtrmIoctlv(BtrmIoctlv* ioctlv, uint8_t lib, uint8_t func)
{
    ioctlv->vecs[0].vaddr = &ioctlv->request;
    ioctlv->vecs[0].len = sizeof(BtrmRequest_t);
    ioctlv->vecs[1].vaddr = &ioctlv->response;
    ioctlv->vecs[1].len = sizeof(BtrmResponse_t);

    ioctlv->request.lib = lib;
    ioctlv->request.func = func;
}

static IOSError executeBtrmIoctlv(IOSHandle handle, BtrmIoctlv* ioctlv)
{
    return IOS_Ioctlv(handle, 0, 1, 1, ioctlv->vecs);
}

IOSHandle openBtrm()
{
    return IOS_Open("/dev/usb/btrm", (IOSOpenMode) 0);
}

void closeBtrm(IOSHandle handle)
{
    IOS_Close(handle);
}

bool isBloopairRunning(IOSHandle handle)
{
    BtrmIoctlv* ioctlv = (BtrmIoctlv*) memalign(0x20, sizeof(BtrmIoctlv));
    initBtrmIoctlv(ioctlv, BLOOPAIR_LIB, BLOOPAIR_FUNC_IS_ACTIVE);

    // if bloopair isn't running this will fail since BLOOPAIR_LIB isn't a valid lib
    bool running = executeBtrmIoctlv(handle, ioctlv) >= 0;

    free(ioctlv);

    return running;
}

IOSError readControllerBDAddr(IOSHandle handle, uint8_t* outBDA)
{
    BtrmIoctlv* ioctlv = (BtrmIoctlv*) memalign(0x20, sizeof(BtrmIoctlv));
    initBtrmIoctlv(ioctlv, BLOOPAIR_LIB, BLOOPAIR_FUNC_READ_DEVICE_BDADDR);

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        memcpy(outBDA, ioctlv->response.data, 6);
    }

    free(ioctlv);

    return res;
}

IOSError addControllerPairing(IOSHandle handle, uint8_t* bda, uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid)
{
    BtrmIoctlv* ioctlv = (BtrmIoctlv*) memalign(0x20, sizeof(BtrmIoctlv));
    initBtrmIoctlv(ioctlv, BLOOPAIR_LIB, BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING);

    PairingData_t pairingData{};
    memcpy(pairingData.bd_address, bda, 6);
    memcpy(pairingData.link_key, link_key, 16);
    strncpy((char*) pairingData.name, name, 63);
    pairingData.vendor_id = vid;
    pairingData.product_id = pid;

    memcpy(ioctlv->request.data, &pairingData, sizeof(PairingData_t));

    IOSError result = executeBtrmIoctlv(handle, ioctlv);

    free(ioctlv);

    return result;
}
