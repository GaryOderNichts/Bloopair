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

#include "dualshock3_controller.h"
#include <bloopair/controllers/dualshock3_controller.h>

static const MappingConfiguration default_dualshock3_mapping = {
    .num = 25,
    .mappings = {
        { BLOOPAIR_PRO_STICK_L_UP,      BLOOPAIR_PRO_STICK_L_UP, },
        { BLOOPAIR_PRO_STICK_L_DOWN,    BLOOPAIR_PRO_STICK_L_DOWN, },
        { BLOOPAIR_PRO_STICK_L_LEFT,    BLOOPAIR_PRO_STICK_L_LEFT, },
        { BLOOPAIR_PRO_STICK_L_RIGHT,   BLOOPAIR_PRO_STICK_L_RIGHT, },

        { BLOOPAIR_PRO_STICK_R_UP,      BLOOPAIR_PRO_STICK_R_UP, },
        { BLOOPAIR_PRO_STICK_R_DOWN,    BLOOPAIR_PRO_STICK_R_DOWN, },
        { BLOOPAIR_PRO_STICK_R_LEFT,    BLOOPAIR_PRO_STICK_R_LEFT, },
        { BLOOPAIR_PRO_STICK_R_RIGHT,   BLOOPAIR_PRO_STICK_R_RIGHT, },

        { DUALSHOCK3_BUTTON_DOWN,       BLOOPAIR_PRO_BUTTON_DOWN, },
        { DUALSHOCK3_BUTTON_UP,         BLOOPAIR_PRO_BUTTON_UP },
        { DUALSHOCK3_BUTTON_RIGHT,      BLOOPAIR_PRO_BUTTON_RIGHT, },
        { DUALSHOCK3_BUTTON_LEFT,       BLOOPAIR_PRO_BUTTON_LEFT, },

        { DUALSHOCK3_BUTTON_CIRCLE,     BLOOPAIR_PRO_BUTTON_A, },
        { DUALSHOCK3_BUTTON_CROSS,      BLOOPAIR_PRO_BUTTON_B, },
        { DUALSHOCK3_BUTTON_TRIANGLE,   BLOOPAIR_PRO_BUTTON_X, },
        { DUALSHOCK3_BUTTON_SQUARE,     BLOOPAIR_PRO_BUTTON_Y, },

        { DUALSHOCK3_TRIGGER_L1,        BLOOPAIR_PRO_TRIGGER_L, },
        { DUALSHOCK3_TRIGGER_R1,        BLOOPAIR_PRO_TRIGGER_R, },
        { DUALSHOCK3_TRIGGER_L2,        BLOOPAIR_PRO_TRIGGER_ZL, },
        { DUALSHOCK3_TRIGGER_R2,        BLOOPAIR_PRO_TRIGGER_ZR, },

        { DUALSHOCK3_BUTTON_SELECT,     BLOOPAIR_PRO_BUTTON_MINUS, },
        { DUALSHOCK3_BUTTON_START,      BLOOPAIR_PRO_BUTTON_PLUS, },
        { DUALSHOCK3_BUTTON_L3,         BLOOPAIR_PRO_BUTTON_STICK_L, },
        { DUALSHOCK3_BUTTON_R3,         BLOOPAIR_PRO_BUTTON_STICK_R, },

        { DUALSHOCK3_BUTTON_PS_HOME,    BLOOPAIR_PRO_BUTTON_HOME, },
    },
};

static const Dualshock3LedConfig led_config = { 0xff, 0x27, 0x10, 0x00, 0x32 };

static const uint8_t enable_payload[] = { 0xf4, 0x42, 0x03, 0x00, 0x00 };

static void sendRumbleLedState(Controller* controller)
{
    Dualshock3Data* ds_data = (Dualshock3Data*) controller->additionalData;

    Dualshock3OutputReport rep;
    memset(&rep, 0, sizeof(rep));

    rep.report_id = DUALSHOCK3_OUTPUT_REPORT_ID;

    rep.right_motor_duration = 1;
    rep.right_motor_force = ds_data->rumble;
    rep.left_motor_duration = 1;
    rep.left_motor_force = ds_data->rumble * 64;

    rep.led_mask = ds_data->led_mask << 1;
    memcpy(&rep.leds[0], &led_config, sizeof(Dualshock3LedConfig));
    memcpy(&rep.leds[1], &led_config, sizeof(Dualshock3LedConfig));
    memcpy(&rep.leds[2], &led_config, sizeof(Dualshock3LedConfig));
    memcpy(&rep.leds[3], &led_config, sizeof(Dualshock3LedConfig));

    setReport(controller->handle, BTA_HH_RPTT_OUTPUT, &rep, sizeof(rep));
}

