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
#include "Configuration.hpp"
#include "Utils.hpp"

#include <fstream>
#include <regex>

namespace
{

#define BLOOPAIR_CONFIGURATION_DIR "/vol/external01/wiiu/bloopair/"

// Bump up the versions by 100 for breaking config changes
#define BLOOPAIR_CONFIG_VERSION 0
#define BLOOPAIR_CONFIG_VERSION_MIN 0
#define BLOOPAIR_CONFIG_VERSION_MAX 100

const std::map<uint32_t, std::string> bloopairButtonNames = {
    { BLOOPAIR_PRO_BUTTON_UP,       "up" },
    { BLOOPAIR_PRO_BUTTON_LEFT,     "left" },
    { BLOOPAIR_PRO_TRIGGER_ZR,      "zr" },
    { BLOOPAIR_PRO_BUTTON_X,        "x" },
    { BLOOPAIR_PRO_BUTTON_A,        "a" },
    { BLOOPAIR_PRO_BUTTON_Y,        "y" },
    { BLOOPAIR_PRO_BUTTON_B,        "b" },
    { BLOOPAIR_PRO_TRIGGER_ZL,      "zl" },
    { BLOOPAIR_PRO_RESERVED,        "reserved" },
    { BLOOPAIR_PRO_TRIGGER_R,       "r" },
    { BLOOPAIR_PRO_BUTTON_PLUS,     "plus" },
    { BLOOPAIR_PRO_BUTTON_HOME,     "home" },
    { BLOOPAIR_PRO_BUTTON_MINUS,    "minus" },
    { BLOOPAIR_PRO_TRIGGER_L,       "l" },
    { BLOOPAIR_PRO_BUTTON_DOWN,     "down" },
    { BLOOPAIR_PRO_BUTTON_RIGHT,    "right" },
    { BLOOPAIR_PRO_BUTTON_STICK_R,  "rstick" },
    { BLOOPAIR_PRO_BUTTON_STICK_L,  "lstick" },
    { BLOOPAIR_PRO_STICK_L_UP,      "lup" },
    { BLOOPAIR_PRO_STICK_L_DOWN,    "ldown" },
    { BLOOPAIR_PRO_STICK_L_LEFT,    "lleft" },
    { BLOOPAIR_PRO_STICK_L_RIGHT,   "lright" },
    { BLOOPAIR_PRO_STICK_R_UP,      "rrup" },
    { BLOOPAIR_PRO_STICK_R_DOWN,    "rdown" },
    { BLOOPAIR_PRO_STICK_R_LEFT,    "rleft" },
    { BLOOPAIR_PRO_STICK_R_RIGHT,   "rright" },
};

const std::map<std::string, uint32_t> bloopairButtonNameValues = {
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

const std::map<BloopairControllerType, std::string> bloopairControllerTypes = {
    { BLOOPAIR_CONTROLLER_DUALSENSE,             "DualSense", },
    { BLOOPAIR_CONTROLLER_DUALSHOCK3,            "DualShock-3", },
    { BLOOPAIR_CONTROLLER_DUALSHOCK4,            "DualShock-3", },
    { BLOOPAIR_CONTROLLER_SWITCH_GENERIC,        "Switch-Generic", },
    { BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT,    "Switch-JoyCon-Left", },
    { BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT,   "Switch-JoyCon-Right", },
    { BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL,    "Switch-JoyCon-Dual", },
    { BLOOPAIR_CONTROLLER_SWITCH_PRO,            "Switch-Pro", },
    { BLOOPAIR_CONTROLLER_SWITCH_N64,            "Switch-N64", },
    { BLOOPAIR_CONTROLLER_XBOX_ONE,              "Xbox-One", },
};

const std::map<std::string, BloopairControllerType> bloopairControllerTypeValues = {
    { "DualSense",              BLOOPAIR_CONTROLLER_DUALSENSE },
    { "DualShock-3",            BLOOPAIR_CONTROLLER_DUALSHOCK3 },
    { "DualShock-3",            BLOOPAIR_CONTROLLER_DUALSHOCK4 },
    { "Switch-Generic",         BLOOPAIR_CONTROLLER_SWITCH_GENERIC },
    { "Switch-JoyCon-Left",     BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT },
    { "Switch-JoyCon-Right",    BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT },
    { "Switch-JoyCon-Dual",     BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL },
    { "Switch-Pro",             BLOOPAIR_CONTROLLER_SWITCH_PRO },
    { "Switch-N64",             BLOOPAIR_CONTROLLER_SWITCH_N64 },
    { "Xbox-One",               BLOOPAIR_CONTROLLER_XBOX_ONE },
};

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

} // namespace


Configuration::Configuration(const std::filesystem::path& path, BloopairControllerType type)
 : mPath(path), mJson(), mControllerType(type)
{
    InitConfiguration();
}

Configuration::Configuration(BloopairControllerType type)
 : mPath(), mJson(), mControllerType(type)
{
    std::string filename = "Controller-" + bloopairControllerTypes.at(type) + ".conf";
    mPath = std::filesystem::path(BLOOPAIR_CONFIGURATION_DIR) / filename;
    InitConfiguration();
}

Configuration::Configuration(const uint8_t* bda, BloopairControllerType type)
 : mPath(), mJson(), mControllerType(type)
{
    std::string filename = "Controller-" + Utils::ToHexString(bda, 6, true) + ".conf";
    mPath = std::filesystem::path(BLOOPAIR_CONFIGURATION_DIR) / filename;
    InitConfiguration();
}

Configuration::~Configuration()
{
}

std::vector<Configuration> Configuration::LoadAll()
{
    std::vector<Configuration> configurations;
    std::regex regex("^Controller-([A-Za-z0-9-]+)\\.conf$");

    for (const auto& entry : std::filesystem::directory_iterator(BLOOPAIR_CONFIGURATION_DIR)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string filename = entry.path().filename().string();
        std::smatch m;
        if (!std::regex_search(filename, m, regex)) {
            continue;
        }

        BloopairControllerType type = BLOOPAIR_CONTROLLER_INVALID;
        uint8_t bda[6];
        if (bloopairControllerTypeValues.contains(m.str(1))) {
            type = bloopairControllerTypeValues.at(m.str(1));
        } else if (!HexToBDA(m.str(1), bda)) {
            continue;
        }

        configurations.emplace_back(entry.path(), type);
    }

    return configurations;
}

bool Configuration::Save()
{
    std::ofstream file(mPath);
    file << mJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);;

