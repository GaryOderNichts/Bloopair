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
#include "config.hpp"

#include <map>
#include <string>
#include <fstream>
#include <filesystem>
#include <regex>

#include <coreinit/debug.h>

#include <bloopair/bloopair.h>
#include <bloopair/controllers/dualsense_controller.h>
#include <bloopair/controllers/dualshock3_controller.h>
#include <bloopair/controllers/dualshock4_controller.h>
#include <bloopair/controllers/switch_controller.h>
#include <bloopair/controllers/xbox_one_controller.h>
#include <json/json.hpp>

#define BLOOPAIR_CONFIGURATION_DIR "/vol/external01/wiiu/bloopair/"

// Bump up the versions by 100 for breaking config changes
#define BLOOPAIR_CONFIG_VERSION_MIN 0
#define BLOOPAIR_CONFIG_VERSION_MAX 100

static const std::map<std::string, uint32_t> bloopairButtonNameValues = {
    { "up",         BLOOPAIR_PRO_BUTTON_UP },
    { "left",       BLOOPAIR_PRO_BUTTON_LEFT },
    { "zr",         BLOOPAIR_PRO_TRIGGER_ZR },
    { "x",          BLOOPAIR_PRO_BUTTON_X },
    { "a",          BLOOPAIR_PRO_BUTTON_A },
    { "y",          BLOOPAIR_PRO_BUTTON_Y },
    { "b",          BLOOPAIR_PRO_BUTTON_B },
    { "zl",         BLOOPAIR_PRO_TRIGGER_ZL },
    { "reserved",   BLOOPAIR_PRO_RESERVED },
    { "r",          BLOOPAIR_PRO_TRIGGER_R },
    { "plus",       BLOOPAIR_PRO_BUTTON_PLUS },
    { "home",       BLOOPAIR_PRO_BUTTON_HOME },
    { "minus",      BLOOPAIR_PRO_BUTTON_MINUS },
    { "l",          BLOOPAIR_PRO_TRIGGER_L },
    { "down",       BLOOPAIR_PRO_BUTTON_DOWN },
    { "right",      BLOOPAIR_PRO_BUTTON_RIGHT },
    { "rstick",     BLOOPAIR_PRO_BUTTON_STICK_R },
    { "lstick",     BLOOPAIR_PRO_BUTTON_STICK_L },
    { "lup",        BLOOPAIR_PRO_STICK_L_UP },
    { "ldown",      BLOOPAIR_PRO_STICK_L_DOWN },
    { "lleft",      BLOOPAIR_PRO_STICK_L_LEFT },
    { "lright",     BLOOPAIR_PRO_STICK_L_RIGHT },
    { "rrup",       BLOOPAIR_PRO_STICK_R_UP },
    { "rdown",      BLOOPAIR_PRO_STICK_R_DOWN },
    { "rleft",      BLOOPAIR_PRO_STICK_R_LEFT },
    { "rright",     BLOOPAIR_PRO_STICK_R_RIGHT },
};

static const std::map<std::string, BloopairControllerType> bloopairControllerTypeValues = {
    { "DualSense",              BLOOPAIR_CONTROLLER_DUALSENSE },
    { "DualShock-3",            BLOOPAIR_CONTROLLER_DUALSHOCK3 },
    { "DualShock-4",            BLOOPAIR_CONTROLLER_DUALSHOCK4 },
    { "Switch-Generic",         BLOOPAIR_CONTROLLER_SWITCH_GENERIC },
    { "Switch-JoyCon-Left",     BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT },
    { "Switch-JoyCon-Right",    BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT },
    { "Switch-JoyCon-Dual",     BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL },
    { "Switch-Pro",             BLOOPAIR_CONTROLLER_SWITCH_PRO },
    { "Switch-N64",             BLOOPAIR_CONTROLLER_SWITCH_N64 },
    { "Xbox-One",               BLOOPAIR_CONTROLLER_XBOX_ONE },
};

