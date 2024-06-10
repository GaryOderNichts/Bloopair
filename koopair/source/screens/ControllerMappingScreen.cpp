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
#include "ControllerMappingScreen.hpp"
#include "Gfx.hpp"
#include "BloopairIPC.hpp"
#include "ProcUI.hpp"

#include <algorithm>
#include <bloopair/controllers/dualsense_controller.h>
#include <bloopair/controllers/dualshock3_controller.h>
#include <bloopair/controllers/dualshock4_controller.h>
#include <bloopair/controllers/switch_controller.h>
#include <bloopair/controllers/xbox_one_controller.h>

namespace
{

constexpr size_t kMaxEntriesPerPage = 6;

// TODO split this up
std::string GetButtonName(BloopairControllerType type, uint8_t button)
{
    switch (button) {
        case BLOOPAIR_PRO_STICK_L_UP: return "\ue081\ue092";
        case BLOOPAIR_PRO_STICK_L_DOWN: return "\ue081\ue093";
        case BLOOPAIR_PRO_STICK_L_LEFT: return "\ue081\ue091";
        case BLOOPAIR_PRO_STICK_L_RIGHT: return "\ue081\ue090";
        case BLOOPAIR_PRO_STICK_R_UP: return "\ue082\ue092";
        case BLOOPAIR_PRO_STICK_R_DOWN: return "\ue082\ue093";
        case BLOOPAIR_PRO_STICK_R_LEFT: return "\ue082\ue091";
        case BLOOPAIR_PRO_STICK_R_RIGHT: return "\ue082\ue090";
        default: break;
    }

    switch (type) {
        case BLOOPAIR_CONTROLLER_DUALSENSE:
            switch (button) {
                case DUALSENSE_BUTTON_TRIANGLE: return "Triangle";
                case DUALSENSE_BUTTON_CIRCLE: return "Circle";
                case DUALSENSE_BUTTON_CROSS: return "Cross";
                case DUALSENSE_BUTTON_SQUARE: return "Square";
                case DUALSENSE_BUTTON_UP: return "\ue079";
                case DUALSENSE_BUTTON_DOWN: return "\ue07a";
                case DUALSENSE_BUTTON_LEFT: return "\ue07b";
                case DUALSENSE_BUTTON_RIGHT: return "\ue07c";
                case DUALSENSE_BUTTON_R3: return "\ue08b";
                case DUALSENSE_BUTTON_L3: return "\ue08a";
                case DUALSENSE_BUTTON_OPTIONS: return "Options";
                case DUALSENSE_BUTTON_CREATE: return "Create";
                case DUALSENSE_TRIGGER_R2: return "R2";
                case DUALSENSE_TRIGGER_L2: return "L2";
                case DUALSENSE_TRIGGER_R1: return "R1";
                case DUALSENSE_TRIGGER_L1: return "L1";
                case DUALSENSE_BUTTON_MUTE: return "Mute";
                case DUALSENSE_BUTTON_TOUCHPAD: return "Touchpad";
                case DUALSENSE_BUTTON_PS_HOME: return "\ue044";
            }
            break;
        case BLOOPAIR_CONTROLLER_DUALSHOCK3:
            switch (button) {
                case DUALSHOCK3_BUTTON_TRIANGLE: return "Triangle";
                case DUALSHOCK3_BUTTON_CIRCLE: return "Circle";
                case DUALSHOCK3_BUTTON_CROSS: return "Cross";
                case DUALSHOCK3_BUTTON_SQUARE: return "Square";
                case DUALSHOCK3_BUTTON_UP: return "\ue079";
                case DUALSHOCK3_BUTTON_DOWN: return "\ue07a";
                case DUALSHOCK3_BUTTON_LEFT: return "\ue07b";
                case DUALSHOCK3_BUTTON_RIGHT: return "\ue07c";
                case DUALSHOCK3_BUTTON_R3: return "\ue08b";
                case DUALSHOCK3_BUTTON_L3: return "\ue08a";
                case DUALSHOCK3_BUTTON_START: return "Start";
                case DUALSHOCK3_BUTTON_SELECT: return "Select";
                case DUALSHOCK3_TRIGGER_R2: return "R2";
                case DUALSHOCK3_TRIGGER_L2: return "L2";
                case DUALSHOCK3_TRIGGER_R1: return "R1";
                case DUALSHOCK3_TRIGGER_L1: return "L1";
                case DUALSHOCK3_BUTTON_PS_HOME: return "\ue044";
            }
            break;
        case BLOOPAIR_CONTROLLER_DUALSHOCK4:
            switch (button) {
                case DUALSHOCK4_BUTTON_TRIANGLE: return "Triangle";
                case DUALSHOCK4_BUTTON_CIRCLE: return "Circle";
                case DUALSHOCK4_BUTTON_CROSS: return "Cross";
                case DUALSHOCK4_BUTTON_SQUARE: return "Square";
                case DUALSHOCK4_BUTTON_UP: return "\ue079";
                case DUALSHOCK4_BUTTON_DOWN: return "\ue07a";
                case DUALSHOCK4_BUTTON_LEFT: return "\ue07b";
                case DUALSHOCK4_BUTTON_RIGHT: return "\ue07c";
                case DUALSHOCK4_BUTTON_R3: return "\ue08b";
                case DUALSHOCK4_BUTTON_L3: return "\ue08a";
                case DUALSHOCK4_BUTTON_OPTIONS: return "Options";
                case DUALSHOCK4_BUTTON_CREATE: return "Create";
                case DUALSHOCK4_TRIGGER_R2: return "R2";
                case DUALSHOCK4_TRIGGER_L2: return "L2";
                case DUALSHOCK4_TRIGGER_R1: return "R1";
                case DUALSHOCK4_TRIGGER_L1: return "L1";
                case DUALSHOCK4_BUTTON_TOUCHPAD: return "Touchpad";
                case DUALSHOCK4_BUTTON_PS_HOME: return "\ue044";
            }
            break;
        case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
        case BLOOPAIR_CONTROLLER_SWITCH_PRO:
            switch (button) {
                case SWITCH_TRIGGER_ZR: return "\ue086";
                case SWITCH_TRIGGER_R: return "\ue084";
                case SWITCH_TRIGGER_SL_R: return "SL (R)";
                case SWITCH_TRIGGER_SR_R: return "SR (R)";
                case SWITCH_BUTTON_A: return "\ue000";
                case SWITCH_BUTTON_B: return "\ue001";
                case SWITCH_BUTTON_X: return "\ue002";
                case SWITCH_BUTTON_Y: return "\ue003";
                case SWITCH_BUTTON_CAPTURE: return "\ue01e";
                case SWITCH_BUTTON_HOME: return "\ue044";
                case SWITCH_BUTTON_STICK_L: return "\ue08a";
                case SWITCH_BUTTON_STICK_R: return "\ue08b";
                case SWITCH_BUTTON_PLUS: return "\ue045";
                case SWITCH_BUTTON_MINUS: return "\ue046";
                case SWITCH_TRIGGER_ZL: return "\ue085";
                case SWITCH_TRIGGER_L: return "\ue083";
                case SWITCH_TRIGGER_SL_L: return "SL (L)";
                case SWITCH_TRIGGER_SR_L: return "SR (L)";
                case SWITCH_BUTTON_UP: return "\ue079";
                case SWITCH_BUTTON_DOWN: return "\ue07a";
                case SWITCH_BUTTON_LEFT: return "\ue07b";
                case SWITCH_BUTTON_RIGHT: return "\ue07c";
            }
            break;
        case BLOOPAIR_CONTROLLER_SWITCH_N64:
            switch (button) {
                case SWITCH_N64_C_UP: return "C\ue092";
                case SWITCH_TRIGGER_R: return "\ue084";
                case SWITCH_TRIGGER_SL_R: return "SL (R)";
                case SWITCH_TRIGGER_SR_R: return "SR (R)";
                case SWITCH_BUTTON_A: return "\ue000";
                case SWITCH_BUTTON_B: return "\ue001";
                case SWITCH_N64_C_LEFT: return "C\ue091";
                case SWITCH_N64_C_DOWN: return "C\ue093";
                case SWITCH_BUTTON_CAPTURE: return "\ue01e";
                case SWITCH_BUTTON_HOME: return "\ue044";
                case SWITCH_BUTTON_STICK_L: return "\ue08a";
                case SWITCH_BUTTON_STICK_R: return "\ue08b";
                case SWITCH_BUTTON_PLUS: return "\ue045";
                case SWITCH_N64_C_RIGHT: return "C\ue090";
                case SWITCH_TRIGGER_ZL: return "\ue085";
                case SWITCH_TRIGGER_L: return "\ue083";
                case SWITCH_TRIGGER_SL_L: return "SL (L)";
                case SWITCH_TRIGGER_SR_L: return "SR (L)";
                case SWITCH_BUTTON_UP: return "\ue079";
                case SWITCH_BUTTON_DOWN: return "\ue07a";
                case SWITCH_BUTTON_LEFT: return "\ue07b";
                case SWITCH_BUTTON_RIGHT: return "\ue07c";
            }
        case BLOOPAIR_CONTROLLER_XBOX_ONE:
            switch (button) {
                case XBOX_ONE_BUTTON_UP: return "\ue079";
                case XBOX_ONE_BUTTON_DOWN: return "\ue07a";
                case XBOX_ONE_BUTTON_LEFT: return "\ue07b";
                case XBOX_ONE_BUTTON_RIGHT: return "\ue07c";
                case XBOX_ONE_TRIGGER_RB: return "RB";
                case XBOX_ONE_TRIGGER_LB: return "LB";
                case XBOX_ONE_BUTTON_A: return "\ue000";
                case XBOX_ONE_BUTTON_B: return "\ue001";
                case XBOX_ONE_BUTTON_X: return "\ue002";
                case XBOX_ONE_BUTTON_Y: return "\ue003";
                case XBOX_ONE_BUTTON_LSTICK: return "\ue08a";
                case XBOX_ONE_BUTTON_RSTICK: return "\ue08b";
                case XBOX_ONE_BUTTON_XBOX: return "Xbox";
                case XBOX_ONE_BUTTON_MENU: return "Menu";
                case XBOX_ONE_BUTTON_VIEW: return "View";
                case XBOX_ONE_TRIGGER_R: return "Trigger (R)";
                case XBOX_ONE_TRIGGER_L: return "Trigger (L)";
            }
            break;
        default: break;
    }

    return "Unknown";
}

}

