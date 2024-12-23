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
#include "SettingsScreen.hpp"
#include "Gfx.hpp"
#include "LanguageSelectionScreen.hpp"

SettingsScreen::SettingsScreen()
 : mSelectedOption(OPTION_LANGUAGE)
{
    mEntries = {
        { OPTION_LANGUAGE, "Language Selection" },
        // Further entries can be added here
    };
}

SettingsScreen::~SettingsScreen()
{
}

void SettingsScreen::Draw()
{
    DrawTopBar("Settings");

    int yOff = Gfx::SCREEN_HEIGHT / 2 - 75;
    Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
    Gfx::Print(Gfx::SCREEN_WIDTH / 2, yOff + 75, 60, Gfx::COLOR_TEXT, "Language Selection", Gfx::ALIGN_CENTER);

    // Highlight the button when it is selected
    if (mSelectedOption == OPTION_LANGUAGE) {
        Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
    }

    DrawBottomBar("\ue001 Back", nullptr, "\ue000 Select");
}

bool SettingsScreen::Update(const CombinedInputController& input)
{
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    // If A is pressed and the Language button is selected, switch to the language selection page
    if ((input.GetButtonsTriggered() & Controller::BUTTON_A) && mSelectedOption == OPTION_LANGUAGE) {
        mSubscreen = std::make_unique<LanguageSelectionScreen>();
    }

    return true;
}
