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

#include <utility>
#include <vector>
#include <string>

#include "Controller.hpp"

class Screen
{
public:
    Screen() = default;
    virtual ~Screen() = default;

    virtual void Draw() = 0;

    virtual bool Update(const CombinedInputController& input) = 0;

protected:
    void DrawTopBar(const char* name);

    void DrawBottomBar(const char* leftHint, const char* centerHint, const char* rightHint);

    int DrawHeader(int x, int y, int w, uint16_t icon, const char* text);

    struct ScreenListElement
    {
        ScreenListElement(std::string string, bool monospace = false) 
            : string(string), monospace(monospace) {}
        ScreenListElement(const char* string, bool monospace = false)
            : string(string), monospace(monospace) {}

        std::string string;
        bool monospace;
    };
    using ScreenList = std::vector<std::pair<std::string, ScreenListElement>>;

    int DrawList(int x, int y, int w, ScreenList items);

private:
};
