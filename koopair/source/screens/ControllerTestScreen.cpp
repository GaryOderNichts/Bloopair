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
#include "ControllerTestScreen.hpp"
#include "Gfx.hpp"
#include "Utils.hpp"
#include "ControllerManager.hpp"
#include "ProcUI.hpp"

namespace
{

const char* GetCharacterForButton(WPADProButton button)
{
    switch (button) {
    case WPAD_PRO_BUTTON_A: return "\ue000";
    case WPAD_PRO_BUTTON_B: return "\ue001";
    case WPAD_PRO_BUTTON_X: return "\ue002";
    case WPAD_PRO_BUTTON_Y: return "\ue003";
    case WPAD_PRO_RESERVED: return "\ue01e";
    case WPAD_PRO_BUTTON_HOME: return "\ue044";
    case WPAD_PRO_BUTTON_PLUS: return "\ue045";
    case WPAD_PRO_BUTTON_MINUS: return "\ue046";
    case WPAD_PRO_BUTTON_UP: return "\ue079";
    case WPAD_PRO_BUTTON_DOWN: return "\ue07a";
    case WPAD_PRO_BUTTON_LEFT: return "\ue07b";
    case WPAD_PRO_BUTTON_RIGHT: return "\ue07c";
    case WPAD_PRO_TRIGGER_L: return "\ue083";
    case WPAD_PRO_TRIGGER_R: return "\ue084";
    case WPAD_PRO_TRIGGER_ZL: return "\ue085";
    case WPAD_PRO_TRIGGER_ZR: return "\ue086";
    default: break;
    }

    return nullptr;
}

}

ControllerTestScreen::ControllerTestScreen(const KPADController* controller)
 : mController(controller),
   mStatus(),
   mHoldCount(0)
{
    ProcUI::SetHomeButtonMenuEnabled(false);
}

ControllerTestScreen::~ControllerTestScreen()
{
    ProcUI::SetHomeButtonMenuEnabled(true);
}

void ControllerTestScreen::Draw()
{
    DrawTopBar("Controller Test");

    uint32_t controllerCenterX = Gfx::SCREEN_WIDTH / 2;
    uint32_t controllerCenterY = Gfx::SCREEN_HEIGHT / 2;

    // minus, home, plus
    DrawButton(controllerCenterX - 100, controllerCenterY, mStatus.pro.hold, WPAD_PRO_BUTTON_MINUS);
    DrawButton(controllerCenterX, controllerCenterY, mStatus.pro.hold, WPAD_PRO_BUTTON_HOME);
    DrawButton(controllerCenterX + 100, controllerCenterY, mStatus.pro.hold, WPAD_PRO_BUTTON_PLUS);

    // reserved button
    if (mController->IsBloopairController()) {
        DrawButton(controllerCenterX, controllerCenterY - 100, mStatus.pro.hold, WPAD_PRO_RESERVED);
    }

    // right stick, left stick
    DrawStick(controllerCenterX + 450, controllerCenterY - 50, mStatus.pro.rightStick, mStatus.pro.hold & WPAD_PRO_BUTTON_STICK_R);
    DrawStick(controllerCenterX - 450, controllerCenterY - 50, mStatus.pro.leftStick, mStatus.pro.hold & WPAD_PRO_BUTTON_STICK_L);

    // L, ZL
    DrawButton(controllerCenterX - 450, controllerCenterY - 200, mStatus.pro.hold, WPAD_PRO_TRIGGER_L);
    DrawButton(controllerCenterX - 450, controllerCenterY - 270, mStatus.pro.hold, WPAD_PRO_TRIGGER_ZL);

    // R, ZR
    DrawButton(controllerCenterX + 450, controllerCenterY - 200, mStatus.pro.hold, WPAD_PRO_TRIGGER_R);
    DrawButton(controllerCenterX + 450, controllerCenterY - 270, mStatus.pro.hold, WPAD_PRO_TRIGGER_ZR);

    // Dpad
    DrawDPAD(controllerCenterX - 300, controllerCenterY + 150, mStatus.pro.hold);

    uint32_t abxyCenterX = controllerCenterX + 300;
    uint32_t abxyCenterY = controllerCenterY + 150;

    // A, B, X, Y
    DrawButton(abxyCenterX + 80, abxyCenterY, mStatus.pro.hold, WPAD_PRO_BUTTON_A);
    DrawButton(abxyCenterX, abxyCenterY + 80, mStatus.pro.hold, WPAD_PRO_BUTTON_B);
    DrawButton(abxyCenterX, abxyCenterY - 80, mStatus.pro.hold, WPAD_PRO_BUTTON_X);
    DrawButton(abxyCenterX - 80, abxyCenterY, mStatus.pro.hold, WPAD_PRO_BUTTON_Y);

    // TODO draw LEDs?

    DrawBottomBar(nullptr, nullptr, "\ue001 Back (Hold)");
}

