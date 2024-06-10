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
#include <bloopair/controllers/dualshock4_controller.h>

static const MappingConfiguration default_dualshock4_mapping = {
    .num = 26,
    .mappings = {
        { BLOOPAIR_PRO_STICK_L_UP,      BLOOPAIR_PRO_STICK_L_UP, },
        { BLOOPAIR_PRO_STICK_L_DOWN,    BLOOPAIR_PRO_STICK_L_DOWN, },
        { BLOOPAIR_PRO_STICK_L_LEFT,    BLOOPAIR_PRO_STICK_L_LEFT, },
        { BLOOPAIR_PRO_STICK_L_RIGHT,   BLOOPAIR_PRO_STICK_L_RIGHT, },

        { BLOOPAIR_PRO_STICK_R_UP,      BLOOPAIR_PRO_STICK_R_UP, },
        { BLOOPAIR_PRO_STICK_R_DOWN,    BLOOPAIR_PRO_STICK_R_DOWN, },
        { BLOOPAIR_PRO_STICK_R_LEFT,    BLOOPAIR_PRO_STICK_R_LEFT, },
        { BLOOPAIR_PRO_STICK_R_RIGHT,   BLOOPAIR_PRO_STICK_R_RIGHT, },

        { DUALSHOCK4_BUTTON_DOWN,       BLOOPAIR_PRO_BUTTON_DOWN, },
        { DUALSHOCK4_BUTTON_UP,         BLOOPAIR_PRO_BUTTON_UP },
        { DUALSHOCK4_BUTTON_RIGHT,      BLOOPAIR_PRO_BUTTON_RIGHT, },
        { DUALSHOCK4_BUTTON_LEFT,       BLOOPAIR_PRO_BUTTON_LEFT, },

        { DUALSHOCK4_BUTTON_CIRCLE,     BLOOPAIR_PRO_BUTTON_A, },
        { DUALSHOCK4_BUTTON_CROSS,      BLOOPAIR_PRO_BUTTON_B, },
        { DUALSHOCK4_BUTTON_TRIANGLE,   BLOOPAIR_PRO_BUTTON_X, },
        { DUALSHOCK4_BUTTON_SQUARE,     BLOOPAIR_PRO_BUTTON_Y, },

        { DUALSHOCK4_TRIGGER_L1,        BLOOPAIR_PRO_TRIGGER_L, },
        { DUALSHOCK4_TRIGGER_R1,        BLOOPAIR_PRO_TRIGGER_R, },
        { DUALSHOCK4_TRIGGER_L2,        BLOOPAIR_PRO_TRIGGER_ZL, },
        { DUALSHOCK4_TRIGGER_R2,        BLOOPAIR_PRO_TRIGGER_ZR, },

        { DUALSHOCK4_BUTTON_CREATE,     BLOOPAIR_PRO_BUTTON_MINUS, },
        { DUALSHOCK4_BUTTON_OPTIONS,    BLOOPAIR_PRO_BUTTON_PLUS, },
        { DUALSHOCK4_BUTTON_L3,         BLOOPAIR_PRO_BUTTON_STICK_L, },
        { DUALSHOCK4_BUTTON_R3,         BLOOPAIR_PRO_BUTTON_STICK_R, },

        { DUALSHOCK4_BUTTON_PS_HOME,    BLOOPAIR_PRO_BUTTON_HOME, },

        // Map the touchpad to the reserved button bit
        { DUALSHOCK4_BUTTON_TOUCHPAD,   BLOOPAIR_PRO_RESERVED, },
    },
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
    BTN(DUALSHOCK4_BUTTON_UP),
    BTN(DUALSHOCK4_BUTTON_UP)    | BTN(DUALSHOCK4_BUTTON_RIGHT),
    BTN(DUALSHOCK4_BUTTON_RIGHT),
    BTN(DUALSHOCK4_BUTTON_RIGHT) | BTN(DUALSHOCK4_BUTTON_DOWN),
    BTN(DUALSHOCK4_BUTTON_DOWN),
    BTN(DUALSHOCK4_BUTTON_DOWN)  | BTN(DUALSHOCK4_BUTTON_LEFT),
    BTN(DUALSHOCK4_BUTTON_LEFT),
    BTN(DUALSHOCK4_BUTTON_LEFT)  | BTN(DUALSHOCK4_BUTTON_UP),
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
        BloopairReportBuffer* rep = &controller->reportBuffer;
        Dualshock4InputReport* inRep = (Dualshock4InputReport*) buf;

        rep->left_stick_x = scaleStickAxis(inRep->left_stick_x, 256);
        rep->left_stick_y = scaleStickAxis(inRep->left_stick_y, 256);
        rep->right_stick_x = scaleStickAxis(inRep->right_stick_x, 256);
        rep->right_stick_y = scaleStickAxis(inRep->right_stick_y, 256);

        rep->buttons = 0;

        if (inRep->buttons.dpad < 9)
            rep->buttons |= dpad_map[inRep->buttons.dpad];

        if (inRep->buttons.circle)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_CIRCLE);
        if (inRep->buttons.cross)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_CROSS);
        if (inRep->buttons.triangle)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_TRIANGLE);
        if (inRep->buttons.square)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_SQUARE);
        if (inRep->buttons.l1)
            rep->buttons |= BTN(DUALSHOCK4_TRIGGER_L1);
        if (inRep->buttons.r1)
            rep->buttons |= BTN(DUALSHOCK4_TRIGGER_R1);
        if (inRep->buttons.l2)
            rep->buttons |= BTN(DUALSHOCK4_TRIGGER_L2);
        if (inRep->buttons.r2)
            rep->buttons |= BTN(DUALSHOCK4_TRIGGER_R2);
        if (inRep->buttons.create)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_CREATE);
        if (inRep->buttons.options)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_OPTIONS);
        if (inRep->buttons.l3)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_L3);
        if (inRep->buttons.r3)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_R3);
        if (inRep->buttons.touchpad)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_TOUCHPAD);
        if (inRep->buttons.ps_home)
            rep->buttons |= BTN(DUALSHOCK4_BUTTON_PS_HOME);

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

    controller->type = BLOOPAIR_CONTROLLER_DUALSHOCK3;
    Configuration_GetAll(controller->type, controller->bda,
        &controller->commonConfig, &controller->mapping,
        &controller->customConfig, &controller->customConfigSize);
}

void controllerModuleInit_dualshock4(void)
{
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_DUALSHOCK4, NULL, &default_dualshock4_mapping, NULL, 0);
}
