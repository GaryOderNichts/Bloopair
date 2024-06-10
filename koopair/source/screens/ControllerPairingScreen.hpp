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

#include <memory>
#include <functional>
#include <nsyshid/hid.h>

class MessageBox;

class ControllerPairingScreen : public Screen
{
public:
    ControllerPairingScreen();
    virtual ~ControllerPairingScreen();

    void Draw();

    bool Update(const CombinedInputController& input);

private:
    static int32_t HidAttachCallback(HIDClient* client, HIDDevice* device, HIDAttachEvent event);
    int32_t OnHidDeviceAttached(HIDClient* client, HIDDevice* device, HIDAttachEvent event);

    std::unique_ptr<MessageBox> mMessageBox;

    std::array<uint8_t, 6> mConsoleBDA;

    struct PairingClient {
        HIDClient hidClient;
        std::function<int32_t(HIDClient*, HIDDevice*, HIDAttachEvent)> callback;
    } mPairingClient;
};