static bool LoadCommonConfiguration(const nlohmann::json& common, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    // Start by getting the default configuration
    BloopairCommonConfiguration configuration;
    if (Bloopair_GetDefaultControllerConfiguration(handle, type, &configuration) < 0) {
        return false;
    }

    // Overwrite fields from the config
    if (common.contains("stickAsButtonDeadzone")) {
        configuration.stickAsButtonDeadzone = common["stickAsButtonDeadzone"];
    }

    // Apply configuration
    IOSError error;
    if (bda) {
        error = Bloopair_ApplyControllerConfigurationForBDA(handle, bda, &configuration);
    } else {
        error = Bloopair_ApplyControllerConfigurationForControllerType(handle, type, &configuration);
    }

    if (error < 0) {
        OSReport("Bloopair Loader: ApplyControllerConfiguration failed %x\n", error);
        return false;
    }

    return true;
}

static bool LoadControllerMapping(const nlohmann::json& mapping, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    std::vector<BloopairMappingEntry> mappings;
    for (const auto& [key, val] : mapping.items()) {
        if (!bloopairButtonNameValues.contains(key)) {
            OSReport("Bloopair Loader: Ignoring unknown button %s\n", key.c_str());
            continue;
        }

        uint8_t button = bloopairButtonNameValues.at(key);
        for (const auto& ent : val) {
            mappings.push_back(BloopairMappingEntry{ent.get<uint8_t>(), button});
        }
    }

    IOSError error;
    if (bda) {
        error = Bloopair_ApplyControllerMappingForBDA(handle, bda, mappings.data(), mappings.size());
    } else {
        error = Bloopair_ApplyControllerMappingForControllerType(handle, type, mappings.data(), mappings.size());
    }

    if (error < 0) {
        OSReport("Bloopair Loader: ApplyControllerMapping failed %x\n", error);
        return false;
    }

    return true;
}

static bool LoadDualSenseCustomConfiguration(const nlohmann::json& custom, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    return true;
}

static bool LoadDualShock3CustomConfiguration(const nlohmann::json& custom, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    // Start by getting the default configuration
    Dualshock3Configuration config;
    uint32_t configSize = sizeof(config);
    if (Bloopair_GetDefaultCustomConfiguration(handle, type, &config, &configSize) < 0) {
        return false;
    }

    // Overwrite fields from the config
    if (custom.contains("motorForce")) {
        config.motorForce = custom["motorForce"];
    }
    if (custom.contains("motorDuration")) {
        config.motorDuration = custom["motorDuration"];
    }

    // Apply configuration
    IOSError error;
    if (bda) {
        error = Bloopair_ApplyCustomConfigurationForBDA(handle, bda, &config, sizeof(config));
    } else {
        error = Bloopair_ApplyCustomConfigurationForControllerType(handle, type, &config, sizeof(config));
    }

    if (error < 0) {
        OSReport("Bloopair Loader: ApplyCustomConfiguration failed %x\n", error);
        return false;
    }

    return true;
}

static bool LoadDualShock4CustomConfiguration(const nlohmann::json& custom, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    return true;
}

static bool LoadSwitchCustomConfiguration(const nlohmann::json& custom, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    // Start by getting the default configuration
    SwitchConfiguration config;
    uint32_t configSize = sizeof(config);
    if (Bloopair_GetDefaultCustomConfiguration(handle, type, &config, &configSize) < 0) {
        return false;
    }

    // Overwrite fields from the config
    if (custom.contains("disableCalibration")) {
        config.disableCalibration = custom["disableCalibration"];
    }

    // Apply configuration
    IOSError error;
    if (bda) {
        error = Bloopair_ApplyCustomConfigurationForBDA(handle, bda, &config, sizeof(config));
    } else {
        error = Bloopair_ApplyCustomConfigurationForControllerType(handle, type, &config, sizeof(config));
    }

    if (error < 0) {
        OSReport("Bloopair Loader: ApplyCustomConfiguration failed %x\n", error);
        return false;
    }

    return true;
}

static bool LoadXboxOneCustomConfiguration(const nlohmann::json& custom, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    return true;
}

static bool LoadCustomConfiguration(const nlohmann::json& custom, IOSHandle handle, BloopairControllerType type, const uint8_t* bda)
{
    switch (type) {
        case BLOOPAIR_CONTROLLER_DUALSENSE:
            return LoadDualSenseCustomConfiguration(custom, handle, type, bda);
        case BLOOPAIR_CONTROLLER_DUALSHOCK3:
            return LoadDualShock3CustomConfiguration(custom, handle, type, bda);
        case BLOOPAIR_CONTROLLER_DUALSHOCK4:
            return LoadDualShock4CustomConfiguration(custom, handle, type, bda);
        case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
        case BLOOPAIR_CONTROLLER_SWITCH_PRO:
        case BLOOPAIR_CONTROLLER_SWITCH_N64:
            return LoadSwitchCustomConfiguration(custom, handle, type, bda);
        case BLOOPAIR_CONTROLLER_XBOX_ONE:
            return LoadXboxOneCustomConfiguration(custom, handle, type, bda);
        default: break;
    }

    return false;
}

