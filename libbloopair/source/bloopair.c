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

#include "bloopair/bloopair.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

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

uint8_t* getWPADData(void)
{
    // This is incredibly hacky
    // WPADGetPowerSaveMode loads the WPADData pointer into r0 and doesn't clear it
    // So call it and read r0 afterwards
    uint8_t** wpadData = NULL;
    WPADGetPowerSaveMode(WPAD_CHAN_0);
    asm("or %0, 0, 0" : "=r"(wpadData));
    return *wpadData;
}

static uint8_t getHandleForChannel(WPADChan chan)
{
    uint8_t* ctx = getWPADData() + (chan * 0xc28);
    return ctx[0x90b];
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
        return IOS_ERROR_FAILALLOC;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    freeBtrmIoctlv(ioctlv);

    return (int32_t) res;
}

IOSError Bloopair_GetCommitHash(IOSHandle handle, char* commitHashStr, uint32_t commitHashStrLen)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_GET_COMMIT_HASH);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        strncpy(commitHashStr, (char*) ioctlv->response.data, commitHashStrLen);
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_ReadConsoleBDA(IOSHandle handle, uint8_t* outBDA)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_READ_CONSOLE_BDADDR);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        memcpy(outBDA, ioctlv->response.data, 6);
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_AddControllerPairing(IOSHandle handle, const uint8_t* bda, const uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
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

IOSError Bloopair_GetControllerInformation(IOSHandle handle, WPADChan chan, BloopairControllerInformationData* data)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairControllerRequestData* request = (BloopairControllerRequestData*) ioctlv->request.data;
    request->handle = getHandleForChannel(chan);

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        if (res != sizeof(*data)) {
            return IOS_ERROR_INVALIDSIZE;
        }

        memcpy(data, ioctlv->response.data, res);
        res = IOS_ERROR_OK;
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_ReadRawReport(IOSHandle handle, WPADChan chan, BloopairReportBuffer* outReport)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_READ_RAW_REPORT);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairControllerRequestData* request = (BloopairControllerRequestData*) ioctlv->request.data;
    request->handle = getHandleForChannel(chan);

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        if (res != sizeof(*outReport)) {
            return IOS_ERROR_INVALIDSIZE;
        }

        memcpy(outReport, ioctlv->response.data, res);
        res = IOS_ERROR_OK;
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

