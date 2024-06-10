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
#include "Controller.hpp"
#include "BloopairIPC.hpp"

namespace {

std::string GetNameForExtensionType(WPADExtensionType type)
{
    switch (type) {
    case WPAD_EXT_CORE:
    case WPAD_EXT_MPLUS:
        return "Wii Remote";
    case WPAD_EXT_NUNCHUK:
    case WPAD_EXT_MPLUS_NUNCHUK:
        return "Wii Remote + Nunchuk";
    case WPAD_EXT_CLASSIC:
    case WPAD_EXT_MPLUS_CLASSIC:
        return "Classic Controller";
    case WPAD_EXT_PRO_CONTROLLER:
        return "Pro Controller";
    default:
        return "Unknown Controller";
    }
}

std::string GetNameForBloopairControllerType(BloopairControllerType type)
{
    switch (type) {
    case BLOOPAIR_CONTROLLER_OFFICIAL:
        return "Official Pro Controller";
    case BLOOPAIR_CONTROLLER_DUALSENSE:
        return "DualSense";
    case BLOOPAIR_CONTROLLER_DUALSHOCK3:
        return "DualShock 3";
    case BLOOPAIR_CONTROLLER_DUALSHOCK4:
        return "DualShock 4";
    case BLOOPAIR_CONTROLLER_SWITCH_GENERIC:
        return "Generic Switch Controller";
    case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT:
        return "Left Joy-Con";
    case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT:
        return "Right Joy-Con";
    case BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL:
        return "Joy-Cons";
    case BLOOPAIR_CONTROLLER_SWITCH_PRO:
        return "Switch Pro Controller";
    case BLOOPAIR_CONTROLLER_SWITCH_N64:
        return "Switch N64 Controller";
    case BLOOPAIR_CONTROLLER_XBOX_ONE:
        return "Xbox One Controller";
    default:
        return "Unknown Pro Controller";
    }
}

}

Controller::Controller() : mButtonsTriggered(),
    mButtonsHeld(),
    mStickL(),
    mStickR()
{
}

Controller::~Controller()
{
}

bool Controller::Update()
{
    mButtonsTriggered = BUTTON_NONE;
    mButtonsHeld = BUTTON_NONE;
    mStickL.x = mStickL.y = 0.0f;
    mStickR.x = mStickR.y = 0.0f;

    return true;
}

VPADController::VPADController(VPADChan chan) : Controller(),
    mIsConnected(false),
    mName(),
    mChannel(chan),
    mStatus()
{
    mName = "Wii U Gamepad " + std::to_string(static_cast<int>(mChannel) + 1);
}

VPADController::~VPADController()
{
}

bool VPADController::Update() 
{
    if (!Controller::Update()) {
        return false;
    }

    VPADReadError error;
    if (VPADRead(mChannel, &mStatus, 1, &error) <= 0 || error != VPAD_READ_SUCCESS) {
        mStatus = { 0 };
        mIsConnected = false;
        return false;
    }

    mIsConnected = true;

#define MAP_BTN(from, to) \
    do { \
        if (mStatus.hold & from) mButtonsHeld |= to; \
        if (mStatus.trigger & from) mButtonsTriggered |= to; \
    } while (0)
    MAP_BTN(VPAD_BUTTON_A,          BUTTON_A);
    MAP_BTN(VPAD_BUTTON_B,          BUTTON_B);
    MAP_BTN(VPAD_BUTTON_X,          BUTTON_X);
    MAP_BTN(VPAD_BUTTON_Y,          BUTTON_Y);
    MAP_BTN(VPAD_BUTTON_LEFT,       BUTTON_LEFT);
    MAP_BTN(VPAD_BUTTON_RIGHT,      BUTTON_RIGHT);
    MAP_BTN(VPAD_BUTTON_UP,         BUTTON_UP);
    MAP_BTN(VPAD_BUTTON_DOWN,       BUTTON_DOWN);
    MAP_BTN(VPAD_BUTTON_ZL,         BUTTON_ZL);
    MAP_BTN(VPAD_BUTTON_ZR,         BUTTON_ZR);
    MAP_BTN(VPAD_BUTTON_L,          BUTTON_L);
    MAP_BTN(VPAD_BUTTON_R,          BUTTON_R);
    MAP_BTN(VPAD_BUTTON_PLUS,       BUTTON_PLUS);
    MAP_BTN(VPAD_BUTTON_MINUS,      BUTTON_MINUS);
    MAP_BTN(VPAD_BUTTON_STICK_L,    BUTTON_STICK_L);
    MAP_BTN(VPAD_BUTTON_STICK_R,    BUTTON_STICK_R);
#undef MAP_BTN

    mStickL.x = mStatus.leftStick.x;
    mStickL.y = mStatus.leftStick.y;
    mStickR.x = mStatus.rightStick.x;
    mStickR.y = mStatus.rightStick.y;

    return true;
}