ControllerMappingScreen::ControllerMappingScreen(const KPADController* controller, const std::vector<BloopairMappingEntry>& mappings)
 : mController(controller),
   mMappableButtons({
       { BLOOPAIR_PRO_BUTTON_A, "\ue000" },
       { BLOOPAIR_PRO_BUTTON_B, "\ue001" },
       { BLOOPAIR_PRO_BUTTON_X, "\ue002" },
       { BLOOPAIR_PRO_BUTTON_Y, "\ue003" },
       { BLOOPAIR_PRO_RESERVED, "\ue01e" },
       { BLOOPAIR_PRO_BUTTON_HOME, "\ue044" },
       { BLOOPAIR_PRO_BUTTON_PLUS, "\ue045" },
       { BLOOPAIR_PRO_BUTTON_MINUS, "\ue046" },
       { BLOOPAIR_PRO_BUTTON_UP, "\ue079" },
       { BLOOPAIR_PRO_BUTTON_DOWN, "\ue07a" },
       { BLOOPAIR_PRO_BUTTON_LEFT, "\ue07b" },
       { BLOOPAIR_PRO_BUTTON_RIGHT, "\ue07c" },
       { BLOOPAIR_PRO_TRIGGER_L, "\ue083" },
       { BLOOPAIR_PRO_TRIGGER_R, "\ue084" },
       { BLOOPAIR_PRO_TRIGGER_ZL, "\ue085" },
       { BLOOPAIR_PRO_TRIGGER_ZR, "\ue086" },
       { BLOOPAIR_PRO_BUTTON_STICK_L, "\ue08a" },
       { BLOOPAIR_PRO_BUTTON_STICK_R, "\ue08b" },
       { BLOOPAIR_PRO_STICK_L_UP, "\ue081\ue092" },
       { BLOOPAIR_PRO_STICK_L_DOWN, "\ue081\ue093" },
       { BLOOPAIR_PRO_STICK_L_LEFT, "\ue081\ue091" },
       { BLOOPAIR_PRO_STICK_L_RIGHT, "\ue081\ue090" },
       { BLOOPAIR_PRO_STICK_R_UP, "\ue082\ue092" },
       { BLOOPAIR_PRO_STICK_R_DOWN, "\ue082\ue093" },
       { BLOOPAIR_PRO_STICK_R_LEFT, "\ue082\ue091" },
       { BLOOPAIR_PRO_STICK_R_RIGHT, "\ue082\ue090" },
   }),
   mMappings(),
   mMappingState(MAPPING_STATE_NONE),
   mOldButtons(0),
   mMappingsChanged(false),
   mSelected(0),
   mSelectionStart(0),
   mSelectionEnd(kMaxEntriesPerPage)
{
    ProcUI::SetHomeButtonMenuEnabled(false);

    // Unpack mappings
    for (const BloopairMappingEntry& m : mappings) {
        mMappings[(BloopairProButton) m.to].push_back(m.from);
    }
}

