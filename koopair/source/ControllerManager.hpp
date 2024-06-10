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

#include "Controller.hpp"

#include <vector>
#include <array>
#include <functional>

class ControllerManager {
public:
    static constexpr size_t kMaxKPADControllers = 7;

public:
    ControllerManager(const ControllerManager&) = delete;
    void operator=(const ControllerManager&) = delete;

    static ControllerManager& Get();

    bool Initialize();

    void Finalize();

    bool Update();

    const CombinedInputController& GetCombinedController() const
    {
        return mCombined;
    }

    const VPADController& GetVPADController() const
    {
        return mVPAD;
    }

    size_t GetConnectedKPADControllerCount() const
    {
        size_t count = 0;

        for (const KPADController& c : mKPADs) {
            if (c.IsConnected()) {
                count++;
            }
        }

        return count;
    }

    const KPADController& GetKPADController(size_t i) const
    {
        return mKPADs.at(i);
    }

private:
    ControllerManager();
    ~ControllerManager();

    CombinedInputController mCombined;
    VPADController mVPAD;
    std::array<KPADController, kMaxKPADControllers> mKPADs;
};