bool VPADController::IsConnected() const
{
    return mIsConnected;
}

const std::string& VPADController::GetName() const
{
    return mName;
}

KPADController::KPADController(KPADChan chan) : Controller(),
    mIsConnected(false),
    mRetrieveTimer(60),
    mName(),
    mChannel(chan),
    mExtension(),
    mStatus(),
    mIsBloopairController(false),
    mControllerInfo()
{
    mName = "Unknown Controller (" + std::to_string(static_cast<int>(mChannel) + 1) + ")";
}

KPADController::~KPADController()
{
}

bool KPADController::Update()
{
    if (!Controller::Update()) {
        return false;
    }

    if (WPADProbe(mChannel, &mExtension) != 0) {
        mIsConnected = false;
        return false;
    }

    KPADError error;
    if (KPADReadEx(mChannel, &mStatus, 1, &error) <= 0 || error != KPAD_ERROR_OK) {
        return false;
    }

    // Sometimes the extension type is reported as 0xFF when the controller isn't ready yet
    if (mStatus.extensionType == 0xFF) {
        return false;
    }
    
    if (!mIsConnected) {
        RetreiveControllerInformation();
    }

    // Some controllers e.g. Switch need some time to report back accurate information
    // We'll just continously poll the information every second
    if (--mRetrieveTimer == 0) {
        RetreiveControllerInformation();

        mRetrieveTimer = 60;
    }

    mIsConnected = true;

    if (mStatus.extensionType == WPAD_EXT_CORE || mStatus.extensionType == WPAD_EXT_NUNCHUK) {
#define MAP_BTN(from, to) \
    do { \
        if (mStatus.hold & from) mButtonsHeld |= to; \
        if (mStatus.trigger & from) mButtonsTriggered |= to; \
    } while (0)
        MAP_BTN(WPAD_BUTTON_LEFT,   BUTTON_LEFT);
        MAP_BTN(WPAD_BUTTON_RIGHT,  BUTTON_RIGHT);
        MAP_BTN(WPAD_BUTTON_DOWN,   BUTTON_DOWN);
        MAP_BTN(WPAD_BUTTON_UP,     BUTTON_UP);
        MAP_BTN(WPAD_BUTTON_PLUS,   BUTTON_PLUS);
        MAP_BTN(WPAD_BUTTON_B,      BUTTON_A);
        MAP_BTN(WPAD_BUTTON_A,      BUTTON_B);
        MAP_BTN(WPAD_BUTTON_MINUS,  BUTTON_MINUS);
#undef MAP_BTN
    } else {
#define MAP_BTN(from, to) \
    do { \
        if (mStatus.classic.hold & from) mButtonsHeld |= to; \
        if (mStatus.classic.trigger & from) mButtonsTriggered |= to; \
    } while (0)
        MAP_BTN(WPAD_CLASSIC_BUTTON_LEFT,   BUTTON_LEFT);
        MAP_BTN(WPAD_CLASSIC_BUTTON_RIGHT,  BUTTON_RIGHT);
        MAP_BTN(WPAD_CLASSIC_BUTTON_DOWN,   BUTTON_DOWN);
        MAP_BTN(WPAD_CLASSIC_BUTTON_UP,     BUTTON_UP);
        MAP_BTN(WPAD_CLASSIC_BUTTON_PLUS,   BUTTON_PLUS);
        MAP_BTN(WPAD_CLASSIC_BUTTON_X,      BUTTON_X);
        MAP_BTN(WPAD_CLASSIC_BUTTON_Y,      BUTTON_Y);
        MAP_BTN(WPAD_CLASSIC_BUTTON_B,      BUTTON_B);
        MAP_BTN(WPAD_CLASSIC_BUTTON_A,      BUTTON_A);
        MAP_BTN(WPAD_CLASSIC_BUTTON_MINUS,  BUTTON_MINUS);
        MAP_BTN(WPAD_CLASSIC_BUTTON_ZR,     BUTTON_ZR);
        MAP_BTN(WPAD_CLASSIC_BUTTON_ZL,     BUTTON_ZL);
        MAP_BTN(WPAD_CLASSIC_BUTTON_R,      BUTTON_R);
        MAP_BTN(WPAD_CLASSIC_BUTTON_L,      BUTTON_L);
#undef MAP_BTN
    }

    return true;
}

