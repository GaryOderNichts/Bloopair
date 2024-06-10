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
#include "MenuScreen.hpp"
#include "Gfx.hpp"
#include "AboutScreen.hpp"
#include "ControllerConfigurationsScreen.hpp"
#include "ControllerListScreen.hpp"
#include "ControllerPairingScreen.hpp"
#include "SettingsScreen.hpp"

#include <vector>

MenuScreen::MenuScreen()
 :  mSubscreen(),
    mEntries({
        { MENU_ID_CONTROLLER_LIST,           { 0xf0ca, "Controller List" }},
        { MENU_ID_CONTROLLER_CONFIGURATIONS, { 0xf11b, "Controller Configurations" }},
        { MENU_ID_CONTROLLER_PAIRING,        { 0xf0c1, "Controller Pairing" }},
        { MENU_ID_SETTINGS,                  { 0xf013, "Settings" }},
        { MENU_ID_ABOUT,                     { 0xf05a, "About Koopair" }},
        // { MENU_ID_EXIT,                   { 0xf057, "Exit" }},
    }),
    mSelected(MENU_ID_MIN)
{
}

MenuScreen::~MenuScreen()
{
}

void MenuScreen::Draw()
{
    if (mSubscreen) {
        mSubscreen->Draw();
        return;
    }

    DrawTopBar(nullptr);

    // draw entries
    for (MenuID id = MENU_ID_MIN; id <= MENU_ID_MAX; id = static_cast<MenuID>(id + 1)) {
        int yOff = 75 + static_cast<int>(id) * 150;
        Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
        Gfx::DrawIcon(68, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mEntries[id].icon);
        Gfx::Print(128 + 8, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mEntries[id].name, Gfx::ALIGN_VERTICAL);

        if (id == mSelected) {
            Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
        }
    }

    DrawBottomBar("\ue07d Navigate", "\ue044 Exit", "\ue000 Select");
}

bool MenuScreen::Update(const CombinedInputController& input)
{
    if (mSubscreen) {
        if (!mSubscreen->Update(input)) {
            // subscreen wants to exit
            mSubscreen.reset();
        }
        return true;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (mSelected < MENU_ID_MAX) {
            mSelected = static_cast<MenuID>(mSelected + 1);
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        if (mSelected > MENU_ID_MIN) {
            mSelected = static_cast<MenuID>(mSelected - 1);
        }
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        switch (mSelected) {
        case MENU_ID_CONTROLLER_LIST:
            mSubscreen = std::make_unique<ControllerListScreen>();
            break;
        case MENU_ID_CONTROLLER_CONFIGURATIONS:
            mSubscreen = std::make_unique<ControllerConfigurationsScreen>();
            break;
        case MENU_ID_CONTROLLER_PAIRING:
            mSubscreen = std::make_unique<ControllerPairingScreen>();
            break;
        case MENU_ID_SETTINGS:
            mSubscreen = std::make_unique<SettingsScreen>();
            break;
        case MENU_ID_ABOUT:
            mSubscreen = std::make_unique<AboutScreen>();
            break;
        }
    }

    return true;
}
