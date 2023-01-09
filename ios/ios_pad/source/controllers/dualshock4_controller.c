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

#include "dualshock4_controller.h"

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
    Dualshock4Data* ds_data = (Dualshock4Data*) controller->additionalData;

    Dualshock4OutputReport rep;
    memset(&rep, 0, sizeof(rep));

    rep.report_id = DUALSHOCK4_OUTPUT_REPORT_ID;
    rep.report_mode = 0xc; // HID + CRC
    rep.report_rate = 10; // (1 / 10 ms) * 1000 = 100 Hz
    rep.flags = DUALSHOCK4_OUTPUT_FLAG_MOTOR | DUALSHOCK4_OUTPUT_FLAG_LED_COLOR;

    rep.motor_right = ds_data->rumble;
    rep.motor_left = ds_data->rumble;

    rep.led_red = ds_data->led_color[0];
    rep.led_green = ds_data->led_color[1];
    rep.led_blue = ds_data->led_color[2];

    // seed and calculate crc
    uint8_t seed = DUALSHOCK4_OUTPUT_REPORT_SEED;
    uint32_t crc = crc32(0xffffffff, &seed, sizeof(seed));
    crc = crc32(crc, &rep, sizeof(rep) - 4);
    rep.crc = bswap32(~crc);

    sendOutputData(controller->handle, &rep, sizeof(rep));
}

void controllerRumble_dualshock4(Controller* controller, uint8_t rumble)
{
    Dualshock4Data* ds_data = (Dualshock4Data*) controller->additionalData;

    ds_data->rumble = rumble ? 6 : 0;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualshock4(Controller* controller, uint8_t led)
{
    Dualshock4Data* ds_data = (Dualshock4Data*) controller->additionalData;
    
    uint8_t player_num = ledMaskToPlayerNum(led); 
    memcpy(ds_data->led_color, &led_colors[player_num], 3);

    sendRumbleLedState(controller);
}

void controllerData_dualshock4(Controller* controller, uint8_t* buf, uint16_t len)
{
    if (buf[0] == DUALSHOCK4_INPUT_REPORT_ID) {
        ReportBuffer* rep = &controller->reportBuffer;
        Dualshock4InputReport* inRep = (Dualshock4InputReport*) buf;

        rep->left_stick_x = scaleStickAxis(inRep->left_stick_x, 256);
        rep->left_stick_y = scaleStickAxis(inRep->left_stick_y, 256);
        rep->right_stick_x = scaleStickAxis(inRep->right_stick_x, 256);
        rep->right_stick_y = scaleStickAxis(inRep->right_stick_y, 256);

        rep->buttons = 0;

        if (inRep->buttons.dpad < 9)
            rep->buttons |= dpad_map[inRep->buttons.dpad];

        if (inRep->buttons.circle)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (inRep->buttons.cross)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (inRep->buttons.triangle)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (inRep->buttons.square)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (inRep->buttons.l1)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (inRep->buttons.r1)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (inRep->buttons.l2)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (inRep->buttons.r2)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (inRep->buttons.create)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (inRep->buttons.options)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (inRep->buttons.l3)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (inRep->buttons.r3)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (inRep->buttons.ps_home)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;

        controller->battery = CLAMP(inRep->battery_level >> 1, 0, 4);
        controller->isCharging = inRep->cable && inRep->battery_level <= 10;

        if (!controller->isReady) {
            controller->isReady = 1;
        }
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

    controller->additionalData = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(Dualshock4Data));
    memset(controller->additionalData, 0, sizeof(Dualshock4Data));
}
