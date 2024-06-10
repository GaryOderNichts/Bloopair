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

#include <map>
#include <memory>
#include <bloopair/controllers/common.h>

#include "Screen.hpp"
#include "ControllerOptionsScreen.hpp"

class MessageBox;

class ControllerListOptionsScreen : public Screen
{
public:
    ControllerListOptionsScreen(const KPADController* controller);
    virtual ~ControllerListOptionsScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

private:
    void SaveAndApply();
    void SaveAndApplyAll();
    void Reset();

    const KPADController* mController;
    std::unique_ptr<Screen> mSubscreen;
    std::unique_ptr<MessageBox> mMessageBox;

    enum OptionID {
        OPTION_ID_TEST,
        OPTION_ID_MAPPING,
        OPTION_ID_OPTIONS,
        OPTION_ID_SAVE_APPLY,
        OPTION_ID_RESET,
        OPTION_ID_DISCONNECT,

        OPTION_ID_MIN = OPTION_ID_TEST,
        OPTION_ID_MAX = OPTION_ID_DISCONNECT,
    };

    struct OptionEntry {
        uint16_t icon;
        const char* name;
        bool visible = true;
    };
    std::map<OptionID, OptionEntry> mEntries;
    OptionID mSelected = OPTION_ID_MIN;

    bool mMappingsChanged;
    std::vector<BloopairMappingEntry> mMappings;
    bool mCommonConfigurationChanged;
    BloopairCommonConfiguration mCommonConfiguration;
    bool mCustomConfigurationChanged;
    ControllerOptionsScreen::CustomConfiguration mCustomConfiguration;

    bool mIsApplying;
    bool mApplyToAll;
    bool mIsResetting;

    bool mDiscard;
};