    return !!file;
}

void Configuration::SetCommonConfiguration(const BloopairCommonConfiguration& config)
{
    mJson["configuration"]["stickAsButtonDeadzone"] = config.stickAsButtonDeadzone;
}

void Configuration::SetMappings(const std::vector<BloopairMappingEntry>& mappings)
{
    // Sort mappings into a map
    std::map<BloopairProButton, std::vector<uint8_t>> mappingsMap;
    for (const BloopairMappingEntry& m : mappings) {
        mappingsMap[(BloopairProButton) m.to].push_back(m.from);
    }

    nlohmann::json mapping;
    for (const auto& [key, val] : mappingsMap) {
        std::string buttonName = bloopairButtonNames.at(key);
        mapping[buttonName] = val;
    }

    mJson["mapping"] = mapping;
}

void Configuration::SetCustomConfiguraion(const DualsenseConfiguration& config)
{
    if (mControllerType != BLOOPAIR_CONTROLLER_DUALSENSE) {
        return;
    }

    // mJson["custom"]["someCustomField"] = config.SomeCustomField;
}

void Configuration::SetCustomConfiguraion(const Dualshock3Configuration& config)
{
    if (mControllerType != BLOOPAIR_CONTROLLER_DUALSHOCK3) {
        return;
    }

    // mJson["custom"]["someCustomField"] = config.SomeCustomField;
}

void Configuration::SetCustomConfiguraion(const Dualshock4Configuration& config)
{
    if (mControllerType != BLOOPAIR_CONTROLLER_DUALSHOCK4) {
        return;
    }

    // mJson["custom"]["someCustomField"] = config.SomeCustomField;
}

void Configuration::SetCustomConfiguraion(const SwitchConfiguration& config)
{
    if (mControllerType < BLOOPAIR_CONTROLLER_SWITCH_GENERIC || mControllerType > BLOOPAIR_CONTROLLER_SWITCH_N64) {
        return;
    }

    mJson["custom"]["disableCalibration"] = config.disableCalibration;
}

void Configuration::SetCustomConfiguraion(const XboxOneConfiguration& config)
{
    if (mControllerType != BLOOPAIR_CONTROLLER_XBOX_ONE) {
        return;
    }

    // mJson["custom"]["someCustomField"] = config.SomeCustomField;
}

void Configuration::Remove()
{
    std::filesystem::remove(mPath);
}

void Configuration::Remove(BloopairControllerType type)
{
    std::string filename = "Controller-" + bloopairControllerTypes.at(type) + ".conf";
    std::filesystem::path path = std::filesystem::path(BLOOPAIR_CONFIGURATION_DIR) / filename;
    std::filesystem::remove(path);
}

void Configuration::Remove(const uint8_t* bda)
{
    std::string filename = "Controller-" + Utils::ToHexString(bda, 6, true) + ".conf";
    std::filesystem::path path = std::filesystem::path(BLOOPAIR_CONFIGURATION_DIR) / filename;
    std::filesystem::remove(path);
}

std::string Configuration::GetFilename() const
{
    return mPath.filename().string();
}

bool Configuration::InitConfiguration()
{
    if (!bloopairControllerTypes.contains(mControllerType)) {
        return false;
    }

    // Try to load an existing configuration
    if (LoadConfiguration()) {
        return true;
    }

    // Initialize a default configuration
    mJson = {};
    mJson["version"] = BLOOPAIR_CONFIG_VERSION;
    mJson["controllerType"] = bloopairControllerTypes.at(mControllerType);

    return true;
}

bool Configuration::LoadConfiguration()
{
    mJson = nlohmann::ordered_json::parse(std::ifstream(mPath), nullptr, false);
    if (mJson.is_discarded()) {
        return false;
    }

    // Version check
    if (mJson.contains("version") &&
       (mJson["version"].get<uint32_t>() < BLOOPAIR_CONFIG_VERSION_MIN ||
        mJson["version"].get<uint32_t>() > BLOOPAIR_CONFIG_VERSION_MAX)) {
        return false;
    }

    // Controller type check
    BloopairControllerType controllerType = BLOOPAIR_CONTROLLER_INVALID;
    if (mJson.contains("controllerType")) {
        std::string typeString = mJson["controllerType"].get<std::string>();
        if (bloopairControllerTypeValues.contains(typeString)) {
            controllerType = bloopairControllerTypeValues.at(typeString);
        } else {
            return false;
        }
    } else {
        return false;
    }

    if (mControllerType == BLOOPAIR_CONTROLLER_INVALID) {
        mControllerType = controllerType;
    } else {
        if (controllerType != mControllerType) {
            return false;
        }
    }

    // Update the version field
    mJson["version"] = BLOOPAIR_CONFIG_VERSION;

    return true;
}
