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
#include <map>
#include <bloopair/controllers/common.h>

class ControllerMappingScreen : public Screen
{
public:
    ControllerMappingScreen(const KPADController* controller, const std::vector<BloopairMappingEntry>& mappings);
    virtual ~ControllerMappingScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

    bool GetMappingsChanged() const;
    std::vector<BloopairMappingEntry> GetMappings() const;

private:
    bool HandleButtonRemap(const BloopairReportBuffer& report);
    bool HandleStickRemap(const BloopairReportBuffer& report);

    const KPADController* mController;

    std::vector<std::pair<BloopairProButton, const char*>> mMappableButtons;
    std::map<BloopairProButton, std::vector<uint8_t>> mMappings;

    enum {
        MAPPING_STATE_NONE,
        MAPPING_STATE_WAIT_RELEASE,
        MAPPING_STATE_WAIT,
        MAPPING_STATE_WAIT_RELEASE2,
    } mMappingState;
    uint32_t mOldButtons;
    bool mMappingsChanged;

    size_t mSelected;
    size_t mSelectionStart;
    size_t mSelectionEnd;
};
