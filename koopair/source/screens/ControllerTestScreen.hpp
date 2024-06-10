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

#include "Controller.hpp"

class ControllerTestScreen : public Screen
{
public:
    ControllerTestScreen(const KPADController* controller);
    virtual ~ControllerTestScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

private:
    void DrawStick(uint32_t x, uint32_t y, const KPADVec2D& stick, bool pressed);

    void DrawButton(uint32_t x, uint32_t y, uint32_t held, WPADProButton button);

    void DrawDPAD(uint32_t x, uint32_t y, uint32_t held);

    const KPADController* mController;
    KPADStatus mStatus;
    int mHoldCount;
};
