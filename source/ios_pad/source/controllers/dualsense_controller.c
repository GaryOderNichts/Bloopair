/*
 *   Copyright (C) 2021 GaryOderNichts
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

#include <controllers.h>
#include "utils.h"

typedef struct {
    uint8_t player_leds;
    uint8_t led_color[3];
    uint8_t rumble;
} DualsenseData_t;

// the dualsense has 5 leds, with one in the center
// we'll use the same pattern the Wii U uses on the 4 outer ones
static const uint8_t player_leds[] = {
    0,
    0b00001,
    0b00010,
    0b01000,
    0b10000,
    0b00011,
    0b01001,
    0b10001,
};

static const uint8_t led_colors[][3] = {
    {0},
    // same colors as used on ps4/ps5
    {0x00, 0x00, 0x40}, // blue
    {0x40, 0x00, 0x00}, // red
    {0x00, 0x40, 0x00}, // green
    {0x20, 0x00, 0x20}, // pink
    // for 5-7 we'll use the same colors missioncontrol is using
    {0x00, 0x20, 0x20}, // cyan
    {0x30, 0x10, 0x00}, // orange
    {0x20, 0x20, 0x00}, // yellow
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
    DualsenseData_t* ds_data = (DualsenseData_t*) controller->additionalData;

    uint8_t data[79];
    memset(data, 0, sizeof(data));
    data[0] = 0xa2;
    data[1] = 0x31;
    data[2] = 0x02;
    data[3] = 0x03;
    data[4] = 0x14;
    data[5] = ds_data->rumble;
    data[6] = ds_data->rumble;
    data[41] = 0x02;
    data[44] = 0x02;
    data[46] = ds_data->player_leds;
    data[47] = ds_data->led_color[0];
    data[48] = ds_data->led_color[1];
    data[49] = ds_data->led_color[2];

    uint32_t crc = bswap32(~crc32(0xffffffff, data, sizeof(data) - 4));
    memcpy(&data[75], &crc, 4);

    sendOutputData(controller->handle, data + 1, sizeof(data) - 1);
}

void controllerRumble_dualsense(Controller_t* controller, uint8_t rumble)
{
    DualsenseData_t* ds_data = (DualsenseData_t*) controller->additionalData;

    ds_data->rumble = rumble ? 6 : 0;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualsense(Controller_t* controller, uint8_t led)
{
    DualsenseData_t* ds_data = (DualsenseData_t*) controller->additionalData;

    uint8_t player_num = ledMaskToPlayerNum(led); 

    ds_data->player_leds = player_leds[player_num];

    memcpy(ds_data->led_color, &led_colors[player_num], 3);

    sendRumbleLedState(controller);
}

void controllerData_dualsense(Controller_t* controller, uint8_t* buf, uint16_t len)
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
    else if (buf[0] == 0x31) {
        rep->left_stick_x = (buf[2] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->left_stick_y = (buf[3] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_x = (buf[4] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_y = (buf[5] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;

        rep->buttons = 0;

        if ((buf[9] & 0xf) < 9)
            rep->buttons |= dpad_map[buf[9] & 0xf];

        if (buf[9] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[9] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[9] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[9] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[10] & 0x01)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[10] & 0x02)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[10] & 0x04)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (buf[10] & 0x08)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[10] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[10] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[10] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (buf[10] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (buf[11] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;

        uint8_t battery_level = (buf[54] & 0xf) >> 1;
        uint8_t battery_status = buf[54] >> 4;
        if (battery_status == 0) { // discharging
            controller->battery = CLAMP(battery_level, 0, 4);
            controller->isCharging = 0;
        }
        else if (battery_status == 1) { // charging
            controller->battery = CLAMP(battery_level, 0, 4);
            controller->isCharging = 1;
        }
        else if (battery_status == 2) { // full
            controller->battery = 4;
            controller->isCharging = 0;
        }
        else { // everything else indicates an error
            controller->battery = 0;
            controller->isCharging = 0;
        }
    }
}

void controllerDeinit_dualsense(Controller_t* controller)
{
    deinitContinuousReports(controller);

    IOS_Free(0xcaff, controller->additionalData);
}

void controllerInit_dualsense(Controller_t* controller)
{
    initContinuousReports(controller);

    controller->data = controllerData_dualsense;
    controller->setPlayerLed = controllerSetLed_dualsense;
    controller->rumble = controllerRumble_dualsense;
    controller->deinit = controllerDeinit_dualsense;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->additionalData = IOS_Alloc(0xcaff, sizeof(DualsenseData_t));
    memset(controller->additionalData, 0, sizeof(DualsenseData_t));
}
