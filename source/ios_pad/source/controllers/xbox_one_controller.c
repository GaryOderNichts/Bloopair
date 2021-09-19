/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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

#include <controllers.h>

static const uint32_t dpad_map[9] = {
    0,
    WPAD_PRO_BUTTON_UP,
    WPAD_PRO_BUTTON_UP | WPAD_PRO_BUTTON_RIGHT,
    WPAD_PRO_BUTTON_RIGHT,
    WPAD_PRO_BUTTON_RIGHT | WPAD_PRO_BUTTON_DOWN,
    WPAD_PRO_BUTTON_DOWN,
    WPAD_PRO_BUTTON_DOWN | WPAD_PRO_BUTTON_LEFT,
    WPAD_PRO_BUTTON_LEFT,
    WPAD_PRO_BUTTON_LEFT | WPAD_PRO_BUTTON_UP
};

enum {
    RUMBLE_MOTOR_RIGHT         = 1 << 0,
    RUMBLE_MOTOR_LEFT          = 1 << 1,
    RUMBLE_MOTOR_TRIGGER_RIGHT = 1 << 2,
    RUMBLE_MOTOR_TRIGGER_LEFT  = 1 << 3,
};

#define AXIS_NORMALIZE_VALUE (1140 * 2)

void controllerData_xbox_one(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    if (report_semaphore == -1) {
        return;
    }

    IOS_WaitSemaphore(report_semaphore, 0);

    ReportBuffer_t* rep = controller->reportData;

    if (buf[0] == 0x01) {
        rep->left_stick_x = (((buf[2] << 8) | buf[1]) - 65536 / 2) * AXIS_NORMALIZE_VALUE / 65536;
        rep->left_stick_y = (((buf[4] << 8) | buf[3]) - 65536 / 2) * AXIS_NORMALIZE_VALUE / 65536;
        rep->right_stick_x = (((buf[6] << 8) | buf[5]) - 65536 / 2) * AXIS_NORMALIZE_VALUE / 65536;
        rep->right_stick_y = (((buf[8] << 8) | buf[7]) - 65536 / 2) * AXIS_NORMALIZE_VALUE / 65536;

        // clear all buttons besides home
        rep->buttons &= WPAD_PRO_BUTTON_HOME;

        rep->buttons |= dpad_map[buf[13] & 0xf];

        if (buf[10])
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (buf[12])
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;

        if (len >= 17) {
            // new format
            if (buf[14] & 0x01)
                rep->buttons |= WPAD_PRO_BUTTON_B;
            if (buf[14] & 0x02)
                rep->buttons |= WPAD_PRO_BUTTON_A;
            if (buf[14] & 0x08)
                rep->buttons |= WPAD_PRO_BUTTON_Y;
            if (buf[14] & 0x10)
                rep->buttons |= WPAD_PRO_BUTTON_X;
            if (buf[14] & 0x40)
                rep->buttons |= WPAD_PRO_TRIGGER_L;
            if (buf[14] & 0x80)
                rep->buttons |= WPAD_PRO_TRIGGER_R;
            if (buf[15] & 0x08)
                rep->buttons |= WPAD_PRO_BUTTON_PLUS;
            if (buf[15] & 0x10)
                rep->buttons |= WPAD_PRO_BUTTON_HOME;
            else
                rep->buttons &= ~WPAD_PRO_BUTTON_HOME;
            if (buf[15] & 0x20)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
            if (buf[15] & 0x40)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
            if (buf[16] & 0x01)
                rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        }
        else {
            // old format
            if (buf[14] & 0x01)
                rep->buttons |= WPAD_PRO_BUTTON_B;
            if (buf[14] & 0x02)
                rep->buttons |= WPAD_PRO_BUTTON_A;
            if (buf[14] & 0x04)
                rep->buttons |= WPAD_PRO_BUTTON_Y;
            if (buf[14] & 0x08)
                rep->buttons |= WPAD_PRO_BUTTON_X;
            if (buf[14] & 0x10)
                rep->buttons |= WPAD_PRO_TRIGGER_L;
            if (buf[14] & 0x20)
                rep->buttons |= WPAD_PRO_TRIGGER_R;
            if (buf[14] & 0x40)
                rep->buttons |= WPAD_PRO_BUTTON_MINUS;
            if (buf[14] & 0x80)
                rep->buttons |= WPAD_PRO_BUTTON_PLUS;
            if (buf[15] & 0x01)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
            if (buf[15] & 0x02)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        }
    }
    else if (buf[0] == 0x02) {
        if (buf[1] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        else
            rep->buttons &= ~WPAD_PRO_BUTTON_HOME;
    }
    else if (buf[0] == 0x04) {
        controller->battery = (buf[1] & 0x3) + 1;
        controller->isCharging = buf[1] & 0x10;
    }

    IOS_SignalSempahore(report_semaphore);
}

void controllerRumble_xbox_one(Controller_t* controller, uint8_t rumble)
{
    if (rumble) {
        uint8_t data[9];
        data[0] = 0x03;
        data[1] = RUMBLE_MOTOR_RIGHT | RUMBLE_MOTOR_LEFT; // motors
        data[2] = 0; // left trigger force
        data[3] = 0; // right trigger force
        data[4] = 35; // left force
        data[5] = 35; // right force
        data[6] = 1; // duration
        data[7] = 0; // delay
        data[8] = 0; // loop

        sendOutputData(controller->handle, data, sizeof(data));
    }
}

void controllerDeinit_xbox_one(Controller_t* controller)
{
    deinitContinuousReports(controller);
}

void controllerInit_xbox_one(Controller_t* controller)
{
    initContinuousReports(controller);

    controller->setPlayerLed = NULL;
    controller->rumble = controllerRumble_xbox_one;
    controller->data = controllerData_xbox_one;
    controller->deinit = controllerDeinit_xbox_one;

    controller->battery = 4;
    controller->isCharging = 0;
}
