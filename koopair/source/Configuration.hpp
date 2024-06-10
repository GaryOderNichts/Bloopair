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

#include <vector>
#include <filesystem>
#include <json/json.hpp>
#include <bloopair/controllers/common.h>
#include <bloopair/controllers/dualsense_controller.h>
#include <bloopair/controllers/dualshock3_controller.h>
#include <bloopair/controllers/dualshock4_controller.h>
#include <bloopair/controllers/switch_controller.h>
#include <bloopair/controllers/xbox_one_controller.h>

class Configuration {
public:
    Configuration(const std::filesystem::path& path, BloopairControllerType type);
    Configuration(BloopairControllerType type);
    Configuration(const uint8_t* bda, BloopairControllerType type);
    virtual ~Configuration();

    static std::vector<Configuration> LoadAll();

    bool Save();

    void SetCommonConfiguration(const BloopairCommonConfiguration& config);

    void SetMappings(const std::vector<BloopairMappingEntry>& mappings);

    void SetCustomConfiguraion(const DualsenseConfiguration& config);
    void SetCustomConfiguraion(const Dualshock3Configuration& config);
    void SetCustomConfiguraion(const Dualshock4Configuration& config);
    void SetCustomConfiguraion(const SwitchConfiguration& config);
    void SetCustomConfiguraion(const XboxOneConfiguration& config);

    void Remove();
    static void Remove(BloopairControllerType type);
    static void Remove(const uint8_t* bda);

    std::string GetFilename() const;

private:
    bool InitConfiguration();
    bool LoadConfiguration();

    std::filesystem::path mPath;
    nlohmann::ordered_json mJson;
    BloopairControllerType mControllerType;
};
