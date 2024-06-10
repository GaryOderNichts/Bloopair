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

SettingsScreen::SettingsScreen()
{
}

SettingsScreen::~SettingsScreen()
{
}

void SettingsScreen::Draw()
{
    // TODO
    DrawTopBar("SettingsScreen");

    Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 60, Gfx::COLOR_TEXT, "Nothing here yet :)", Gfx::ALIGN_CENTER);

    DrawBottomBar("\ue001 Back", nullptr, nullptr);
}

bool SettingsScreen::Update(const CombinedInputController& input)
{
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    return true;
}
