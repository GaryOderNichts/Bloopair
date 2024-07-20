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
#include "MainScreen.hpp"
#include "MenuScreen.hpp"
#include "Gfx.hpp"
#include "BloopairIPC.hpp"
#include "Utils.hpp"

#include <vector>
#include <filesystem>

namespace
{

// Let's make sure to not break API for patch versions
constexpr uint32_t kMinBloopairVersion = BLOOPAIR_VERSION(1,   0,   0);
constexpr uint32_t kMaxBloopairVersion = BLOOPAIR_VERSION(1,   0, 255);

}

MainScreen::MainScreen()
 :  mState(STATE_INIT),
    mStateFailure(false),
    mBloopairVersion(0),
    mCommitHash(),
    mMenuScreen()
{
}

MainScreen::~MainScreen()
{
    if (mState > STATE_BLOOPAIR_INIT) {
        BloopairIPC::Shutdown();
    }
}

void MainScreen::Draw()
{
    Gfx::Clear(Gfx::COLOR_BACKGROUND);

    if (mMenuScreen) {
        mMenuScreen->Draw();
        return;
    }

    DrawTopBar(nullptr);

    switch (mState) {
        case STATE_INIT:
            DrawStatus("Initializing...");
            break;
        case STATE_CACHE_FONTS:
            DrawStatus("Caching Fonts...");
            break;
        case STATE_BLOOPAIR_INIT:
            if (mStateFailure) {
                DrawStatus("Failed to init Bloopair IPC!\nSomething is wrong...", Gfx::COLOR_ERROR);
                break;
            }

            DrawStatus("Initializing Bloopair...");
            break;
        case STATE_BLOOPAIR_ACTIVE_CHECK:
            if (mStateFailure) {
                DrawStatus("Bloopair isn't running!\nDo you have Bloopair installed properly?", Gfx::COLOR_ERROR);
                break;
            }

            DrawStatus("Checking if Bloopair is active...");
            break;
        case STATE_BLOOPAIR_VERSION_CHECK:
#ifndef NDEBUG
            if (mStateFailure) {
                DrawStatus(Utils::sprintf("Koopair debug build without matching Bloopair build!\n"
                                          "Make sure to always use matching debug builds.\n"
                                          "Koopair: %s\n"
                                          "Bloopair: %s",
                        std::string(COMMIT_HASH).substr(0, 7).c_str(),
                        mCommitHash.empty() ? "Not a debug build" : mCommitHash.substr(0, 7).c_str()),
                    Gfx::COLOR_ERROR);
                break;
            }
#else
            if (mStateFailure) {
                if (mCommitHash.empty()) {
                    DrawStatus(Utils::sprintf("Unsupported Bloopair version (v%d.%d.%d)!\nMake sure to update Bloopair and Koopair.",
                        BLOOPAIR_VERSION_MAJOR(mBloopairVersion), BLOOPAIR_VERSION_MINOR(mBloopairVersion), BLOOPAIR_VERSION_PATCH(mBloopairVersion)),
                        Gfx::COLOR_ERROR);
                } else {
                    DrawStatus(Utils::sprintf("Unsupported Bloopair version (%s)!\nRunning debug Bloopair and release Koopair.",
                        mCommitHash.substr(0, 7).c_str()), Gfx::COLOR_ERROR);
                }
                break;
            }
#endif

            DrawStatus("Checking Bloopair version...");
            break;
        case STATE_LOAD_MENU:
            DrawStatus("Loading menu...");
            break;
        case STATE_IN_MENU:
            break;
    }

    DrawBottomBar(mStateFailure ? nullptr : "Please wait...", mStateFailure ? "\ue044 Exit" : nullptr, nullptr);
}

bool MainScreen::Update(const CombinedInputController& input)
{
    if (mMenuScreen) {
        if (!mMenuScreen->Update(input)) {
            // menu wants to exit
            return false;
        }
        return true;
    }

    if (mStateFailure) {
        return true;
    }

    switch (mState) {
    case STATE_INIT:
        std::filesystem::create_directories("wiiu/bloopair");
        mState = STATE_CACHE_FONTS;
        break;
    case STATE_CACHE_FONTS:
        // Trigger the font cache to cache some commonly used sizes
        // TODO using Print is not nice
        Gfx::Print(0, 0, 40, {0, 0, 0, 0}, "");
        Gfx::Print(0, 0, 80, {0, 0, 0, 0}, "");
        Gfx::Print(0, 0, 200, {0, 0, 0, 0}, "");
        mState = STATE_BLOOPAIR_INIT;
        break; 
    case STATE_BLOOPAIR_INIT:
        if (!BloopairIPC::Init()) {
            mStateFailure = true;
            break;
        }
        mState = STATE_BLOOPAIR_ACTIVE_CHECK;
        break;
    case STATE_BLOOPAIR_ACTIVE_CHECK:
        if (!BloopairIPC::IsActive()) {
            mStateFailure = true;
            break;
        }
        mState = STATE_BLOOPAIR_VERSION_CHECK;
        break;
    case STATE_BLOOPAIR_VERSION_CHECK:
#ifndef NDEBUG
        mCommitHash = BloopairIPC::GetCommitHash();
        if (mCommitHash != COMMIT_HASH) {
            mStateFailure = true;
            break;
        }
#else
        mCommitHash = BloopairIPC::GetCommitHash();
        if (!mCommitHash.empty()) {
            mStateFailure = true;
            break;  
        }

        mBloopairVersion = BloopairIPC::GetVersion();
        if (mBloopairVersion < kMinBloopairVersion || mBloopairVersion > kMaxBloopairVersion) {
            mStateFailure = true;
            break;
        }
#endif

        mState = STATE_LOAD_MENU;
        break;
    case STATE_LOAD_MENU:
        mMenuScreen = std::make_unique<MenuScreen>();
        mState = STATE_IN_MENU;
        break;
    case STATE_IN_MENU:
        break;
    };

    return true;
}

void MainScreen::DrawStatus(std::string status, SDL_Color color)
{
    Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, color, status, Gfx::ALIGN_CENTER);
}
