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
#include "BloopairIPC.hpp"
#include <bloopair/bloopair.h>

namespace
{

IOSHandle bloopairHandle = -1;

};

namespace BloopairIPC
{

bool Init()
{
    if (bloopairHandle >= 0) {
        return true;
    }

    bloopairHandle = Bloopair_Open();
    if (bloopairHandle < 0) {
        return false;
    }

    return true;
}

bool Shutdown()
{
    if (bloopairHandle < 0) {
        return true;
    }

    Bloopair_Close(bloopairHandle);
    bloopairHandle = -1;
    return true;
}

bool IsActive()
{
    return Bloopair_IsActive(bloopairHandle);
}

uint32_t GetVersion()
{
    int32_t ver = Bloopair_GetVersion(bloopairHandle);
    if (ver < 0) {
        return 0;
    }

    return static_cast<uint32_t>(ver);
}

std::string GetCommitHash()
{
    char hash[41]; // 40 hex characters + 1 null terminator
    if (Bloopair_GetCommitHash(bloopairHandle, hash, sizeof(hash)) < 0) {
        return std::string();
    }

    return std::string(hash);
}

std::optional<std::array<uint8_t, 6>> ReadConsoleBDA()
{
    std::array<uint8_t, 6> bda;
    if (Bloopair_ReadConsoleBDA(bloopairHandle, bda.data()) < 0) {
        return {};
    }

    return bda;
}

bool AddControllerPairing(const uint8_t* bda, const uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid)
{
    return Bloopair_AddControllerPairing(bloopairHandle, bda, link_key, name, vid, pid) >= 0;
}

bool GetControllerInformation(KPADChan chan, BloopairControllerInformationData& outData)
{
    return Bloopair_GetControllerInformation(bloopairHandle, (WPADChan) chan, &outData) >= 0;
}

bool ReadRawReport(KPADChan chan, BloopairReportBuffer& outReport)
{
    return Bloopair_ReadRawReport(bloopairHandle, (WPADChan) chan, &outReport) >= 0;
}

bool ApplyConfiguration(const uint8_t* bda, const BloopairCommonConfiguration& configuration)
{
    return Bloopair_ApplyControllerConfigurationForBDA(bloopairHandle, bda, &configuration) >= 0;
}

bool ApplyConfiguration(BloopairControllerType type, const BloopairCommonConfiguration& configuration)
{
    return Bloopair_ApplyControllerConfigurationForControllerType(bloopairHandle, type, &configuration) >= 0;
}

bool ClearConfiguration(const uint8_t* bda)
{
    return Bloopair_ApplyControllerConfigurationForBDA(bloopairHandle, bda, nullptr) >= 0;
}

bool GetConfiguration(KPADChan chan, BloopairCommonConfiguration& outConfiguration)
{
    return Bloopair_GetControllerConfiguration(bloopairHandle, chan, &outConfiguration) >= 0;
}

bool ApplyControllerMapping(const uint8_t* bda, const BloopairMappingEntry* mappings, uint8_t numMappings)
{
    return Bloopair_ApplyControllerMappingForBDA(bloopairHandle, bda, mappings, numMappings) >= 0;
}

bool ApplyControllerMapping(BloopairControllerType type, const BloopairMappingEntry* mappings, uint8_t numMappings)
{
    return Bloopair_ApplyControllerMappingForControllerType(bloopairHandle, type, mappings, numMappings) >= 0;
}

bool GetControllerMapping(KPADChan chan, BloopairMappingEntry* outEntries, uint8_t* outNumMappings)
{
    return Bloopair_GetControllerMapping(bloopairHandle, chan, outEntries, outNumMappings) >= 0;
}

bool ClearCustomConfiguration(const uint8_t* bda)
{
    return Bloopair_ApplyCustomConfigurationForBDA(bloopairHandle, bda, nullptr, 0) >= 0;
}

namespace detail
{

bool ApplyCustomConfiguration(const uint8_t* bda, const std::span<const std::byte>& configuration)
{
    return Bloopair_ApplyCustomConfigurationForBDA(bloopairHandle, bda, reinterpret_cast<const uint8_t*>(configuration.data()), configuration.size()) >= 0;
}

bool ApplyCustomConfiguration(BloopairControllerType type, const std::span<const std::byte>& configuration)
{
    return Bloopair_ApplyCustomConfigurationForControllerType(bloopairHandle, type, reinterpret_cast<const uint8_t*>(configuration.data()), configuration.size()) >= 0;
}

bool GetCustomConfiguration(KPADChan chan, const std::span<std::byte>& configuration)
{
    uint32_t maxSize = configuration.size();
    return Bloopair_GetCustomConfiguration(bloopairHandle, chan, reinterpret_cast<uint8_t*>(configuration.data()), &maxSize) >= 0;
}

};

};
