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
#include "ControllerManager.hpp"

#include <algorithm>

ControllerManager& ControllerManager::Get()
{
    static ControllerManager mgr;
    return mgr;
}

ControllerManager::ControllerManager() : mCombined(),
    mVPAD(VPAD_CHAN_0),
    mKPADs({
        KPADController(WPAD_CHAN_0),
        KPADController(WPAD_CHAN_1),
        KPADController(WPAD_CHAN_2),
        KPADController(WPAD_CHAN_3),
        KPADController(WPAD_CHAN_4),
        KPADController(WPAD_CHAN_5),
        KPADController(WPAD_CHAN_6),
    })
{
}

ControllerManager::~ControllerManager()
{
}

bool ControllerManager::Initialize()
{
    KPADInit();
    WPADEnableURCC(TRUE);

    KPADSetMaxControllers(kMaxKPADControllers);

    return true;
}

void ControllerManager::Finalize()
{
    KPADShutdown();
}

bool ControllerManager::Update()
{
    std::vector<const Controller*> toCombine;
    
    if (mVPAD.Update()) {
        toCombine.push_back(&mVPAD);
    }

    for (KPADController& kpad : mKPADs) {
        if (!kpad.Update()) {
            continue;
        }

        toCombine.push_back(&kpad);
    }

    mCombined.Combine(toCombine);

    return true;
}
