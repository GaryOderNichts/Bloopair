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
#include "ControllerOptionsScreen.hpp"
#include "Gfx.hpp"
#include "Utils.hpp"

namespace
{

constexpr size_t kMaxEntriesPerPage = 6;

}

ControllerOptionsScreen::ControllerOptionsScreen(const KPADController* controller, BloopairCommonConfiguration& common, CustomConfiguration& custom)
 : mController(controller),
   mCommonChanged(false),
   mCommonConfiguration(common),
   mCustomChanged(false),
   mCustomConfiguration(custom),
   mOptions({
      Option{ .name = "General Options", .type = OPTION_TYPE_TITLE_BAR },
      Option{ .name = "Stick as button deadzone", .type = OPTION_TYPE_INT,
        .d = { common.stickAsButtonDeadzone, 0, 1140 },
        .callback = [this](const Option& opt) {
            mCommonConfiguration.stickAsButtonDeadzone = opt.d.value;
        } 
      },
   }),
   mCustomOptions(),
   mSelected(1),
   mSelectionStart(0),
   mSelectionEnd(kMaxEntriesPerPage),
   mIncrease(false),
   mButtonHoldTime(0)
{
    switch (mController->GetControllerType()) {
        case BLOOPAIR_CONTROLLER_DUALSENSE:
            break;
        case BLOOPAIR_CONTROLLER_DUALSHOCK3:
            break;
        case BLOOPAIR_CONTROLLER_DUALSHOCK4:
            break;
        case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
        case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
        case BLOOPAIR_CONTROLLER_SWITCH_PRO:
        case BLOOPAIR_CONTROLLER_SWITCH_N64:
            mCustomOptions.insert(mCustomOptions.begin(), 1, Option{ .name = "Switch Specific Options", .type = OPTION_TYPE_TITLE_BAR });
            mCustomOptions.push_back(Option{ .name = "Disable calibration", .type = OPTION_TYPE_BOOL,
                .b = { !!custom.switch_.disableCalibration },
                .callback = [this](const Option& opt) {
                    mCustomConfiguration.switch_.disableCalibration = opt.b.value;
                }
            });
            break;
        case BLOOPAIR_CONTROLLER_XBOX_ONE:
            break;
        default: break;
    }

    mSelectionEnd = std::min(mCustomOptions.size() + mOptions.size(), kMaxEntriesPerPage);
}

ControllerOptionsScreen::~ControllerOptionsScreen()
{
}

void ControllerOptionsScreen::Draw()
{
    DrawTopBar("ControllerOptionsScreen");

    uint32_t yOff = 128;

    for (size_t i = mSelectionStart; i < mSelectionEnd; i++) {
        Option& option = i < mOptions.size() ? mOptions[i] : mCustomOptions[i - mOptions.size()];

        // Eh I tried adding in some quick scrolling title bars, this is not nice though
        if (option.type == OPTION_TYPE_TITLE_BAR) {
            DrawHeader(32, yOff + 64, Gfx::SCREEN_WIDTH - 64, 0, option.name.c_str());
            yOff += 150;
            continue;
        }

        Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
        Gfx::Print(128 + 8, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, option.name, Gfx::ALIGN_VERTICAL);
        Gfx::Print(Gfx::SCREEN_WIDTH - 128, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, GetOptionAsString(option), Gfx::ALIGN_VERTICAL | Gfx::ALIGN_RIGHT);

        if (i == mSelected) {
            Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
        }

        yOff += 150;
    }

    // Draw scroll indicators
    if (mSelectionEnd < mOptions.size() + mCustomOptions.size()) {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT - 100, 60, Gfx::COLOR_ACCENT, "\ufe3e", Gfx::ALIGN_CENTER);
    }
    if (mSelectionStart > 0) {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, 100, 60, Gfx::COLOR_ACCENT, "\ufe3d", Gfx::ALIGN_CENTER);
    }

    DrawBottomBar("\ue07d Navigate", "\ue001 Back", "\ue07e Modify");
}