ControllerMappingScreen::~ControllerMappingScreen()
{
    ProcUI::SetHomeButtonMenuEnabled(true);
}

void ControllerMappingScreen::Draw()
{
    // TODO
    DrawTopBar("ControllerMappingScreen");

    int drawIndex = 0;
    for (size_t i = mSelectionStart; i < mSelectionEnd; i++) {
        int yOff = 75 + drawIndex * 150;
        Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
        Gfx::DrawRectFilled(0, yOff, 250 - 8, 150, Gfx::COLOR_GRAY);
        Gfx::Print(250 / 2, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mMappableButtons[i].second, Gfx::ALIGN_CENTER);

        std::string str;
        for (uint8_t v : mMappings[mMappableButtons[i].first]) {
            str += GetButtonName(mController->GetControllerType(), v) + ",";
        }

        // Remove the trailing comma
        if (!str.empty()) {
            str.pop_back();
        }

        Gfx::Print(275, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, str, Gfx::ALIGN_VERTICAL);

        if (i == mSelected) {
            Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
        }

        drawIndex++;
    }

    // Draw scroll indicators
    if (mSelectionEnd < mMappableButtons.size()) {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT - 100, 60, Gfx::COLOR_ACCENT, "\ufe3e", Gfx::ALIGN_CENTER);
    }
    if (mSelectionStart > 0) {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, 100, 60, Gfx::COLOR_ACCENT, "\ufe3d", Gfx::ALIGN_CENTER);
    }

    DrawBottomBar("\ue07d Navigate", "\ue001 Back", "\ue002 Clear / \ue000 Add");

    if (mMappingState != MAPPING_STATE_NONE) {
        Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_WIDTH, { 0, 0, 0, 0xa0 });
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, Gfx::COLOR_TEXT, "Press any button to map...", Gfx::ALIGN_CENTER);
    }
}

