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
#include "ControllerListOptionsScreen.hpp"
#include "Gfx.hpp"
#include "MessageBox.hpp"
#include "ControllerTestScreen.hpp"
#include "ControllerMappingScreen.hpp"
#include "BloopairIPC.hpp"
#include "Configuration.hpp"
#include "ControllerManager.hpp"

ControllerListOptionsScreen::ControllerListOptionsScreen(const KPADController* controller)
 :  mController(controller),
    mSubscreen(),
    mMessageBox(),
    mEntries({
        { OPTION_ID_TEST,       { 0xf11b, "Test Controller" }},
        { OPTION_ID_MAPPING,    { 0xf074, "Edit Controller Mapping" }},
        { OPTION_ID_OPTIONS,    { 0xf013, "Edit Controller Options" }},
        { OPTION_ID_SAVE_APPLY, { 0xf00c, "Save and Apply Changes" }},
        { OPTION_ID_RESET,      { 0xf1f8, "Reset to Defaults" }},
        { OPTION_ID_DISCONNECT, { 0xf05e, "Disconnect Controller" }},
    }),
    mSelected(OPTION_ID_MIN),
    mMappingsChanged(false),
    mMappings(),
    mCommonConfigurationChanged(false),
    mCommonConfiguration(),
    mCustomConfigurationChanged(false),
    mCustomConfiguration({}),
    mIsApplying(false),
    mApplyToAll(false),
    mIsResetting(false),
    mDiscard(false)
{
    // Save and apply is hidden by default
    mEntries[OPTION_ID_SAVE_APPLY].visible = false;

    // Official controllers only have the test and disconnect option
    if (!mController->IsBloopairController()) {
        mEntries[OPTION_ID_MAPPING].visible = false;
        mEntries[OPTION_ID_OPTIONS].visible = false;
        mEntries[OPTION_ID_RESET].visible = false;
    } else {
        // Get mappings and options for the current controller
        uint8_t numMappings;
        if (BloopairIPC::GetControllerMapping(mController->GetChannel(), nullptr, &numMappings)) {
            mMappings.resize(numMappings);
            if (!BloopairIPC::GetControllerMapping(mController->GetChannel(), mMappings.data(), &numMappings)) {
                mMappings.clear();
            }
        }

        // TODO error handling
        BloopairIPC::GetConfiguration(mController->GetChannel(), mCommonConfiguration);

        // TODO error handling
        switch (mController->GetControllerType()) {
            case BLOOPAIR_CONTROLLER_DUALSENSE:
                BloopairIPC::GetCustomConfiguration(mController->GetChannel(), mCustomConfiguration.dualsense);
                break;
            case BLOOPAIR_CONTROLLER_DUALSHOCK3:
                BloopairIPC::GetCustomConfiguration(mController->GetChannel(), mCustomConfiguration.dualshock3);
                break;
            case BLOOPAIR_CONTROLLER_DUALSHOCK4:
                BloopairIPC::GetCustomConfiguration(mController->GetChannel(), mCustomConfiguration.dualshock4);
                break;
            case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
            case BLOOPAIR_CONTROLLER_SWITCH_PRO:
            case BLOOPAIR_CONTROLLER_SWITCH_N64:
                BloopairIPC::GetCustomConfiguration(mController->GetChannel(), mCustomConfiguration.switch_);
                break;
            case BLOOPAIR_CONTROLLER_XBOX_ONE:
                BloopairIPC::GetCustomConfiguration(mController->GetChannel(), mCustomConfiguration.xboxOne);
                break;
            default: break;
        }
    }
}

ControllerListOptionsScreen::~ControllerListOptionsScreen()
{
}

