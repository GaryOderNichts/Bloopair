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
#include "ControllerConfigurationsScreen.hpp"
#include "Gfx.hpp"
#include "MessageBox.hpp"

namespace
{

constexpr size_t kMaxEntriesPerPage = 6;

}

ControllerConfigurationsScreen::ControllerConfigurationsScreen()
 : mMessageBox(),
   mConfigurations(),
   mSelected(0),
   mSelectionStart(0),
   mSelectionEnd(kMaxEntriesPerPage)
{
    mConfigurations = Configuration::LoadAll();
    mSelectionEnd = std::min(mConfigurations.size(), kMaxEntriesPerPage);
}

ControllerConfigurationsScreen::~ControllerConfigurationsScreen()
{
}

void ControllerConfigurationsScreen::Draw()
{
    // TODO
    DrawTopBar("ControllerConfigurationsScreen");

    if (!mConfigurations.empty()) {
        int drawIndex = 0;
        for (size_t i = mSelectionStart; i < mSelectionEnd; i++) {
            int yOff = 75 + drawIndex * 150;
            Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
            // Gfx::DrawIcon(68, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mEntries[id].icon);
            Gfx::Print(128 + 8, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mConfigurations[i].GetFilename(), Gfx::ALIGN_VERTICAL);

            if (i == mSelected) {
                Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
            }

            drawIndex++;
        }
    } else {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, Gfx::COLOR_TEXT, "No configurations found", Gfx::ALIGN_CENTER);
    }

    DrawBottomBar("\ue07d Navigate", "\ue001 Back", mConfigurations.empty() ? nullptr : "\ue000 Select");

    if (mMessageBox) {
        mMessageBox->Draw();
    }
}

bool ControllerConfigurationsScreen::Update(const CombinedInputController& input)
{
    if (mMessageBox) {
        if (!mMessageBox->Update(input)) {
            mMessageBox.reset();
        }

        return true;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        mMessageBox = std::make_unique<MessageBox>(
            mConfigurations[mSelected].GetFilename(),
            "Do you want to remove this configuration?",
            std::vector{
                MessageBox::Option{0, "\ue001 Back", [this]() {} },
                MessageBox::Option{0xf1f8, "Remove", [this]() {
                    mConfigurations[mSelected].Remove();
                    mConfigurations.erase(mConfigurations.begin() + mSelected);
                    mSelected = std::min(mConfigurations.size(), mSelected);

                    // TODO also remove applied configuration
                }},
            }
        );
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (mSelected < mConfigurations.size() - 1) {
            mSelected++;
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        if (mSelected > 0) {
            mSelected--;
        }
    }

    if (mSelected >= mSelectionEnd) {
        mSelectionEnd = mSelected + 1;
        mSelectionStart = mSelectionEnd - kMaxEntriesPerPage;
    } else if (mSelected < mSelectionStart) {
        mSelectionStart = mSelected;
        mSelectionEnd = mSelectionStart + kMaxEntriesPerPage;
    }

    return true;
}
