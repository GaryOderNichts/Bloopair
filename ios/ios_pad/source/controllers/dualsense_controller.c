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

#include "dualsense_controller.h"
#include <bloopair/controllers/dualsense_controller.h>

static const MappingConfiguration default_dualsense_mapping = {
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

        { DUALSENSE_BUTTON_DOWN,        BLOOPAIR_PRO_BUTTON_DOWN, },
        { DUALSENSE_BUTTON_UP,          BLOOPAIR_PRO_BUTTON_UP },
        { DUALSENSE_BUTTON_RIGHT,       BLOOPAIR_PRO_BUTTON_RIGHT, },
        { DUALSENSE_BUTTON_LEFT,        BLOOPAIR_PRO_BUTTON_LEFT, },

        { DUALSENSE_BUTTON_CIRCLE,      BLOOPAIR_PRO_BUTTON_A, },
        { DUALSENSE_BUTTON_CROSS,       BLOOPAIR_PRO_BUTTON_B, },
        { DUALSENSE_BUTTON_TRIANGLE,    BLOOPAIR_PRO_BUTTON_X, },
        { DUALSENSE_BUTTON_SQUARE,      BLOOPAIR_PRO_BUTTON_Y, },

        { DUALSENSE_TRIGGER_L1,         BLOOPAIR_PRO_TRIGGER_L, },
        { DUALSENSE_TRIGGER_R1,         BLOOPAIR_PRO_TRIGGER_R, },
        { DUALSENSE_TRIGGER_L2,         BLOOPAIR_PRO_TRIGGER_ZL, },
        { DUALSENSE_TRIGGER_R2,         BLOOPAIR_PRO_TRIGGER_ZR, },

        { DUALSENSE_BUTTON_CREATE,      BLOOPAIR_PRO_BUTTON_MINUS, },
        { DUALSENSE_BUTTON_OPTIONS,     BLOOPAIR_PRO_BUTTON_PLUS, },

        { DUALSENSE_BUTTON_L3,          BLOOPAIR_PRO_BUTTON_STICK_L, },
        { DUALSENSE_BUTTON_R3,          BLOOPAIR_PRO_BUTTON_STICK_R, },

        { DUALSENSE_BUTTON_PS_HOME,     BLOOPAIR_PRO_BUTTON_HOME, },

        // Map the touchpad to the reserved button bit
        { DUALSENSE_BUTTON_TOUCHPAD,    BLOOPAIR_PRO_RESERVED, },
    },
};

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
    BTN(DUALSENSE_BUTTON_UP),
    BTN(DUALSENSE_BUTTON_UP)    | BTN(DUALSENSE_BUTTON_RIGHT),
    BTN(DUALSENSE_BUTTON_RIGHT),
    BTN(DUALSENSE_BUTTON_RIGHT) | BTN(DUALSENSE_BUTTON_DOWN),
    BTN(DUALSENSE_BUTTON_DOWN),
    BTN(DUALSENSE_BUTTON_DOWN)  | BTN(DUALSENSE_BUTTON_LEFT),
    BTN(DUALSENSE_BUTTON_LEFT),
    BTN(DUALSENSE_BUTTON_LEFT)  | BTN(DUALSENSE_BUTTON_UP),
    0,
};

static void sendRumbleLedState(Controller* controller)
{
    DualsenseData* ds_data = (DualsenseData*) controller->additionalData;

    DualsenseOutputReport rep;
    memset(&rep, 0, sizeof(rep));
    rep.report_id = DUALSENSE_OUTPUT_REPORT_ID;
    rep.seq_tag = (ds_data->output_seq++ & 0xf) << 4;
    rep.tag = DUALSENSE_OUTPUT_REPORT_TAG;

    rep.valid_flag0 = DUALSENSE_VF0_COMPATIBLE_VIBRATION | DUALSENSE_VF0_HAPTICS_SELECT;
    rep.valid_flag1 = DUALSENSE_VF1_LIGHTBAR_CONTROL_ENABLE | DUALSENSE_VF1_PLAYER_INDICATOR_CONTROL_ENABLE;
    rep.motor_right = ds_data->rumble;
    rep.motor_left = ds_data->rumble;

    rep.valid_flag2 = DUALSENSE_VF2_COMPATIBLE_VIBRATION2;
    rep.lightbar_setup = DUALSENSE_LIGHTBAR_SETUP_LIGHT_OUT;
    rep.player_leds = ds_data->player_leds;
    rep.lightbar_red = ds_data->led_color[0];
    rep.lightbar_green = ds_data->led_color[1];
    rep.lightbar_blue = ds_data->led_color[2];

    // seed and calculate crc
    uint8_t seed = DUALSENSE_OUTPUT_REPORT_SEED;
    uint32_t crc = crc32(0xffffffff, &seed, sizeof(seed));
    crc = crc32(crc, &rep, sizeof(rep) - 4);
    rep.crc = bswap32(~crc);

    sendOutputData(controller->handle, &rep, sizeof(rep));
}