void ControllerListOptionsScreen::Draw()
{
    if (mSubscreen) {
        mSubscreen->Draw();
        return;
    }

    DrawTopBar(mController->GetName().c_str());

    // draw entries
    int i = 0;
    for (OptionID id = OPTION_ID_MIN; id <= OPTION_ID_MAX; id = static_cast<OptionID>(id + 1)) {
        if (!mEntries[id].visible) {
            continue;
        }

        int yOff = 75 + i * 150;
        Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
        Gfx::DrawIcon(68, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mEntries[id].icon);
        Gfx::Print(128 + 8, yOff + 150 / 2, 60, Gfx::COLOR_TEXT, mEntries[id].name, Gfx::ALIGN_VERTICAL);

        if (id == mSelected) {
            Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
        }

        i++;
    }

    DrawBottomBar("\ue07d Navigate", "\ue044 Exit", "\ue000 Select / \ue001 Back");

    if (mMessageBox) {
        mMessageBox->Draw();
    }

    if (mIsApplying) {
        Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_WIDTH, { 0, 0, 0, 0xa0 });
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, Gfx::COLOR_TEXT, "Saving and applying changes...", Gfx::ALIGN_CENTER);
    }

    if (mIsResetting) {
        Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_WIDTH, { 0, 0, 0, 0xa0 });
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, Gfx::COLOR_TEXT, "Resetting to defaults...", Gfx::ALIGN_CENTER);
    }
}

