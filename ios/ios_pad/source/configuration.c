/*
 *   Copyright (C) 2024 GaryOderNichts
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
#include "configuration.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

void controllerModuleInit_switch(void);
void controllerModuleInit_xbox_one(void);
void controllerModuleInit_dualsense(void);
void controllerModuleInit_dualshock4(void);
void controllerModuleInit_dualshock3(void);

static int configuration_initialized = 0;

static ConfigurationEntry* first_fallback_entry = NULL;
static ConfigurationEntry* first_controller_type_entry = NULL;
static ConfigurationEntry* first_bda_entry = NULL;

static const BloopairCommonConfiguration default_common_configuration = {
    .stickAsButtonDeadzone = 500,
};

int Configuration_Init(void)
{
    if (configuration_initialized) {
        return 0;
    }
    configuration_initialized = 1;

    controllerModuleInit_switch();
    controllerModuleInit_xbox_one();
    controllerModuleInit_dualsense();
    controllerModuleInit_dualshock4();
    controllerModuleInit_dualshock3();

    return 0;
}

void Configuration_Deinit(void)
{
    if (!configuration_initialized) {
        return;
    }

    // TODO free entries

    configuration_initialized = 0;
}

static ConfigurationEntry* _Configuration_AddNew(ConfigurationEntry** firstPtr)
{
    ConfigurationEntry* entry = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    memset(entry, 0, sizeof(*entry));

    entry->next = *firstPtr;
    *firstPtr = entry;

    return entry;
}

ConfigurationEntry* Configuration_GetFallback(BloopairControllerType type, uint8_t allocateIfMissing)
{
    ConfigurationEntry* entry;

    // Search for a matching entry
    for (entry = first_fallback_entry; entry; entry = entry->next) {
        if (entry->filter.type == type) {
            break;
        }
    }

    // Allocate a new one if it doesn't exist
    if (!entry && allocateIfMissing) {
        entry = _Configuration_AddNew(&first_fallback_entry);
        if (entry) {
            entry->filter.type = type;
        }
    }

    return entry;
}

ConfigurationEntry* Configuration_GetForControllerType(BloopairControllerType type, uint8_t allocateIfMissing)
{
    ConfigurationEntry* entry;

    // Search for a matching entry
    for (entry = first_controller_type_entry; entry; entry = entry->next) {
        if (entry->filter.type == type) {
            break;
        }
    }

    // Allocate a new one if it doesn't exist
    if (!entry && allocateIfMissing) {
        entry = _Configuration_AddNew(&first_controller_type_entry);
        if (entry) {
            entry->filter.type = type;
        }
    }

    return entry;
}

ConfigurationEntry* Configuration_GetForBDA(uint8_t* bda, uint8_t allocateIfMissing)
{
    ConfigurationEntry* entry;

    // Search for a matching entry
    for (entry = first_bda_entry; entry; entry = entry->next) {
        if (memcmp(entry->filter.bda, bda, sizeof(entry->filter.bda)) == 0) {
            break;
        }
    }

    // Allocate a new one if it doesn't exist
    if (!entry && allocateIfMissing) {
        entry = _Configuration_AddNew(&first_bda_entry);
        if (entry) {
            memcpy(entry->filter.bda, bda, sizeof(entry->filter.bda));
        }
    }

    return entry;
}

BloopairCommonConfiguration* Configuration_GetCommon(BloopairControllerType type, uint8_t* bda)
{
    ConfigurationEntry* entry;

    // First see if we have a BDA config override
    if (bda) {
        entry = Configuration_GetForBDA(bda, 0);
        if (entry && entry->common) {
            return entry->common;
        }
    }

    // Next see if there's an override for this controller type
    entry = Configuration_GetForControllerType(type, 0);
    if (entry && entry->common) {
        return entry->common;
    }

    // Go for the default fallback if nothing else found
    entry = Configuration_GetFallback(type, 0);
    if (entry && entry->common) {
        return entry->common;
    }

    // We have an additional common fallback
    // TODO don't cast away the const here
    return (BloopairCommonConfiguration*) &default_common_configuration;
}

MappingConfiguration* Configuration_GetMapping(BloopairControllerType type, uint8_t* bda)
{
    ConfigurationEntry* entry;

    if (bda) { 
        // First see if we have a BDA config override
        entry = Configuration_GetForBDA(bda, 0);
        if (entry && entry->mapping) {
            return entry->mapping;
        }
    }

    // Next see if there's an override for this controller type
    entry = Configuration_GetForControllerType(type, 0);
    if (entry && entry->mapping) {
        return entry->mapping;
    }

    // Go for the default fallback if nothing else found
    entry = Configuration_GetFallback(type, 0);
    if (entry && entry->mapping) {
        return entry->mapping;
    }

    return NULL;
}

void* Configuration_GetCustom(BloopairControllerType type, uint8_t* bda, uint32_t* outSize)
{
    ConfigurationEntry* entry;

    if (bda) {
        // First see if we have a BDA config override
        entry = Configuration_GetForBDA(bda, 0);
        if (entry && entry->custom) {
            *outSize = entry->customSize;
            return entry->custom;
        }
    }

    // Next see if there's an override for this controller type
    entry = Configuration_GetForControllerType(type, 0);
    if (entry && entry->custom) {
        return entry->custom;
    }

    // Go for the default fallback if nothing else found
    entry = Configuration_GetFallback(type, 0);
    if (entry && entry->custom) {
        *outSize = entry->customSize;
        return entry->custom;
    }

    return NULL;
}

int Configuration_GetAll(BloopairControllerType type, uint8_t* bda, BloopairCommonConfiguration** outCommon, MappingConfiguration** outMapping, void** outCustom, uint32_t* outCustomSize)
{
    // TODO this can be improved further so we don't have to iterate over the configuration multiple times

    BloopairCommonConfiguration* common = Configuration_GetCommon(type, bda);
    if (!common) {
        return -1;
    }

    MappingConfiguration* mapping = Configuration_GetMapping(type, bda);
    if (!mapping) {
        return -1;
    }

    uint32_t customSize = 0;
    void* custom = Configuration_GetCustom(type, bda, &customSize);
    if (!custom) {
        // Not an error, the controller modules themself should check if there's a custom configuration or not
    }

    *outCommon = common;
    *outMapping = mapping;
    *outCustom = custom;
    *outCustomSize = customSize;
    return 0;
}

void Configuration_SetFallback(BloopairControllerType type, const BloopairCommonConfiguration* common, const MappingConfiguration* mapping, const void* custom, uint32_t customSize)
{
    ConfigurationEntry* entry = Configuration_GetFallback(type, 1);
    if (entry) {
        // Meh casting const away isn't nice, but will keep it like this for now
        entry->common = (BloopairCommonConfiguration*) common;
        entry->mapping = (MappingConfiguration*) mapping;
        entry->custom = (void*) custom;
        entry->customSize = customSize;
    }
}