void controllerRumble_dualsense(Controller* controller, uint8_t rumble)
{
    DualsenseData* ds_data = (DualsenseData*) controller->additionalData;

    ds_data->rumble = rumble ? 25 : 0;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualsense(Controller* controller, uint8_t led)
{
    DualsenseData* ds_data = (DualsenseData*) controller->additionalData;

    uint8_t player_num = ledMaskToPlayerNum(led); 
    ds_data->player_leds = player_leds[player_num];
    memcpy(ds_data->led_color, &led_colors[player_num], 3);

    sendRumbleLedState(controller);
}

void controllerData_dualsense(Controller* controller, uint8_t* buf, uint16_t len)
{
    if (buf[0] == DUALSENSE_INPUT_REPORT_ID) {
        BloopairReportBuffer* rep = &controller->reportBuffer;
        DualsenseInputReport* inRep = (DualsenseInputReport*) buf;

        rep->left_stick_x = scaleStickAxis(inRep->left_stick_x, 256);
        rep->left_stick_y = scaleStickAxis(inRep->left_stick_y, 256);
        rep->right_stick_x = scaleStickAxis(inRep->right_stick_x, 256);
        rep->right_stick_y = scaleStickAxis(inRep->right_stick_y, 256);

        rep->buttons = 0;

        if (inRep->buttons.dpad < 9)
            rep->buttons |= dpad_map[inRep->buttons.dpad];

        if (inRep->buttons.circle)
            rep->buttons |= BTN(DUALSENSE_BUTTON_CIRCLE);
        if (inRep->buttons.cross)
            rep->buttons |= BTN(DUALSENSE_BUTTON_CROSS);
        if (inRep->buttons.triangle)
            rep->buttons |= BTN(DUALSENSE_BUTTON_TRIANGLE);
        if (inRep->buttons.square)
            rep->buttons |= BTN(DUALSENSE_BUTTON_SQUARE);
        if (inRep->buttons.l1)
            rep->buttons |= BTN(DUALSENSE_TRIGGER_L1);
        if (inRep->buttons.r1)
            rep->buttons |= BTN(DUALSENSE_TRIGGER_R1);
        if (inRep->buttons.l2)
            rep->buttons |= BTN(DUALSENSE_TRIGGER_L2);
        if (inRep->buttons.r2)
            rep->buttons |= BTN(DUALSENSE_TRIGGER_R2);
        if (inRep->buttons.create)
            rep->buttons |= BTN(DUALSENSE_BUTTON_CREATE);
        if (inRep->buttons.options)
            rep->buttons |= BTN(DUALSENSE_BUTTON_OPTIONS);
        if (inRep->buttons.l3)
            rep->buttons |= BTN(DUALSENSE_BUTTON_L3);
        if (inRep->buttons.r3)
            rep->buttons |= BTN(DUALSENSE_BUTTON_R3);
        if (inRep->buttons.ps_home)
            rep->buttons |= BTN(DUALSENSE_BUTTON_PS_HOME);
        if (inRep->buttons.mute)
            rep->buttons |= BTN(DUALSENSE_BUTTON_MUTE);
        if (inRep->buttons.touchpad)
            rep->buttons |= BTN(DUALSENSE_BUTTON_TOUCHPAD);
        
        switch (inRep->battery_status) {
        case 0: // discharging
            controller->battery = CLAMP(inRep->battery_level >> 1, 0, 4);
            controller->isCharging = 0;
            break;
        case 1: // charging
            controller->battery = CLAMP(inRep->battery_level >> 1, 0, 4);
            controller->isCharging = 1;
            break;
        default: // everything else indicates an error
            controller->battery = 0;
            controller->isCharging = 0;
            break;
        }

        if (!controller->isReady) {
            controller->isReady = 1;
        }
    }
}

void controllerDeinit_dualsense(Controller* controller)
{
    IOS_Free(LOCAL_PROCESS_HEAP_ID, controller->additionalData);
}

void controllerInit_dualsense(Controller* controller)
{
    controller->data = controllerData_dualsense;
    controller->setPlayerLed = controllerSetLed_dualsense;
    controller->rumble = controllerRumble_dualsense;
    controller->deinit = controllerDeinit_dualsense;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->additionalData = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(DualsenseData));
    memset(controller->additionalData, 0, sizeof(DualsenseData));

    controller->type = BLOOPAIR_CONTROLLER_DUALSENSE;
    Configuration_GetAll(controller->type, controller->bda,
        &controller->commonConfig, &controller->mapping,
        &controller->customConfig, &controller->customConfigSize);
}

void controllerModuleInit_dualsense(void)
{
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_DUALSENSE, NULL, &default_dualsense_mapping, NULL, 0);
}
