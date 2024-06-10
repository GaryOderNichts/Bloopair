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

#include <string>
#include <vector>
#include <array>

#include <vpad/input.h>
#include <padscore/kpad.h>
#include <bloopair/bloopair.h>

class Controller {
public:
    Controller();
    virtual ~Controller();

    enum Buttons {
        BUTTON_NONE     = 0,
        BUTTON_A        = 1 << 0,
        BUTTON_B        = 1 << 1,
        BUTTON_X        = 1 << 2,
        BUTTON_Y        = 1 << 3,
        BUTTON_LEFT     = 1 << 4,
        BUTTON_RIGHT    = 1 << 5,
        BUTTON_UP       = 1 << 6,
        BUTTON_DOWN     = 1 << 7,
        BUTTON_ZL       = 1 << 8,
        BUTTON_ZR       = 1 << 9,
        BUTTON_L        = 1 << 10,
        BUTTON_R        = 1 << 11,
        BUTTON_PLUS     = 1 << 12,
        BUTTON_MINUS    = 1 << 13,
        BUTTON_STICK_L  = 1 << 14,
        BUTTON_STICK_R  = 1 << 15,
    };

    struct Stick {
        float x;
        float y;
    };

    virtual bool Update();

    virtual bool IsConnected() const = 0;

    virtual const std::string& GetName() const = 0;

    Buttons GetButtonsTriggered() const
    {
        return mButtonsTriggered;
    }

    Buttons GetButtonsHeld() const
    {
        return mButtonsHeld;
    }

    const Stick& GetStickL() const
    {
        return mStickL;
    }

    const Stick& GetStickR() const
    {
        return mStickR;
    }

protected:
    Buttons mButtonsTriggered;
    Buttons mButtonsHeld;

    Stick mStickL;
    Stick mStickR;
};

inline Controller::Buttons operator|=(Controller::Buttons& lhs, Controller::Buttons rhs)
{
    lhs = static_cast<Controller::Buttons>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    return lhs;
}

class VPADController : public Controller {
public:
    VPADController(VPADChan chan);
    ~VPADController();

    virtual bool Update() override;

    virtual bool IsConnected() const override;

    virtual const std::string& GetName() const override;

    VPADChan GetChannel() const
    {
        return mChannel;
    }

    const VPADStatus& GetStatus() const
    {
        return mStatus;
    }

private:
    bool mIsConnected;
    std::string mName;

    VPADChan mChannel;
    VPADStatus mStatus;
};

class KPADController : public Controller {
public:
    KPADController(KPADChan chan);
    ~KPADController();

    virtual bool Update() override;

    virtual bool IsConnected() const override;

    virtual const std::string& GetName() const override;

    WPADExtensionType GetExtensionType() const;

    bool IsBloopairController() const;

    BloopairControllerType GetControllerType() const;

    uint16_t GetVID() const;

    uint16_t GetPID() const;

    void Disconnect() const;

    std::array<uint8_t, 6> GetBDA() const;

    KPADChan GetChannel() const
    {
        return mChannel;
    }

    const KPADStatus& GetStatus() const
    {
        return mStatus;
    }

private:
    void RetreiveControllerInformation();

    bool mIsConnected;
    uint32_t mRetrieveTimer;
    std::string mName;

    KPADChan mChannel;
    WPADExtensionType mExtension;
    KPADStatus mStatus;

    bool mIsBloopairController;
    BloopairControllerInformationData mControllerInfo;
};

class CombinedInputController : public Controller {
public:
    CombinedInputController();
    ~CombinedInputController();

    bool Combine(const std::vector<const Controller*>& controllers);

    virtual bool IsConnected() const override;

    virtual const std::string& GetName() const override;

private:
    std::string mName;
};
