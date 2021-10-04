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
#include "utils.h"

typedef struct {
    uint8_t led_color[3];
    uint8_t rumble;
} Dualshock4Data_t;

static const uint8_t led_colors[][3] = {
    {0},
    {0x00, 0x00, 0x40}, // player 1
    {0x40, 0x00, 0x00}, // player 2
    {0},
    {0x00, 0x40, 0x00}, // player 3
    {0},
    {0},
    {0},
    {0x20, 0x00, 0x20}, // player 4
};

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

#define AXIS_NORMALIZE_VALUE (1140 * 2)

static void sendRumbleLedState(Controller_t* controller)
{
    Dualshock4Data_t* ds_data = (Dualshock4Data_t*) controller->additionalData;

    uint8_t data[79];
    _memset(data, 0, sizeof(data));
    data[0] = 0xa2;
    data[1] = 0x11; // report id
    data[2] = 0xc8; // report rate (125Hz) 
    data[3] = 0x20; 
    data[4] = 0xf3;
    data[5] = 0x04;
    data[7] = ds_data->rumble;
    data[8] = ds_data->rumble;
    data[9] = ds_data->led_color[0];
    data[10] = ds_data->led_color[1];
    data[11] = ds_data->led_color[2];

    uint32_t crc = bswap32(~crc32(0xffffffff, data, sizeof(data) - 4));
    _memcpy(&data[75], &crc, 4);

    sendOutputData(controller->handle, data + 1, sizeof(data) - 1);
}

void controllerRumble_dualshock4(Controller_t* controller, uint8_t rumble)
{
    Dualshock4Data_t* ds_data = (Dualshock4Data_t*) controller->additionalData;

    ds_data->rumble = rumble ? 6 : 0;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualshock4(Controller_t* controller, uint8_t led)
{
    Dualshock4Data_t* ds_data = (Dualshock4Data_t*) controller->additionalData;
    
    _memcpy(ds_data->led_color, &led_colors[led], 3);

    sendRumbleLedState(controller);
}

void controllerData_dualshock4(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    ReportBuffer_t* rep = controller->reportData;

    if (buf[0] == 0x01) {
        rep->left_stick_x = (buf[1] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->left_stick_y = (buf[2] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_x = (buf[3] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_y = (buf[4] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;

        rep->buttons = 0;

        if ((buf[5] & 0xf) < 9)
            rep->buttons |= dpad_map[buf[5] & 0xf];

        if (buf[5] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[5] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[5] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[5] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[6] & 0x01)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[6] & 0x02)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[6] & 0x04)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (buf[6] & 0x08)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[6] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[6] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[6] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (buf[6] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (buf[7] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
    }
    else if (buf[0] == 0x11) {
        rep->left_stick_x = (buf[3] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->left_stick_y = (buf[4] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_x = (buf[5] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_y = (buf[6] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;

        rep->buttons = 0;

        if ((buf[7] & 0xf) < 9)
            rep->buttons |= dpad_map[buf[7] & 0xf];

        if (buf[7] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[7] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[7] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[7] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[8] & 0x01)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[8] & 0x02)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[8] & 0x04)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (buf[8] & 0x08)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[8] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[8] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[8] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (buf[8] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (buf[9] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;

        uint8_t battery_level = (buf[32] & 0xf) >> 1;
        controller->battery = battery_level > 4 ? 4 : battery_level;
        controller->isCharging = (buf[32] & 0x10) && (buf[32] & 0xf) <= 10;
    }
}

void controllerDeinit_dualshock4(Controller_t* controller)
{
    deinitContinuousReports(controller);

    IOS_Free(0xcaff, controller->additionalData);
}

void controllerInit_dualshock4(Controller_t* controller)
{
    initContinuousReports(controller);

    controller->data = controllerData_dualshock4;
    controller->setPlayerLed = controllerSetLed_dualshock4;
    controller->rumble = controllerRumble_dualshock4;
    controller->deinit = controllerDeinit_dualshock4;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->additionalData = IOS_Alloc(0xcaff, sizeof(Dualshock4Data_t));
    _memset(controller->additionalData, 0, sizeof(Dualshock4Data_t));
}