bool ControllerOptionsScreen::Update(const CombinedInputController& input)
{
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_LEFT) {
        mButtonHoldTime = 0;
        mIncrease = false;
        DecreaseOption(GetCurrentOption());
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_RIGHT) {
        mButtonHoldTime = 0;
        mIncrease = true;
        IncreaseOption(GetCurrentOption());
    }

    if (input.GetButtonsHeld() & (Controller::BUTTON_LEFT | Controller::BUTTON_RIGHT)) {
        mButtonHoldTime++;

        if (mButtonHoldTime >= 20) {
            Option& option = GetCurrentOption();
            if (mIncrease) {
                IncreaseOption(option);
            } else {
                DecreaseOption(option);
            }
        }
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (mSelected < mOptions.size() + mCustomOptions.size() - 1) {
            mSelected++;

            // Avoid selecting title bars (this assumes a title is never the last option)
            if (GetCurrentOption().type == OPTION_TYPE_TITLE_BAR) {
                mSelected++;
            }
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        // assume the first element is always a title
        if (mSelected > 1) {
            mSelected--;

            if (GetCurrentOption().type == OPTION_TYPE_TITLE_BAR) {
                mSelected--;
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

    // assume the first element is always a title which should be visible
    if (mSelected == 1) {
        mSelectionStart = 0;
        mSelectionEnd = std::min(mCustomOptions.size() + mOptions.size(), kMaxEntriesPerPage);
    }

    return true;
}

bool ControllerOptionsScreen::GetCommonChanged() const
{
    return mCommonChanged;
}

const BloopairCommonConfiguration& ControllerOptionsScreen::GetCommonConfiguration() const
{
    return mCommonConfiguration;
}

bool ControllerOptionsScreen::GetCustomChanged() const
{
    return mCustomChanged;
}

const ControllerOptionsScreen::CustomConfiguration& ControllerOptionsScreen::GetCustomConfiguration() const
{
    return mCustomConfiguration;
}

ControllerOptionsScreen::Option& ControllerOptionsScreen::GetCurrentOption()
{
    return mSelected < mOptions.size() ? mOptions[mSelected] : mCustomOptions[mSelected - mOptions.size()];
}

std::string ControllerOptionsScreen::GetOptionAsString(const Option& option)
{
    std::string str;

    switch (option.type) {
        case OPTION_TYPE_FLOAT:
            if (option.f.value > option.f.min) {
                str += "< ";
            }
            str += Utils::sprintf("%f", option.f.value);
            if (option.f.value < option.f.max) {
                str += " >";
            }
            break;
        case OPTION_TYPE_INT:
            if (option.d.value > option.d.min) {
                str += "< ";
            }
            str += Utils::sprintf("%d", option.d.value);
            if (option.d.value < option.d.max) {
                str += " >";
            }
            break;
        case OPTION_TYPE_BOOL:
            str += option.b.value ? "< True" : "False >";
            break;
        default: break;
    }

    return str;
}

void ControllerOptionsScreen::IncreaseOption(Option& option)
{
    switch (option.type) {
        case OPTION_TYPE_FLOAT:
            if (option.f.value < option.f.max) {
                option.f.value += 0.1f;
            }
            break;
        case OPTION_TYPE_INT:
            if (option.d.value < option.d.max) {
                option.d.value += 1;
            }
            break;
        case OPTION_TYPE_BOOL:
            option.b.value = true;
            break;
        default: break;
    }

    option.callback(option);

    if (mSelected > mOptions.size()) {
        mCustomChanged = true;
    } else {
        mCommonChanged = true;
    }
}
void ControllerOptionsScreen::DecreaseOption(Option& option)
{
    switch (option.type) {
        case OPTION_TYPE_FLOAT:
            if (option.f.value > option.f.min) {
                option.f.value -= 0.1f;
            }
            break;
        case OPTION_TYPE_INT:
            if (option.d.value > option.d.min) {
                option.d.value -= 1;
            }
            break;
        case OPTION_TYPE_BOOL:
            option.b.value = false;
            break;
        default: break;
    }

    option.callback(option);

    if (mSelected > mOptions.size()) {
        mCustomChanged = true;
    } else {
        mCommonChanged = true;
    }
}
