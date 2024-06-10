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
#pragma once

#include <stdint.h>
#include <bloopair/controllers/common.h>

typedef struct {
    uint8_t num;
    BloopairMappingEntry mappings[];
} MappingConfiguration;

typedef struct ConfigurationEntry {
    struct ConfigurationEntry* next;

    union {
        BloopairControllerType type;
        uint8_t bda[6];
    } filter;

    BloopairCommonConfiguration* common;
    MappingConfiguration* mapping;
    void* custom;
    uint32_t customSize;
} ConfigurationEntry;

int Configuration_Init(void);

void Configuration_Deinit(void);

ConfigurationEntry* Configuration_GetFallback(BloopairControllerType type, uint8_t allocateIfMissing);

ConfigurationEntry* Configuration_GetForControllerType(BloopairControllerType type, uint8_t allocateIfMissing);

ConfigurationEntry* Configuration_GetForBDA(uint8_t* bda, uint8_t allocateIfMissing);

BloopairCommonConfiguration* Configuration_GetCommon(BloopairControllerType type, uint8_t* bda);

MappingConfiguration* Configuration_GetMapping(BloopairControllerType type, uint8_t* bda);

void* Configuration_GetCustom(BloopairControllerType type, uint8_t* bda, uint32_t* outSize);

int Configuration_GetAll(BloopairControllerType type, uint8_t* bda, BloopairCommonConfiguration** outCommon, MappingConfiguration** outMapping, void** outCustom, uint32_t* outCustomSize);

void Configuration_SetFallback(BloopairControllerType type, const BloopairCommonConfiguration* common, const MappingConfiguration* mapping, const void* custom, uint32_t customSize);
