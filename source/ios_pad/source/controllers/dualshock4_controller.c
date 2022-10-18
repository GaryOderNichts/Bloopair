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

// Information about the reports can be found here:
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-sony.c>

typedef struct {
    uint8_t led_color[3];
    uint8_t rumble;
} Dualshock4Data_t;

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

static void sendRumbleLedState(Controller* controller)
{
    Dualshock4Data_t* ds_data = (Dualshock4Data_t*) controller->additionalData;

    uint8_t data[79];
    memset(data, 0, sizeof(data));
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
    memcpy(&data[75], &crc, 4);

    sendOutputData(controller->handle, data + 1, sizeof(data) - 1);
}

void controllerRumble_dualshock4(Controller* controller, uint8_t rumble)
{
    Dualshock4Data_t* ds_data = (Dualshock4Data_t*) controller->additionalData;

    ds_data->rumble = rumble ? 6 : 0;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualshock4(Controller* controller, uint8_t led)
{
    Dualshock4Data_t* ds_data = (Dualshock4Data_t*) controller->additionalData;
    
    uint8_t player_num = ledMaskToPlayerNum(led); 
    memcpy(ds_data->led_color, &led_colors[player_num], 3);

    sendRumbleLedState(controller);
}

void controllerData_dualshock4(Controller* controller, uint8_t* buf, uint16_t len)
{
    ReportBuffer* rep = &controller->reportBuffer;

    if (buf[0] == 0x01) {
        rep->left_stick_x = scaleStickAxis(buf[1], 256);
        rep->left_stick_y = scaleStickAxis(buf[2], 256);
        rep->right_stick_x = scaleStickAxis(buf[3], 256);
        rep->right_stick_y = scaleStickAxis(buf[4], 256);

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

        if (!controller->isReady)
            controller->isReady = 1;
    } else if (buf[0] == 0x11) {
        rep->left_stick_x = scaleStickAxis(buf[3], 256);
        rep->left_stick_y = scaleStickAxis(buf[4], 256);
        rep->right_stick_x = scaleStickAxis(buf[5], 256);
        rep->right_stick_y = scaleStickAxis(buf[6], 256);

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

        uint8_t battery_level = buf[32] & 0xf;
        controller->battery = CLAMP(battery_level >> 1, 0, 4);
        controller->isCharging = (buf[32] & 0x10) && battery_level <= 10;

        if (!controller->isReady)
            controller->isReady = 1;
    }
}

void controllerDeinit_dualshock4(Controller* controller)
{
    IOS_Free(LOCAL_PROCESS_HEAP_ID, controller->additionalData);
}

void controllerInit_dualshock4(Controller* controller)
{
    controller->data = controllerData_dualshock4;
    controller->setPlayerLed = controllerSetLed_dualshock4;
    controller->rumble = controllerRumble_dualshock4;
    controller->deinit = controllerDeinit_dualshock4;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->additionalData = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(Dualshock4Data_t));
    memset(controller->additionalData, 0, sizeof(Dualshock4Data_t));
}