bool ControllerListOptionsScreen::Update(const CombinedInputController& input)
{
    // Back out if the controller disconnects
    if (!mController->IsConnected()) {
        return false;
    }

    if (mDiscard) {
        return false;
    }

    if (mSubscreen) {
        if (!mSubscreen->Update(input)) {
            if (mSelected == OPTION_ID_MAPPING) {
                ControllerMappingScreen* mappingScreen = static_cast<ControllerMappingScreen*>(mSubscreen.get());

                mMappings = mappingScreen->GetMappings();
                mMappingsChanged = mMappingsChanged || mappingScreen->GetMappingsChanged();
            } else if (mSelected == OPTION_ID_OPTIONS) {
                ControllerOptionsScreen* optionsScreen = static_cast<ControllerOptionsScreen*>(mSubscreen.get());

                mCommonConfiguration = optionsScreen->GetCommonConfiguration();
                mCommonConfigurationChanged = mCommonConfigurationChanged || optionsScreen->GetCommonChanged();
                mCustomConfiguration = optionsScreen->GetCustomConfiguration();
                mCustomConfigurationChanged = mCustomConfigurationChanged || optionsScreen->GetCustomChanged();
            }

            mEntries[OPTION_ID_SAVE_APPLY].visible = mMappingsChanged || mCommonConfigurationChanged || mCustomConfigurationChanged;
            mSubscreen.reset();
        }
        return true;
    }

    if (mMessageBox) {
        if (!mMessageBox->Update(input)) {
            mMessageBox.reset();
        }

        return true;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        if (mEntries[OPTION_ID_SAVE_APPLY].visible) {
            mMessageBox = std::make_unique<MessageBox>(
                "You have unsaved changes!",
                "Are you sure you want to go back without saving?",
                std::vector{
                    MessageBox::Option{0, "\ue001 Back", [this]() {} },
                    MessageBox::Option{0xf00d, "Discard Changes", [this]() {
                        mDiscard = true;
                    }},
                }
            );
        } else {
            return false;
        }
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (mSelected != OPTION_ID_MAX) {
            for (OptionID id = static_cast<OptionID>(mSelected + 1); id <= OPTION_ID_MAX; id = static_cast<OptionID>(id + 1)) {
                if (mEntries[id].visible) {
                    mSelected = id;
                    break;
                }
            }
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        if (mSelected != OPTION_ID_MIN) {
            for (OptionID id = static_cast<OptionID>(mSelected - 1); id >= OPTION_ID_MIN; id = static_cast<OptionID>(id - 1)) {
                if (mEntries[id].visible) {
                    mSelected = id;
                    break;
                }
            }
        }
    }

    if (mIsApplying) {
        if (mApplyToAll) {
            SaveAndApplyAll();
        } else {
            SaveAndApply();
        }
        mIsApplying = false;
    }

    if (mIsResetting) {
        Reset();
        mIsResetting = false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        switch (mSelected) {
        case OPTION_ID_TEST:
            mSubscreen = std::make_unique<ControllerTestScreen>(mController);
            break;
        case OPTION_ID_MAPPING:
            mSubscreen = std::make_unique<ControllerMappingScreen>(mController, mMappings);
            break;
        case OPTION_ID_OPTIONS:
            mSubscreen = std::make_unique<ControllerOptionsScreen>(mController, mCommonConfiguration, mCustomConfiguration);
            break;
        case OPTION_ID_SAVE_APPLY:
            mMessageBox = std::make_unique<MessageBox>(
                "Are you sure?",
                "This will apply the settings to the current controller or to all\n"
                "controllers of this type.\n\n"
                "The controller will be disconnected from the system\nand needs to be turned on again.",
                std::vector{
                    MessageBox::Option{0, "\ue001 No", [this]() {} },
                    MessageBox::Option{0xf00c, "Apply", [this]() {
                        mApplyToAll = false;
                        mIsApplying = true;
                    }},
                    MessageBox::Option{0xf560, "Apply to all", [this]() {
                        mApplyToAll = true;
                        mIsApplying = true;
                    }},
                }
            );
            break;
        case OPTION_ID_RESET:
            mMessageBox = std::make_unique<MessageBox>(
                "Are you sure?",
                "This will reset the options and mappings for the current controller\n"
                "to the defaults.\n\n"
                "The controller will be disconnected from the system\nand needs to be turned on again.",
                std::vector{
                    MessageBox::Option{0, "\ue001 No", [this]() {} },
                    MessageBox::Option{0xf1f8, "Reset", [this]() {
                        mIsResetting = true;
                    }},
                }
            );
            break;
        case OPTION_ID_DISCONNECT:
            mMessageBox = std::make_unique<MessageBox>(
                "Disconnect controller?",
                "Are you sure you want to disconnect the controller without saving?",
                std::vector{
                    MessageBox::Option{0, "\ue001 Back", [this]() {} },
                    MessageBox::Option{0xf05e, "Disconnect", [this]() {
                        mController->Disconnect();
                    }},
                }
            );
            break;
        }
    }

    return true;
}

void ControllerListOptionsScreen::SaveAndApply()
{
    auto bda = mController->GetBDA();

    // Not disconnecting the controller before applying a mapping is undefined behaviour, since Bloopair needs to redo the init sequence
    mController->Disconnect();

    Configuration cfg(bda.data(), mController->GetControllerType());

    if (mMappingsChanged) {
        BloopairIPC::ApplyControllerMapping(bda.data(), mMappings.data(), mMappings.size());
        cfg.SetMappings(mMappings);
    }
    if (mCommonConfigurationChanged) {
        BloopairIPC::ApplyConfiguration(bda.data(), mCommonConfiguration);
        cfg.SetCommonConfiguration(mCommonConfiguration);
    }
    if (mCustomConfigurationChanged) {
        switch (mController->GetControllerType()) {
            case BLOOPAIR_CONTROLLER_DUALSENSE:
                BloopairIPC::ApplyCustomConfiguration(bda.data(), mCustomConfiguration.dualsense);
                cfg.SetCustomConfiguraion(mCustomConfiguration.dualsense);
                break;
            case BLOOPAIR_CONTROLLER_DUALSHOCK3:
                BloopairIPC::ApplyCustomConfiguration(bda.data(), mCustomConfiguration.dualshock3);
                cfg.SetCustomConfiguraion(mCustomConfiguration.dualshock3);
                break;
            case BLOOPAIR_CONTROLLER_DUALSHOCK4:
                BloopairIPC::ApplyCustomConfiguration(bda.data(), mCustomConfiguration.dualshock4);
                cfg.SetCustomConfiguraion(mCustomConfiguration.dualshock4);
                break;
            case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
            case BLOOPAIR_CONTROLLER_SWITCH_PRO:
            case BLOOPAIR_CONTROLLER_SWITCH_N64:
                BloopairIPC::ApplyCustomConfiguration(bda.data(), mCustomConfiguration.switch_);
                cfg.SetCustomConfiguraion(mCustomConfiguration.switch_);
                break;
            case BLOOPAIR_CONTROLLER_XBOX_ONE:
                BloopairIPC::ApplyCustomConfiguration(bda.data(), mCustomConfiguration.xboxOne);
                cfg.SetCustomConfiguraion(mCustomConfiguration.xboxOne);
                break;
            default: break;
        }
    }
    cfg.Save();
}

void ControllerListOptionsScreen::SaveAndApplyAll()
{
    BloopairControllerType type = mController->GetControllerType();

    // Let's apply to the current controller too
    SaveAndApply();

    // Not disconnecting controllers before applying a mapping is undefined behaviour, since Bloopair needs to redo the init sequence
    // If we apply to all controllers of a type just disconnect all controllers
    if (mApplyToAll) {
        for (size_t i = 0; i < 0; i++) {
            const KPADController& controller = ControllerManager::Get().GetKPADController(i);
            if (!controller.IsConnected()) {
                continue;
            }

            controller.Disconnect();
        }
    }

    Configuration cfg(type);

    if (mMappingsChanged) {
        BloopairIPC::ApplyControllerMapping(type, mMappings.data(), mMappings.size());
        cfg.SetMappings(mMappings);
    }
    if (mCommonConfigurationChanged) {
        BloopairIPC::ApplyConfiguration(type, mCommonConfiguration);
        cfg.SetCommonConfiguration(mCommonConfiguration);
    }
    if (mCustomConfigurationChanged) {
        switch (mController->GetControllerType()) {
            case BLOOPAIR_CONTROLLER_DUALSENSE:
                BloopairIPC::ApplyCustomConfiguration(type, mCustomConfiguration.dualsense);
                cfg.SetCustomConfiguraion(mCustomConfiguration.dualsense);
                break;
            case BLOOPAIR_CONTROLLER_DUALSHOCK3:
                BloopairIPC::ApplyCustomConfiguration(type, mCustomConfiguration.dualshock3);
                cfg.SetCustomConfiguraion(mCustomConfiguration.dualshock3);
                break;
            case BLOOPAIR_CONTROLLER_DUALSHOCK4:
                BloopairIPC::ApplyCustomConfiguration(type, mCustomConfiguration.dualshock4);
                cfg.SetCustomConfiguraion(mCustomConfiguration.dualshock4);
                break;
            case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
            case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
            case BLOOPAIR_CONTROLLER_SWITCH_PRO:
            case BLOOPAIR_CONTROLLER_SWITCH_N64:
                BloopairIPC::ApplyCustomConfiguration(type, mCustomConfiguration.switch_);
                cfg.SetCustomConfiguraion(mCustomConfiguration.switch_);
                break;
            case BLOOPAIR_CONTROLLER_XBOX_ONE:
                BloopairIPC::ApplyCustomConfiguration(type, mCustomConfiguration.xboxOne);
                cfg.SetCustomConfiguraion(mCustomConfiguration.xboxOne);
                break;
            default: break;
        }
    }
    cfg.Save();
}

void ControllerListOptionsScreen::Reset()
{
    auto bda = mController->GetBDA();

    // Not disconnecting the controller before applying a mapping is undefined behaviour, since Bloopair needs to redo the init sequence
    mController->Disconnect();

    // TODO error handling
    BloopairIPC::ClearConfiguration(bda.data());
    BloopairIPC::ApplyControllerMapping(bda.data(), nullptr, 0);
    BloopairIPC::ClearCustomConfiguration(bda.data());

    Configuration::Remove(bda.data());
}