static IOSError _Bloopair_ApplyControllerConfiguration(IOSHandle handle, BloopairControllerType controllerType, const uint8_t* bda, const BloopairCommonConfiguration* commonConfiguration)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairApplyControllerConfigurationData* configData = (BloopairApplyControllerConfigurationData*) ioctlv->request.data;
    configData->controllerType = controllerType;
    if (bda) {
        memcpy(configData->bd_address, bda, 6);
    } else {
        memset(configData->bd_address, 0, 6);
    }

    if (commonConfiguration) {
        configData->dataSize = sizeof(*commonConfiguration);
        memcpy(configData->data, commonConfiguration, configData->dataSize);
    } else {
        configData->dataSize = 0;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_ApplyControllerConfigurationForBDA(IOSHandle handle, const uint8_t* bda, const BloopairCommonConfiguration* commonConfiguration)
{
    if (!bda) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_ApplyControllerConfiguration(handle, BLOOPAIR_CONTROLLER_INVALID, bda, commonConfiguration);
}

IOSError Bloopair_ApplyControllerConfigurationForControllerType(IOSHandle handle, BloopairControllerType controllerType, const BloopairCommonConfiguration* commonConfiguration)
{
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_ApplyControllerConfiguration(handle, controllerType, NULL, commonConfiguration);
}

static IOSError _Bloopair_ApplyControllerMapping(IOSHandle handle, BloopairControllerType controllerType, const uint8_t* bda, const BloopairMappingEntry* mappings, uint8_t numMappings)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairApplyControllerConfigurationData* configData = (BloopairApplyControllerConfigurationData*) ioctlv->request.data;
    configData->controllerType = controllerType;
    if (bda) {
        memcpy(configData->bd_address, bda, 6);
    } else {
        memset(configData->bd_address, 0, 6);
    }

    if (mappings) {
        configData->dataSize = numMappings * sizeof(*mappings);
        memcpy(configData->data, mappings, configData->dataSize);
    } else {
        configData->dataSize = 0;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_ApplyControllerMappingForBDA(IOSHandle handle, const uint8_t* bda, const BloopairMappingEntry* mappings, uint8_t numMappings)
{
    if (!bda) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_ApplyControllerMapping(handle, BLOOPAIR_CONTROLLER_INVALID, bda, mappings, numMappings);
}

IOSError Bloopair_ApplyControllerMappingForControllerType(IOSHandle handle, BloopairControllerType controllerType, const BloopairMappingEntry* mappings, uint8_t numMappings)
{
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_ApplyControllerMapping(handle, controllerType, NULL, mappings, numMappings);
}

static IOSError _Bloopair_ApplyCustomConfiguration(IOSHandle handle, BloopairControllerType controllerType, const uint8_t* bda, const void* customConfiguration, uint32_t size)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairApplyControllerConfigurationData* configData = (BloopairApplyControllerConfigurationData*) ioctlv->request.data;
    configData->controllerType = controllerType;
    if (bda) {
        memcpy(configData->bd_address, bda, 6);
    } else {
        memset(configData->bd_address, 0, 6);
    }

    if (customConfiguration) {
        configData->dataSize = size;
        memcpy(configData->data, customConfiguration, configData->dataSize);
    } else {
        configData->dataSize = 0;
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_ApplyCustomConfigurationForBDA(IOSHandle handle, const uint8_t* bda, const void* customConfiguration, uint32_t size)
{
    if (!bda) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_ApplyCustomConfiguration(handle, BLOOPAIR_CONTROLLER_INVALID, bda, customConfiguration, size);
}

IOSError Bloopair_ApplyCustomConfigurationForControllerType(IOSHandle handle, BloopairControllerType controllerType, const void* customConfiguration, uint32_t size)
{
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_ApplyCustomConfiguration(handle, controllerType, NULL, customConfiguration, size);
}

static IOSError _Bloopair_GetControllerConfiguration(IOSHandle handle, BloopairControllerType controllerType, WPADChan chan, BloopairCommonConfiguration* outConfiguration)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairControllerRequestData* request = (BloopairControllerRequestData*) ioctlv->request.data;
    request->controllerType = controllerType;
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        request->handle = getHandleForChannel(chan);
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        if (res != sizeof(*outConfiguration)) {
            return IOS_ERROR_INVALIDSIZE;
        }

        memcpy(outConfiguration, ioctlv->response.data, res);
        res = IOS_ERROR_OK;
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_GetControllerConfiguration(IOSHandle handle, WPADChan chan, BloopairCommonConfiguration* outConfiguration)
{
    return _Bloopair_GetControllerConfiguration(handle, BLOOPAIR_CONTROLLER_INVALID, chan, outConfiguration);
}

IOSError Bloopair_GetDefaultControllerConfiguration(IOSHandle handle, BloopairControllerType controllerType, BloopairCommonConfiguration* outConfiguration)
{
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_GetControllerConfiguration(handle, controllerType, WPAD_CHAN_0, outConfiguration);
}

static IOSError _Bloopair_GetControllerMapping(IOSHandle handle, BloopairControllerType controllerType, WPADChan chan, BloopairMappingEntry* mappings, uint8_t* numMappings)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairControllerRequestData* request = (BloopairControllerRequestData*) ioctlv->request.data;
    request->controllerType = controllerType;
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        request->handle = getHandleForChannel(chan);
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        if (numMappings) {
            if (mappings && *numMappings >= res) {
                memcpy(mappings, ioctlv->response.data, sizeof(*mappings) * res);
            }

            *numMappings = res;
        }

        res = IOS_ERROR_OK;
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_GetControllerMapping(IOSHandle handle, WPADChan chan, BloopairMappingEntry* outMappings, uint8_t* outNumMappings)
{
    return _Bloopair_GetControllerMapping(handle, BLOOPAIR_CONTROLLER_INVALID, chan, outMappings, outNumMappings);
}

IOSError Bloopair_GetDefaultControllerMapping(IOSHandle handle, BloopairControllerType controllerType, BloopairMappingEntry* outMappings, uint8_t* outNumMappings)
{
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_GetControllerMapping(handle, controllerType, WPAD_CHAN_0, outMappings, outNumMappings);
}

static IOSError _Bloopair_GetCustomConfiguration(IOSHandle handle, BloopairControllerType controllerType, WPADChan chan, void* outCustom, uint32_t* outSize)
{
    BtrmIoctlv* ioctlv = allocBtrmIoctlv(BLOOPAIR_LIB, BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION);
    if (!ioctlv) {
        return IOS_ERROR_FAILALLOC;
    }

    BloopairControllerRequestData* request = (BloopairControllerRequestData*) ioctlv->request.data;
    request->controllerType = controllerType;
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        request->handle = getHandleForChannel(chan);
    }

    IOSError res = executeBtrmIoctlv(handle, ioctlv);
    if (res >= 0) {
        if (outSize) {
            if (outCustom && *outSize >= res) {
                memcpy(outCustom, ioctlv->response.data, res);
            }

            *outSize = res;
        }

        res = IOS_ERROR_OK;
    }

    freeBtrmIoctlv(ioctlv);

    return res;
}

IOSError Bloopair_GetCustomConfiguration(IOSHandle handle, WPADChan chan, void* outCustom, uint32_t* outSize)
{
    return _Bloopair_GetCustomConfiguration(handle, BLOOPAIR_CONTROLLER_INVALID, chan, outCustom, outSize);
}

IOSError Bloopair_GetDefaultCustomConfiguration(IOSHandle handle, BloopairControllerType controllerType, void* outCustom, uint32_t* outSize)
{
    if (controllerType == BLOOPAIR_CONTROLLER_INVALID) {
        return IOS_ERROR_INVALIDARG;
    }

    return _Bloopair_GetCustomConfiguration(handle, controllerType, WPAD_CHAN_0, outCustom, outSize);
}
