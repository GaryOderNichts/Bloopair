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

// TODO template this
bool ApplyCustomConfiguration(const uint8_t* bda, const DualsenseConfiguration& configuration);
bool ApplyCustomConfiguration(const uint8_t* bda, const Dualshock3Configuration& configuration);
bool ApplyCustomConfiguration(const uint8_t* bda, const Dualshock4Configuration& configuration);
bool ApplyCustomConfiguration(const uint8_t* bda, const SwitchConfiguration& configuration);
bool ApplyCustomConfiguration(const uint8_t* bda, const XboxOneConfiguration& configuration);
bool ApplyCustomConfiguration(BloopairControllerType type, const DualsenseConfiguration& configuration);
bool ApplyCustomConfiguration(BloopairControllerType type, const Dualshock3Configuration& configuration);
bool ApplyCustomConfiguration(BloopairControllerType type, const Dualshock4Configuration& configuration);
bool ApplyCustomConfiguration(BloopairControllerType type, const SwitchConfiguration& configuration);
bool ApplyCustomConfiguration(BloopairControllerType type, const XboxOneConfiguration& configuration);

bool ClearCustomConfiguration(const uint8_t* bda);

// TODO template this
bool GetCustomConfiguration(KPADChan chan, DualsenseConfiguration& configuration);
bool GetCustomConfiguration(KPADChan chan, Dualshock3Configuration& configuration);
bool GetCustomConfiguration(KPADChan chan, Dualshock4Configuration& configuration);
bool GetCustomConfiguration(KPADChan chan, SwitchConfiguration& configuration);
bool GetCustomConfiguration(KPADChan chan, XboxOneConfiguration& configuration);

}