bool ControllerTestScreen::Update(const CombinedInputController& input)
{
    if (input.GetButtonsHeld() & Controller::BUTTON_B) {
        mHoldCount++;
    } else {
        mHoldCount = 0;
    }

    // Hold for 3 Seconds
    if (mHoldCount >= 60) {
        return false;
    }

    mStatus = mController->GetStatus();

    return true;
}

void ControllerTestScreen::DrawStick(uint32_t x, uint32_t y, const KPADVec2D& stick, bool pressed)
{
    // Draw the pressed background first
    if (pressed) {
        Gfx::DrawCircleFilled(x, y, 80, Gfx::COLOR_ERROR);
    }

    Gfx::DrawCircle(x, y, 80, 6, Gfx::COLOR_GRAY);
    Gfx::DrawCircleFilled(x + stick.x * 50.0f, y + stick.y * -50.0f, 50, Gfx::COLOR_WHITE);

    if (x > Gfx::SCREEN_WIDTH / 2) {
        Gfx::Print(x + 120, y - 25, 40, Gfx::COLOR_TEXT, Utils::sprintf("X: %6.2f", stick.x), Gfx::ALIGN_LEFT | Gfx::ALIGN_VERTICAL);
        Gfx::Print(x + 120, y + 25, 40, Gfx::COLOR_TEXT, Utils::sprintf("Y: %6.2f", stick.y), Gfx::ALIGN_LEFT | Gfx::ALIGN_VERTICAL);
    } else {
        int width = Gfx::GetTextWidth(40, Utils::sprintf("Y: %6.2f", -9.9));

        Gfx::Print(x - 120 - width, y - 25, 40, Gfx::COLOR_TEXT, Utils::sprintf("X: %6.2f", stick.x), Gfx::ALIGN_LEFT | Gfx::ALIGN_VERTICAL);
        Gfx::Print(x - 120 - width, y + 25, 40, Gfx::COLOR_TEXT, Utils::sprintf("Y: %6.2f", stick.y), Gfx::ALIGN_LEFT | Gfx::ALIGN_VERTICAL);
    }
}

void ControllerTestScreen::DrawButton(uint32_t x, uint32_t y, uint32_t held, WPADProButton button)
{
    const char* btnChar = GetCharacterForButton(button);
    if (!btnChar) {
        return;
    }

    Gfx::Print(x, y, 80, held & button ? Gfx::COLOR_ERROR : Gfx::COLOR_WHITE, btnChar, Gfx::ALIGN_CENTER);
}

void ControllerTestScreen::DrawDPAD(uint32_t x, uint32_t y, uint32_t held)
{
    if (held & WPAD_PRO_BUTTON_UP) {
        Gfx::Print(x, y, 200, Gfx::COLOR_ERROR, GetCharacterForButton(WPAD_PRO_BUTTON_UP), Gfx::ALIGN_CENTER);
    }

    if (held & WPAD_PRO_BUTTON_DOWN) {
        Gfx::Print(x, y, 200, Gfx::COLOR_ERROR, GetCharacterForButton(WPAD_PRO_BUTTON_DOWN), Gfx::ALIGN_CENTER);
    }

    if (held & WPAD_PRO_BUTTON_LEFT) {
        Gfx::Print(x, y, 200, Gfx::COLOR_ERROR, GetCharacterForButton(WPAD_PRO_BUTTON_LEFT), Gfx::ALIGN_CENTER);
    }

    if (held & WPAD_PRO_BUTTON_RIGHT) {
        Gfx::Print(x, y, 200, Gfx::COLOR_ERROR, GetCharacterForButton(WPAD_PRO_BUTTON_RIGHT), Gfx::ALIGN_CENTER);
    }

    Gfx::Print(x, y, 200, Gfx::COLOR_WHITE, "\ue041", Gfx::ALIGN_CENTER);
}
