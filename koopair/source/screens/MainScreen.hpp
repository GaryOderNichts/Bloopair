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
#include "Gfx.hpp"
#include <memory>

class MainScreen : public Screen
{
public:
    MainScreen();
    virtual ~MainScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

protected:
    void DrawStatus(std::string status, SDL_Color color = Gfx::COLOR_TEXT);

private:
    enum {
        STATE_INIT,
        STATE_CACHE_FONTS,
        STATE_BLOOPAIR_INIT,
        STATE_BLOOPAIR_ACTIVE_CHECK,
        STATE_BLOOPAIR_VERSION_CHECK,
        STATE_LOAD_MENU,
        STATE_IN_MENU,
    } mState;
    bool mStateFailure;

    uint32_t mBloopairVersion;
    std::string mCommitHash;

    std::unique_ptr<Screen> mMenuScreen;
};
