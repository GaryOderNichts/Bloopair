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
#include "Gfx.hpp"
#include "ProcUI.hpp"
#include "screens/MainScreen.hpp"
#include "ControllerManager.hpp"

#include <memory>
#include <sndcore2/core.h>

int main(int argc, char const* argv[])
{
    ProcUI::Init();
    Gfx::Init();

    // call AXInit to stop already playing sounds
    AXInit();

    ControllerManager& controllerMgr = ControllerManager::Get();

    controllerMgr.Initialize();

    std::unique_ptr<Screen> mainScreen = std::make_unique<MainScreen>();

    while (ProcUI::IsRunning()) {
        controllerMgr.Update();

        if (!mainScreen->Update(controllerMgr.GetCombinedController())) {
            ProcUI::StopRunning();
        }

        mainScreen->Draw();
        Gfx::Render();
    }

    mainScreen.reset();

    controllerMgr.Finalize();

    AXQuit();

    Gfx::Shutdown();
    ProcUI::Shutdown();
    return 0;
}
