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

#include <string>
#include <array>
#include <optional>
#include <span>
#include <bloopair/controllers/common.h>
#include <bloopair/controllers/dualsense_controller.h>
#include <bloopair/controllers/dualshock3_controller.h>
#include <bloopair/controllers/dualshock4_controller.h>
#include <bloopair/controllers/switch_controller.h>
#include <bloopair/controllers/xbox_one_controller.h>
#include <bloopair/ipc.h>
#include <padscore/kpad.h>

namespace BloopairIPC
{

template <typename T>
concept ConfigurationType = std::same_as<T, DualsenseConfiguration> ||
                            std::same_as<T, Dualshock3Configuration> ||
                            std::same_as<T, Dualshock4Configuration> ||
                            std::same_as<T, SwitchConfiguration> ||
                            std::same_as<T, XboxOneConfiguration>;

namespace detail
{

bool ApplyCustomConfiguration(const uint8_t* bda, const std::span<const std::byte>& configuration);

bool ApplyCustomConfiguration(BloopairControllerType type, const std::span<const std::byte>& configuration);

bool GetCustomConfiguration(KPADChan chan, const std::span<std::byte>& configuration);

};

bool Init();

bool Shutdown();

bool IsActive();

uint32_t GetVersion();

std::string GetCommitHash();

std::optional<std::array<uint8_t, 6>> ReadConsoleBDA();

bool AddControllerPairing(const uint8_t* bda, const uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid);

bool GetControllerInformation(KPADChan chan, BloopairControllerInformationData& outData);

bool ReadRawReport(KPADChan chan, BloopairReportBuffer& outReport);

bool ApplyConfiguration(const uint8_t* bda, const BloopairCommonConfiguration& configuration);

bool ApplyConfiguration(BloopairControllerType type, const BloopairCommonConfiguration& configuration);

bool ClearConfiguration(const uint8_t* bda);

bool GetConfiguration(KPADChan chan, BloopairCommonConfiguration& outConfiguration);

bool ApplyControllerMapping(const uint8_t* bda, const BloopairMappingEntry* mappings, uint8_t numMappings);

bool ApplyControllerMapping(BloopairControllerType type, const BloopairMappingEntry* mappings, uint8_t numMappings);

bool GetControllerMapping(KPADChan chan, BloopairMappingEntry* outEntries, uint8_t* outNumMappings);

template <ConfigurationType T>
bool ApplyCustomConfiguration(const uint8_t* bda, const T& configuration)
{
    return detail::ApplyCustomConfiguration(bda, std::as_bytes(std::span{std::addressof(configuration), 1}));
}

template <ConfigurationType T>
bool ApplyCustomConfiguration(BloopairControllerType type, const T& configuration)
{
    return detail::ApplyCustomConfiguration(type, std::as_bytes(std::span{std::addressof(configuration), 1}));
}

bool ClearCustomConfiguration(const uint8_t* bda);

template <ConfigurationType T>
bool GetCustomConfiguration(KPADChan chan, T& configuration)
{
    return detail::GetCustomConfiguration(chan, std::as_writable_bytes(std::span{std::addressof(configuration), 1}));
}

}