void controllerRumble_dualshock3(Controller* controller, uint8_t rumble)
{
    Dualshock3Data* ds_data = (Dualshock3Data*) controller->additionalData;

    ds_data->rumble = rumble;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualshock3(Controller* controller, uint8_t led)
{
    Dualshock3Data* ds_data = (Dualshock3Data*) controller->additionalData;

    ds_data->led_mask = led;

    // we can only set the leds once we've received at least one report
    ds_data->led_update = 1;
}

void controllerData_dualshock3(Controller* controller, uint8_t* buf, uint16_t len)
{
    Dualshock3Data* ds_data = (Dualshock3Data*) controller->additionalData;

    if (ds_data->led_update) {
        sendRumbleLedState(controller);
        ds_data->led_update = 0;
    }

    if (buf[0] == 0x01) {
        BloopairReportBuffer* rep = &controller->reportBuffer;
        Dualshock3InputReport* inRep = (Dualshock3InputReport*) buf;

        rep->left_stick_x = scaleStickAxis(inRep->left_stick_x, 256);
        rep->left_stick_y = scaleStickAxis(inRep->left_stick_y, 256);
        rep->right_stick_x = scaleStickAxis(inRep->right_stick_x, 256);
        rep->right_stick_y = scaleStickAxis(inRep->right_stick_y, 256);

        rep->buttons = 0;

        if (inRep->buttons.select)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_SELECT);
        if (inRep->buttons.l3)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_L3);
        if (inRep->buttons.r3)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_R3);
        if (inRep->buttons.start)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_START);
        if (inRep->buttons.up)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_UP);
        if (inRep->buttons.right)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_RIGHT);
        if (inRep->buttons.down)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_DOWN);
        if (inRep->buttons.left)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_LEFT);
        if (inRep->buttons.l2)
            rep->buttons |= BTN(DUALSHOCK3_TRIGGER_L2);
        if (inRep->buttons.r2)
            rep->buttons |= BTN(DUALSHOCK3_TRIGGER_R2);
        if (inRep->buttons.l1)
            rep->buttons |= BTN(DUALSHOCK3_TRIGGER_L1);
        if (inRep->buttons.r1)
            rep->buttons |= BTN(DUALSHOCK3_TRIGGER_R1);
        if (inRep->buttons.triangle)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_TRIANGLE);
        if (inRep->buttons.circle)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_CIRCLE);
        if (inRep->buttons.cross)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_CROSS);
        if (inRep->buttons.square)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_SQUARE);
        if (inRep->buttons.ps_home)
            rep->buttons |= BTN(DUALSHOCK3_BUTTON_PS_HOME);

        controller->isCharging = inRep->battery_status == 0x02;
        controller->battery = CLAMP(inRep->battery_level, 0, 4);

        if (!controller->isReady) {
            controller->isReady = 1;
        }
    }
}

void controllerDeinit_dualshock3(Controller* controller)
{
    IOS_Free(LOCAL_PROCESS_HEAP_ID, controller->additionalData);
}

void controllerInit_dualshock3(Controller* controller)
{
    controller->data = controllerData_dualshock3;
    controller->setPlayerLed = controllerSetLed_dualshock3;
    controller->rumble = controllerRumble_dualshock3;
    controller->deinit = controllerDeinit_dualshock3;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->additionalData = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(Dualshock3Data));
    memset(controller->additionalData, 0, sizeof(Dualshock3Data));

    controller->type = BLOOPAIR_CONTROLLER_DUALSHOCK3;
    Configuration_GetAll(controller->type, controller->bda,
        &controller->commonConfig, &controller->mapping,
        &controller->customConfig, &controller->customConfigSize);

    // enable the controller so it sends reports
    setReport(controller->handle, BTA_HH_RPTT_FEATURE, enable_payload, sizeof(enable_payload));
}

void controllerModuleInit_dualshock3(void)
{
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_DUALSHOCK3, NULL, &default_dualshock3_mapping, NULL, 0);
}
