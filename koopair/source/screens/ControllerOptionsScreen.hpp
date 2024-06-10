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

#include "Screen.hpp"
#include <map>
#include <functional>
#include <bloopair/controllers/common.h>
#include <bloopair/controllers/dualsense_controller.h>
#include <bloopair/controllers/dualshock3_controller.h>
#include <bloopair/controllers/dualshock4_controller.h>
#include <bloopair/controllers/switch_controller.h>
#include <bloopair/controllers/xbox_one_controller.h>

class ControllerOptionsScreen : public Screen
{
public:
    // TODO handle this better
    union CustomConfiguration {
        DualsenseConfiguration dualsense;
        Dualshock3Configuration dualshock3;
        Dualshock4Configuration dualshock4;
        SwitchConfiguration switch_;
        XboxOneConfiguration xboxOne;
    };

    ControllerOptionsScreen(const KPADController* controller, BloopairCommonConfiguration& common, CustomConfiguration& custom);
    virtual ~ControllerOptionsScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

    bool GetCommonChanged() const;
    const BloopairCommonConfiguration& GetCommonConfiguration() const;

    bool GetCustomChanged() const;
    const CustomConfiguration& GetCustomConfiguration() const;

private:
    const KPADController* mController;

    bool mCommonChanged;
    BloopairCommonConfiguration mCommonConfiguration;

    bool mCustomChanged;
    CustomConfiguration mCustomConfiguration;

    enum OptionType {
        OPTION_TYPE_TITLE_BAR,
        OPTION_TYPE_FLOAT,
        OPTION_TYPE_INT,
        OPTION_TYPE_BOOL,
    };

    struct Option {
        std::string name;
        OptionType type;
        union {
            struct {
                float value;
                float min;
                float max;
            } f;
            struct {
                int value;
                int min;
                int max;
            } d;
            struct {
                bool value;
            } b;
        };
        std::function<void(const Option&)> callback;
    };

    Option& GetCurrentOption();

    std::string GetOptionAsString(const Option& option);
    void IncreaseOption(Option& option);
    void DecreaseOption(Option& option);

    std::vector<Option> mOptions;
    std::vector<Option> mCustomOptions;

    size_t mSelected;
    size_t mSelectionStart;
    size_t mSelectionEnd;

    bool mIncrease;
    uint32_t mButtonHoldTime;
};
