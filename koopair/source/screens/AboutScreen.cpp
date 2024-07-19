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
#include "AboutScreen.hpp"
#include "Gfx.hpp"
#include "BloopairIPC.hpp"
#include "Utils.hpp"

AboutScreen::AboutScreen()
{
    mCreditList.push_back({"Developers:", "GaryOderNichts"});

    mFontList.push_back({"Main Font:", "Wii U System Font"});
    mFontList.push_back({"Icon Font:", "FontAwesome"});
    mFontList.push_back({"Monospace Font:", "Terminus Font"});

    mLinkList.push_back({"GitHub:", ""});
    mLinkList.push_back({"", {"github.com/GaryOderNichts/Bloopair", true}});

    uint32_t bloopairVersion = BloopairIPC::GetVersion();
    std::string commitHash = BloopairIPC::GetCommitHash();
    std::string shortHash = "";
    if (commitHash.size() > 7) {
        shortHash = commitHash.substr(0, 7);
    }

    mBloopairList.push_back({"Running Version:", {
        Utils::sprintf("v%d.%d.%d%s%s",
            BLOOPAIR_VERSION_MAJOR(bloopairVersion),
            BLOOPAIR_VERSION_MINOR(bloopairVersion),
            BLOOPAIR_VERSION_PATCH(bloopairVersion),
            shortHash.empty() ? "" : "-",
            shortHash.c_str())}});
}

AboutScreen::~AboutScreen()
{
}

void AboutScreen::Draw()
{
    DrawTopBar("About");

    int yOff = 128;
    yOff = DrawHeader(32, yOff, 896, 0xf121, "Credits");
    yOff = DrawList(32, yOff, 896, mCreditList);
    yOff = DrawHeader(32, yOff, 896, 0xf031, "Fonts");
    yOff = DrawList(32, yOff, 896, mFontList);

    yOff = 128;
    yOff = DrawHeader(992, yOff, 896, 0xf08e, "Links");
    yOff = DrawList(992, yOff, 896, mLinkList);

    yOff = DrawHeader(992, yOff, 896, 0xf085, "Bloopair");
    yOff = DrawList(992, yOff, 896, mBloopairList);

    DrawBottomBar(nullptr, "\ue044 Exit", "\ue001 Back");
}

bool AboutScreen::Update(const CombinedInputController& input)
{
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    return true;
}
