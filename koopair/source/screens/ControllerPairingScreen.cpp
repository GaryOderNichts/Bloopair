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
#include "ControllerPairingScreen.hpp"
#include "Gfx.hpp"
#include "BloopairIPC.hpp"
#include "MessageBox.hpp"
#include "Utils.hpp"

namespace
{

// Info about the reports can be found here:
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-sony.c>

#define DS3_VID 0x054c
#define DS3_PID 0x0268

#define HID_REPORT_FEATURE 3

int ds3ReadBDA(uint32_t handle, uint8_t* outBDA)
{
    __attribute__ ((aligned (0x20))) uint8_t buf[17]{};
    int res = HIDGetReport(handle, HID_REPORT_FEATURE, 0xf2, buf, sizeof(buf), nullptr, nullptr);
    if (res >= 0) {
        memcpy(outBDA, buf + 4, 6);
    }

    return res;
}

int ds3WriteMasterBDA(uint32_t handle, uint8_t* bda)
{
    __attribute__ ((aligned (0x20))) uint8_t buf[8]{};
    memcpy(buf + 2, bda, 6);
    return HIDSetReport(handle, HID_REPORT_FEATURE, 0xf5, buf, sizeof(buf), nullptr, nullptr);
}

}

ControllerPairingScreen::ControllerPairingScreen()
 : mMessageBox(),
   mConsoleBDA(),
   mPairingClient()
{
    // TODO error handling
    auto bda = BloopairIPC::ReadConsoleBDA();
    if (bda) {
        mConsoleBDA = *bda;
    }

    mPairingClient.callback = std::bind(&ControllerPairingScreen::OnHidDeviceAttached, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    HIDAddClient(&mPairingClient.hidClient, ControllerPairingScreen::HidAttachCallback);
}

ControllerPairingScreen::~ControllerPairingScreen()
{
    HIDDelClient(&mPairingClient.hidClient);
}

void ControllerPairingScreen::Draw()
{
    DrawTopBar("Controller Pairing");

    Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 60, Gfx::COLOR_TEXT,
        "Connect a DualShock 3 using a USB cable\nto any USB port of the system to pair it.\n"
        "To pair a wireless controller\npress the sync button on the console and controller.",
        Gfx::ALIGN_CENTER);

    DrawBottomBar("\ue001 Back", nullptr, nullptr);

    if (mMessageBox) {
        mMessageBox->Draw();
    }
}

bool ControllerPairingScreen::Update(const CombinedInputController& input)
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

    return true;
}

int32_t ControllerPairingScreen::HidAttachCallback(HIDClient* client, HIDDevice* device, HIDAttachEvent event)
{
    PairingClient* pairingClient = reinterpret_cast<PairingClient*>(client);
    return pairingClient->callback(client, device, event);
}

int32_t ControllerPairingScreen::OnHidDeviceAttached(HIDClient* client, HIDDevice* device, HIDAttachEvent event)
{
    if (event == HID_DEVICE_ATTACH) {
        if (__builtin_bswap16(device->vid) == DS3_VID && __builtin_bswap16(device->pid) == DS3_PID) {
            uint8_t bda[6]{};
            int res = ds3ReadBDA(device->handle, bda);
            if (res < 0) {
                mMessageBox = std::make_unique<MessageBox>(
                    "Failed to read BDA.",
                    "Failed to read the BDA of the controller.",
                    std::vector{
                        MessageBox::Option{0, "\ue000 Ok", [this]() {} },
                    }
                );
                return HID_DEVICE_DETACH;
            }

            res = ds3WriteMasterBDA(device->handle, mConsoleBDA.data());
            if (res < 0) {
                mMessageBox = std::make_unique<MessageBox>(
                    "Failed to write master BDA.",
                    "Failed to write the master BDA of the controller.",
                    std::vector{
                        MessageBox::Option{0, "\ue000 Ok", [this]() {} },
                    }
                );
                return HID_DEVICE_DETACH;
            }

            uint8_t link_key[16]{};
            BloopairIPC::AddControllerPairing(bda, 
                link_key, // we'll bypass security for a ds3 anyways, so just add an empty link key
                "Nintendo RVL-CNT-01-UC", // use the pro controller name
                DS3_VID, DS3_PID
            );

            mMessageBox = std::make_unique<MessageBox>(
                "Successfully paired controller!",
                Utils::sprintf(
                    "Paired DualShock 3 (%02x:%02x:%02x:%02x:%02x:%02x).\n\n"
                    "You can now remove the cable from the controller\n"
                    "and use it wirelessly.\n"
                    "You don't need to redo this pairing process again.",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]
                ),
                std::vector{
                    MessageBox::Option{0, "\ue000 Ok", [this]() {} },
                }
            );
        }
    }

    return HID_DEVICE_DETACH;
}