static bool LoadAndApplySingleConfiguration(const std::filesystem::path& path, BloopairControllerType nameType, const uint8_t* bda, IOSHandle handle)
{
    nlohmann::json config = nlohmann::json::parse(std::ifstream(path), nullptr, false);
    if (config.is_discarded()) {
        OSReport("Bloopair Loader: Invalid json\n");
        return false;
    }

    // Version check
    if (config.contains("version") &&
       (config["version"].get<uint32_t>() < BLOOPAIR_CONFIG_VERSION_MIN ||
        config["version"].get<uint32_t>() > BLOOPAIR_CONFIG_VERSION_MAX)) {
        OSReport("Bloopair Loader: Unsupported version\n");
        return false;
    }

    // Controller type check
    BloopairControllerType controllerType = BLOOPAIR_CONTROLLER_INVALID;
    if (config.contains("controllerType")) {
        std::string typeString = config["controllerType"].get<std::string>();
        if (bloopairControllerTypeValues.contains(typeString)) {
            controllerType = bloopairControllerTypeValues.at(typeString);
        } else {
            OSReport("Bloopair Loader: Configuration contains invalid controller type\n");
            return false;
        }
    } else {
        OSReport("Bloopair Loader: Configuration is missing controller type\n");
        return false;
    }

    if (nameType != BLOOPAIR_CONTROLLER_INVALID && controllerType != nameType) {
        OSReport("Bloopair Loader: Configuration name doesn't match controller type in content\n");
        return false;
    }

    if (config.contains("configuration")) {
        if (!LoadCommonConfiguration(config["configuration"], handle, controllerType, bda)) {
            OSReport("Bloopair Loader: Failed to load common configuration\n");
        }
    }

    if (config.contains("mapping")) {
        if (!LoadControllerMapping(config["mapping"], handle, controllerType, bda)) {
            OSReport("Bloopair Loader: Failed to load controller mapping\n");
        }
    }

    if (config.contains("custom")) {
        if (!LoadCustomConfiguration(config["custom"], handle, controllerType, bda)) {
            OSReport("Bloopair Loader: Failed to load custom configuration\n");
        }
    }

    return true;
}

bool HexToBDA(const std::string& hex, uint8_t* bda)
{
    // Need exactly 12 hex characters for a bda
    if (hex.size() != 12) {
        return false;
    }

    // Only support uppercase hex characters
    auto char2int = ([](char in) -> int {
        if(in >= '0' && in <= '9')
            return in - '0';
        if(in >= 'A' && in <= 'F')
            return in - 'A' + 10;
        return -1;
    });

    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = char2int(hex[i]);
        int lo = char2int(hex[i + 1]);
        if (hi == -1 || lo == -1) {
            return false;
        }

        *(bda++) = hi << 4 | lo;
    }

    return true;
}

bool LoadAndApplyBloopairConfiguration(IOSHandle handle)
{
    std::regex regex("^Controller-([A-Za-z0-9-]+)\\.conf$");

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(BLOOPAIR_CONFIGURATION_DIR, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string filename = entry.path().filename().string();
        std::smatch m;
        if (!std::regex_search(filename, m, regex)) {
            OSReport("Bloopair Loader: %s doesn't match filename pattern\n", filename.c_str());
            continue;
        }

        BloopairControllerType type = BLOOPAIR_CONTROLLER_INVALID;
        uint8_t bda[6];
        if (bloopairControllerTypeValues.contains(m.str(1))) {
            type = bloopairControllerTypeValues.at(m.str(1));
        } else if (!HexToBDA(m.str(1), bda)) {
            OSReport("Bloopair Loader: %s has an invalid filename\n", filename.c_str());
            continue;
        }

        if (!LoadAndApplySingleConfiguration(entry.path(), type, type ? nullptr : bda, handle)) {
            OSReport("Bloopair Loader: Failed to load %s\n", filename.c_str());
            continue;
        }
    }

    return true;
}
