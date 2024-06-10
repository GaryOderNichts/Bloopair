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
#include "ControllerListScreen.hpp"
#include "Gfx.hpp"
#include "ControllerListOptionsScreen.hpp"

ControllerListScreen::ControllerListScreen()
 : mControllers(),
   mSelected(0)
{
}

ControllerListScreen::~ControllerListScreen()
{
}

void ControllerListScreen::Draw()
{
    if (mControllerOptionsScreen) {
        mControllerOptionsScreen->Draw();
        return;
    }

    DrawTopBar("Controller List");

    const size_t controllerCount = mControllers.size();
    if (controllerCount) {
        uint32_t cnt = 0;
        for (const auto& [chan, name] : mControllers) {
            int yOff = 75 + cnt * 150;
            Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
            Gfx::DrawIcon(68, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, 0xf11b);
            Gfx::Print(128 + 8, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, name, Gfx::ALIGN_VERTICAL);

            if (cnt == mSelected) {
                Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
            }

            cnt++;
        }
    } else {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, Gfx::COLOR_TEXT, "No controllers connected", Gfx::ALIGN_CENTER);
    }

    DrawBottomBar(controllerCount ? "\ue07d Navigate" : nullptr, "\ue044 Exit", controllerCount ? "\ue000 Select / \ue001 Back" : "\ue001 Back");
}

bool ControllerListScreen::Update(const CombinedInputController& input)
{
    if (mControllerOptionsScreen) {
        if (!mControllerOptionsScreen->Update(input)) {
            mControllerOptionsScreen.reset();
        }
        return true;
    }

    ControllerManager& controllerMgr = ControllerManager::Get();

    mControllers.clear();
    for (size_t i = 0; i < ControllerManager::kMaxKPADControllers; i++) {
        const KPADController& controller = controllerMgr.GetKPADController(i);
        if (!controller.IsConnected() || controller.GetExtensionType() != WPAD_EXT_PRO_CONTROLLER) {
            continue;
        }

        mControllers.emplace_back(controller.GetChannel(), controller.GetName());
    }

    const size_t controllerCount = mControllers.size();

    // Make sure the controller still points at a valid index
    if (controllerCount > 0 && mSelected >= controllerCount) {
        mSelected = controllerCount - 1;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        if (controllerCount > 0) {
            mControllerOptionsScreen = std::make_unique<ControllerListOptionsScreen>(&controllerMgr.GetKPADController(mControllers[mSelected].first));
        }
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (controllerCount > 0 && mSelected < controllerCount - 1) {
            mSelected++;
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        if (mSelected > 0) {
            mSelected--;
        }
    }

    return true;
}