bool ControllerMappingScreen::Update(const CombinedInputController& input)
{
    if (mMappingState == MAPPING_STATE_NONE) {
        if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
            return false;
        }

        if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
            BloopairReportBuffer report{};
            BloopairIPC::ReadRawReport(mController->GetChannel(), report);
            
            mOldButtons = report.buttons;
            mMappingState = MAPPING_STATE_WAIT_RELEASE;
        }

        if (input.GetButtonsTriggered() & Controller::BUTTON_X) {
            mMappings[mMappableButtons[mSelected].first].clear();
            mMappingsChanged = true;
        }

        if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
            if (mSelected < mMappableButtons.size() - 1) {
                mSelected++;
            }
        } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
            if (mSelected > 0) {
                mSelected--;
            }
        }
    }

    if (mMappingState != MAPPING_STATE_NONE) {
        BloopairReportBuffer report{};
        BloopairIPC::ReadRawReport(mController->GetChannel(), report);

        if (mMappingState == MAPPING_STATE_WAIT) {
            if (HandleStickRemap(report)) {
                mMappingState = MAPPING_STATE_NONE;
                mMappingsChanged = true;
            }

            if (HandleButtonRemap(report)) {
                mMappingState = MAPPING_STATE_WAIT_RELEASE2;
                mMappingsChanged = true;
            }
        }

        if (mMappingState == MAPPING_STATE_WAIT_RELEASE) {
            if (report.buttons != mOldButtons) {
                mOldButtons = 0;
                mMappingState = MAPPING_STATE_WAIT;
            }
        }
        if (mMappingState == MAPPING_STATE_WAIT_RELEASE2) {
            if (report.buttons != mOldButtons) {
                mOldButtons = 0;
                mMappingState = MAPPING_STATE_NONE;
            }
        }
    }

    if (mSelected >= mSelectionEnd) {
        mSelectionEnd = mSelected + 1;
        mSelectionStart = mSelectionEnd - kMaxEntriesPerPage;
    } else if (mSelected < mSelectionStart) {
        mSelectionStart = mSelected;
        mSelectionEnd = mSelectionStart + kMaxEntriesPerPage;
    }

    return true;
}

