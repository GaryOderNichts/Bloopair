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
#include "MessageBox.hpp"
#include "Gfx.hpp"

MessageBox::MessageBox(const std::string& title, const std::string& message, const std::vector<Option> options)
 : mTitle(title),
   mMessage(message),
   mOptions(std::move(options)),
   mSelected(0)
{
}

MessageBox::~MessageBox()
{
}

void MessageBox::Draw()
{
    // Dim the background
    Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_WIDTH, { 0, 0, 0, 0xa0 });

    // Draw the background
    Gfx::DrawRectRoundedFilled(128, 128, Gfx::SCREEN_WIDTH - 256, Gfx::SCREEN_HEIGHT - 256, 60, Gfx::COLOR_ALT_BACKGROUND);

    // Print message
    Gfx::Print(Gfx::SCREEN_WIDTH / 2, 128 + 64, 80, Gfx::COLOR_TEXT, mTitle, Gfx::ALIGN_HORIZONTAL | Gfx::ALIGN_TOP);
    Gfx::Print(Gfx::SCREEN_WIDTH / 2, 128 + 64 + Gfx::GetTextHeight(80, mTitle) + 8, 40, Gfx::COLOR_TEXT, mMessage, Gfx::ALIGN_HORIZONTAL | Gfx::ALIGN_TOP);

    uint32_t xSize = Gfx::SCREEN_WIDTH - 256 - 32 - 8 * (mOptions.size() - 1);
    xSize /= mOptions.size();
    uint32_t xOff = 128 + 16;
    for (size_t i = 0; i < mOptions.size(); i++) {
        Gfx::DrawRectFilled(xOff, Gfx::SCREEN_HEIGHT - 300, xSize, 128, Gfx::COLOR_BACKGROUND);

        uint32_t iconWidth = 0;
        if (mOptions[i].icon) {
            iconWidth = Gfx::GetIconWidth(64, mOptions[i].icon);
        }

        uint32_t textStart = xOff + (xSize - iconWidth - Gfx::GetTextWidth(64, mOptions[i].text)) / 2;

        if (iconWidth) {
            Gfx::DrawIcon(textStart, Gfx::SCREEN_HEIGHT - 300 + 64, 64, Gfx::COLOR_TEXT, mOptions[i].icon);
        }
        Gfx::Print(textStart + iconWidth, Gfx::SCREEN_HEIGHT - 300 + 64, 64, Gfx::COLOR_TEXT, mOptions[i].text, Gfx::ALIGN_VERTICAL | Gfx::ALIGN_LEFT);

        if (i == mSelected) {
            Gfx::DrawRect(xOff, Gfx::SCREEN_HEIGHT - 300, xSize, 128, 8, Gfx::COLOR_HIGHLIGHTED);
        }

        xOff += xSize + 8;
    }
}

bool MessageBox::Update(const CombinedInputController& input)
{
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        mOptions[mSelected].callback();
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_RIGHT) {
        if (mSelected < mOptions.size() - 1) {
            mSelected++;
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_LEFT) {
        if (mSelected > 0) {
            mSelected--;
        }
    }

    return true;
}
