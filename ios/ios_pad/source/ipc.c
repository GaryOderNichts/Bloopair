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
#include "controllers.h"
#include <bloopair/ipc.h>

static int bloopairFunc(BtrmRequest* request, BtrmResponse* response)
{
    // ensure config is initialized
    Configuration_Init();

    switch (request->func) {
    case BLOOPAIR_FUNC_GET_VERSION:
        DEBUG_PRINT("BLOOPAIR_FUNC_GET_VERSION\n");
        return BLOOPAIR_VERSION(1, 0, 3);

    case BLOOPAIR_FUNC_READ_CONSOLE_BDADDR: {
        DEBUG_PRINT("BLOOPAIR_FUNC_READ_CONSOLE_BDADDR\n");
        memcpy(response->data, local_device_bdaddr, 6);
        return 0;
    }
    
    case BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING: {
        DEBUG_PRINT("BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING\n");

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
                return -8;
            }
        }

        info->magic = MAGIC_BLOOPAIR;
        info->product_id = data->product_id;
        info->vendor_id = data->vendor_id;
        
        return 0;
    }
    
    case BLOOPAIR_FUNC_GET_COMMIT_HASH: {
        DEBUG_PRINT("BLOOPAIR_FUNC_GET_COMMIT_HASH\n");

#ifdef NDEBUG
        return -4;
#else
        strncpy((char*) response->data, COMMIT_HASH, sizeof(response->data));
        return 0;
#endif
    }

    case BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION: {
        DEBUG_PRINT("BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION\n");
    
        BloopairControllerRequestData* req = (BloopairControllerRequestData*) request->data;
        BloopairControllerInformationData* resp = (BloopairControllerInformationData*) response->data;
        if (req->handle > BTA_HH_MAX_KNOWN) {
            return -4;
        }

        Controller* controller = &controllers[req->handle];
        if (!controller->isInitialized) {
            return -4;
        }

        resp->controllerType = controller->type;
        resp->vendor_id = controller->vendor_id;
        resp->product_id = controller->product_id;

        return sizeof(*resp);
    }
    
    case BLOOPAIR_FUNC_READ_RAW_REPORT: {
        DEBUG_PRINT("BLOOPAIR_FUNC_READ_RAW_REPORT\n");

        BloopairControllerRequestData* req = (BloopairControllerRequestData*) request->data;
        if (req->handle > BTA_HH_MAX_KNOWN) {
            return -4;
        }

        Controller* controller = &controllers[req->handle];
        if (!controller->isInitialized) {
            return -4;
        }

        memcpy(response->data, &controller->reportBuffer, sizeof(controller->reportBuffer));

        return sizeof(controller->reportBuffer);
    }

    case BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG: {
        DEBUG_PRINT("BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG\n");
        BloopairApplyControllerConfigurationData* data = (BloopairApplyControllerConfigurationData*) request->data;
        if (data->dataSize != 0 && data->dataSize != sizeof(BloopairCommonConfiguration)) {
            return -4;
        }

        ConfigurationEntry* entry;
        if (data->controllerType != BLOOPAIR_CONTROLLER_INVALID) {
            entry = Configuration_GetForControllerType(data->controllerType, 1);
        } else {
            // use bda
            entry = Configuration_GetForBDA(data->bd_address, 1);
        }

        if (!entry) {
            return -4;
        }

        // If we already have an entry in here free it
        if (entry->common) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, entry->common);
            entry->common = NULL;
        }

        if (data->dataSize != 0) {
            // Allocate new configuration
            BloopairCommonConfiguration* configuration = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(BloopairCommonConfiguration));
            if (!configuration) {
                return -22;
            }

            memcpy(configuration, data->data, data->dataSize);
            entry->common = configuration;
        }

        return 0;
    }

    case BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING: {
        DEBUG_PRINT("BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING\n");

        BloopairApplyControllerConfigurationData* data = (BloopairApplyControllerConfigurationData*) request->data;
        if (data->dataSize % sizeof(BloopairMappingEntry) != 0) {
            return -4;
        }

        ConfigurationEntry* entry;
        if (data->controllerType != BLOOPAIR_CONTROLLER_INVALID) {
            entry = Configuration_GetForControllerType(data->controllerType, 1);
        } else {
            // use bda
            entry = Configuration_GetForBDA(data->bd_address, 1);
        }

        if (!entry) {
            return -4;
        }

        // If we already have an entry in here free it
        if (entry->mapping) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, entry->mapping);
            entry->mapping = NULL;
        }

        if (data->dataSize != 0) {
            // Allocate new mapping
            MappingConfiguration* mapping = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(MappingConfiguration) + data->dataSize);
            if (!mapping) {
                return -22;
            }

            mapping->num = data->dataSize / sizeof(BloopairMappingEntry);
            memcpy(mapping->mappings, data->data, data->dataSize);
            entry->mapping = mapping;
        }

        return 0;
    }

    case BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION: {
        DEBUG_PRINT("BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION\n");

        BloopairApplyControllerConfigurationData* data = (BloopairApplyControllerConfigurationData*) request->data;

        ConfigurationEntry* entry;
        if (data->controllerType != BLOOPAIR_CONTROLLER_INVALID) {
            entry = Configuration_GetForControllerType(data->controllerType, 1);
        } else {
            // use bda
            entry = Configuration_GetForBDA(data->bd_address, 1);
        }

        if (!entry) {
            return -4;
        }

        // If we already have an entry in here free it
        if (entry->custom) {
            IOS_Free(LOCAL_PROCESS_HEAP_ID, entry->custom);
            entry->custom = NULL;
        }

        if (data->dataSize != 0) {
            // Allocate new configuration
            void* configuration = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, data->dataSize);
            if (!configuration) {
                return -22;
            }

            memcpy(configuration, data->data, data->dataSize);
            entry->custom = configuration;
            entry->customSize = data->dataSize;
        }

        return 0;
    }

    case BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG: {
        DEBUG_PRINT("BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG\n");
        
        BloopairControllerRequestData* data = (BloopairControllerRequestData*) request->data;

        BloopairCommonConfiguration* configuration;
        if (data->controllerType != BLOOPAIR_CONTROLLER_INVALID) {
            configuration = Configuration_GetCommon(data->controllerType, NULL);
        } else {
            // use handle
            if (data->handle > BTA_HH_MAX_KNOWN) {
                return -4;
            }

            Controller* controller = &controllers[data->handle];
            if (!controller->isInitialized) {
                return -4;
            }

            configuration = controller->commonConfig;
        }

        if (!configuration) {
            return -4;
        }

        memcpy(response->data, configuration, sizeof(*configuration));
        return sizeof(*configuration);
    }

    case BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING: {
        DEBUG_PRINT("BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING\n");
        
        BloopairControllerRequestData* data = (BloopairControllerRequestData*) request->data;

        MappingConfiguration* mapping = NULL;
        if (data->controllerType != BLOOPAIR_CONTROLLER_INVALID) {
            mapping = Configuration_GetMapping(data->controllerType, NULL);
        } else {
            // use handle
            if (data->handle > BTA_HH_MAX_KNOWN) {
                return -4;
            }

            Controller* controller = &controllers[data->handle];
            if (!controller->isInitialized) {
                return -4;
            }

            mapping = controller->mapping;
        }

        if (!mapping) {
            return -4;
        }

        memcpy(response->data, mapping->mappings, mapping->num * sizeof(BloopairMappingEntry));
        return mapping->num;
    }

    case BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION: {
        DEBUG_PRINT("BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION\n");

        BloopairControllerRequestData* data = (BloopairControllerRequestData*) request->data;

        void* customConfiguration = NULL;
        uint32_t customSize = 0;
        if (data->controllerType != BLOOPAIR_CONTROLLER_INVALID) {
            customConfiguration = Configuration_GetCustom(data->controllerType, NULL, &customSize);
        } else {
            // use handle
            if (data->handle > BTA_HH_MAX_KNOWN) {
                return -4;
            }

            Controller* controller = &controllers[data->handle];
            if (!controller->isInitialized) {
                return -4;
            }

            customConfiguration = controller->customConfig;
            customSize = controller->customConfigSize;
        }

        if (!customConfiguration) {
            return -4;
        }

        if (customSize > sizeof(response->data)) {
            return -4;
        }

        memcpy(response->data, customConfiguration, customSize);
        return customSize;
    }

    }

    return -4;
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