bool ControllerMappingScreen::GetMappingsChanged() const
{
    return mMappingsChanged;
}

std::vector<BloopairMappingEntry> ControllerMappingScreen::GetMappings() const
{
    // Pack mappings
    std::vector<BloopairMappingEntry> packedMappings;
    for (const auto& [button, mappings] : mMappings) {
        for (uint8_t m : mappings) {
            packedMappings.push_back({ m, (uint8_t)button });
        }
    }

    return packedMappings;
}

bool ControllerMappingScreen::HandleButtonRemap(const BloopairReportBuffer& report)
{
    auto& mapping = mMappings[mMappableButtons[mSelected].first];

    if (report.buttons) {
        for (int i = 0; i < BLOOPAIR_PRO_STICK_MIN; i++) {
            if (report.buttons & BTN(i)) {
                if (std::find(mapping.begin(), mapping.end(), i) == mapping.end()) {
                    mapping.push_back(i);
                }
            }
        }

        mOldButtons = report.buttons;
        return true;
    }

    return false;
}

bool ControllerMappingScreen::HandleStickRemap(const BloopairReportBuffer& report)
{
    bool changed = false;
    auto& mapping = mMappings[mMappableButtons[mSelected].first];
    auto AddMapping = ([&](BloopairProButton button) {
        if (std::find(mapping.begin(), mapping.end(), button) == mapping.end()) {
            mapping.push_back(button);
        }

        changed = true;
    });

    if (report.left_stick_x > 500) {
        AddMapping(BLOOPAIR_PRO_STICK_L_RIGHT);
    }
    if (report.left_stick_x < -500) {
        AddMapping(BLOOPAIR_PRO_STICK_L_LEFT);
    }   
    if (report.left_stick_y > 500) {
        AddMapping(BLOOPAIR_PRO_STICK_L_DOWN);
    }
    if (report.left_stick_y < -500) {
        AddMapping(BLOOPAIR_PRO_STICK_L_UP);
    }   

    if (report.right_stick_x > 500) {
        AddMapping(BLOOPAIR_PRO_STICK_R_RIGHT);
    }
    if (report.right_stick_x < -500) {
        AddMapping(BLOOPAIR_PRO_STICK_R_LEFT);
    }   
    if (report.right_stick_y > 500) {
        AddMapping(BLOOPAIR_PRO_STICK_R_DOWN);
    }
    if (report.right_stick_y < -500) {
        AddMapping(BLOOPAIR_PRO_STICK_R_UP);
    }

    return changed;
}
