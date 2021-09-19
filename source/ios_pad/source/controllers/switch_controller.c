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

typedef struct {
    uint8_t isProController;
    uint8_t isRightJoycon;
} SwitchData_t;

static uint8_t report_count = 0;

static const uint32_t dpad_map[9] = {
    WPAD_PRO_BUTTON_UP,
    WPAD_PRO_BUTTON_UP | WPAD_PRO_BUTTON_RIGHT,
    WPAD_PRO_BUTTON_RIGHT,
    WPAD_PRO_BUTTON_RIGHT | WPAD_PRO_BUTTON_DOWN,
    WPAD_PRO_BUTTON_DOWN,
    WPAD_PRO_BUTTON_DOWN | WPAD_PRO_BUTTON_LEFT,
    WPAD_PRO_BUTTON_LEFT,
    WPAD_PRO_BUTTON_LEFT | WPAD_PRO_BUTTON_UP,
    0,
};

#define AXIS_NORMALIZE_VALUE 3000

// These values are hardcoded based on https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md
void controllerRumble_switch(Controller_t* controller, uint8_t rumble)
{
    uint8_t data[10];
    _memset(data, 0, sizeof(data));
    data[0] = 0x10;
    data[1] = report_count++;

    if (rumble) {
        data[2] = (0x5000 >> 8) & 0xff;
        data[3] = (0x5000 & 0xFF) + 0x8a;
        data[4] = 0x34 + ((0x8062 >> 8) & 0xff);
        data[5] = 0x8062 & 0xff;

        data[6] = (0x5000 >> 8) & 0xff;
        data[7] = (0x5000 & 0xFF) + 0x8a;
        data[8] = 0x34 + ((0x8062 >> 8) & 0xff);
        data[9] = 0x8062 & 0xff;
    }
    else {
        data[2] = 0x00;
        data[3] = 0x01;
        data[4] = 0x40;
        data[5] = 0x40;

        data[6] = 0x00;
        data[7] = 0x01;
        data[8] = 0x40;
        data[9] = 0x40;
    }

    sendOutputData(controller->handle, data, sizeof(data));
}

void controllerSetLed_switch(Controller_t* controller, uint8_t led)
{
    // if this is the right joycon swap led order
    if (((SwitchData_t*) controller->additionalData)->isRightJoycon) {
        led = ((led & 1) << 3) | ((led & 2) << 1) | ((led & 4) >> 1) | ((led & 8) >> 3);
    }

    uint8_t data[12];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = report_count++;
    data[10] = 0x30;
    data[11] = led;
    sendOutputData(controller->handle, data, sizeof(data));
}

void controllerData_switch(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    if (controller->reportData && report_semaphore == -1) {
        return;
    }

    ReportBuffer_t* rep = controller->reportData;

    if (buf[0] == 0x3f) {
        int16_t left_stick_x = ((buf[5] << 8) | buf[4]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;
        int16_t right_stick_x = ((buf[9] << 8) | buf[8]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;
        int16_t left_stick_y = ((buf[7] << 8) | buf[6]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;
        int16_t right_stick_y = ((buf[11] << 8) | buf[10]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;

        // apparently that's something the pro controller does once paired
        // that completely messes with the start calibration, so drop that report
        if (left_stick_x < -1450) {
            return;
        }

        uint32_t buttons = 0;

        buttons |= dpad_map[buf[3] & 0xf];

        if (buf[1] & 0x01)
            buttons |= WPAD_PRO_BUTTON_B;
        if (buf[1] & 0x02)
            buttons |= WPAD_PRO_BUTTON_A;
        if (buf[1] & 0x04)
            buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[1] & 0x08)
            buttons |= WPAD_PRO_BUTTON_X;
        if (buf[1] & 0x10)
            buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[1] & 0x20)
            buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[1] & 0x40)
            buttons |= WPAD_PRO_TRIGGER_ZL;
        if (buf[1] & 0x80)
            buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[2] & 0x01)
            buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[2] & 0x02)
            buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[2] & 0x04)
            buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (buf[2] & 0x08)
            buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (buf[2] & 0x10)
            buttons |= WPAD_PRO_BUTTON_HOME;
        // capture button
        // if (buf[2] & 0x20)
        //     buttons |= WPAD_PRO_BUTTON_;

        if (controller->reportData) {
            IOS_WaitSemaphore(report_semaphore, 0);

            rep->buttons = buttons;
            rep->left_stick_x = left_stick_x;
            rep->right_stick_x = right_stick_x;
            rep->left_stick_y = left_stick_y;
            rep->right_stick_y = right_stick_y;

            IOS_SignalSempahore(report_semaphore);
        }
        else {
            sendControllerInput(controller, buttons, left_stick_x, right_stick_x, left_stick_y, right_stick_y);
        }
    }
}

void controllerDeinit_switch(Controller_t* controller)
{
    if (controller->reportData) {
        deinitContinuousReports(controller);
    }
}

void controllerInit_switch(Controller_t* controller, uint8_t isProController, uint8_t right_joycon)
{
    if (!isProController) {
        // The pro controller already sends continous reports but the joy-con don't
        initContinuousReports(controller);
    }

    controller->data = controllerData_switch;
    controller->setPlayerLed = controllerSetLed_switch;
    controller->rumble = controllerRumble_switch;
    controller->deinit = controllerDeinit_switch;

    controller->battery = 4;
    controller->isCharging = 0;

    SwitchData_t* data = (SwitchData_t*) IOS_Alloc(0xcaff, sizeof(SwitchData_t));
    controller->additionalData = data;

    data->isProController = isProController;
    data->isRightJoycon = right_joycon;
}