bool KPADController::IsConnected() const
{
    return mIsConnected;
}

const std::string& KPADController::GetName() const
{
    return mName;
}

WPADExtensionType KPADController::GetExtensionType() const
{
    return mExtension;
}

bool KPADController::IsBloopairController() const
{
    return mIsBloopairController;
}

void KPADController::RetreiveControllerInformation()
{
    mIsBloopairController = false;
    if (mExtension == WPAD_EXT_PRO_CONTROLLER) {
        if (BloopairIPC::GetControllerInformation(mChannel, mControllerInfo)) {
            // Official controllers are not supported
            if (GetControllerType() != BLOOPAIR_CONTROLLER_OFFICIAL) {
                mIsBloopairController = true;
            }

            mName = GetNameForBloopairControllerType(GetControllerType()) + " ("
                + std::to_string(static_cast<int>(mChannel) + 1) + ")";
        } else {
            mName = "Unknown Pro Controller (" + std::to_string(static_cast<int>(mChannel) + 1) + ")";
        }
    } else {
        mName = GetNameForExtensionType(mExtension) + " (" + std::to_string(static_cast<int>(mChannel) + 1) + ")";
    }
}

BloopairControllerType KPADController::GetControllerType() const
{
    return (BloopairControllerType) mControllerInfo.controllerType;
}

uint16_t KPADController::GetVID() const
{
    return mControllerInfo.vendor_id;
}

uint16_t KPADController::GetPID() const
{
    return mControllerInfo.product_id;
}

void KPADController::Disconnect() const
{
    WPADDisconnect(mChannel);
}

std::array<uint8_t, 6> KPADController::GetBDA() const
{
    WPADAddress addr;
    WPADGetAddress(mChannel, &addr);

    std::array<uint8_t, 6> bda;
    std::copy(std::begin(addr.btDeviceAddress), std::end(addr.btDeviceAddress), bda.begin());
    return bda;
}

CombinedInputController::CombinedInputController() : Controller(),
    mName("Combined Controller")
{
}

CombinedInputController::~CombinedInputController()
{
}

bool CombinedInputController::Combine(const std::vector<const Controller*>& controllers)
{
    if (!Controller::Update()) {
        return false;
    }

    for (const Controller* c : controllers) {
        mButtonsHeld |= c->GetButtonsHeld();
        mButtonsTriggered |= c->GetButtonsTriggered();

        // TODO sticks
    }

    return true;
}

bool CombinedInputController::IsConnected() const
{
    return true;
}

const std::string& CombinedInputController::GetName() const
{
    return mName;
}
