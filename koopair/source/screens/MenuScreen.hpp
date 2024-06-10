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
#include <memory>
#include <map>

class MenuScreen : public Screen
{
public:
    MenuScreen();
    virtual ~MenuScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

private:
    std::unique_ptr<Screen> mSubscreen;

    enum MenuID {
        MENU_ID_CONTROLLER_LIST,
        MENU_ID_CONTROLLER_CONFIGURATIONS,
        MENU_ID_CONTROLLER_PAIRING,
        MENU_ID_SETTINGS,
        MENU_ID_ABOUT,

        MENU_ID_MIN = MENU_ID_CONTROLLER_LIST,
        MENU_ID_MAX = MENU_ID_ABOUT,
    };

    struct MenuEntry {
        uint16_t icon;
        const char* name;
    };
    std::map<MenuID, MenuEntry> mEntries;
    MenuID mSelected;
};
