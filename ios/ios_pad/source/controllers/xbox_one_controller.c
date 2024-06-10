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

#include "xbox_one_controller.h"
#include <bloopair/controllers/xbox_one_controller.h>

static const MappingConfiguration default_xbox_one_mapping = {
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

        { XBOX_ONE_TRIGGER_RB,          BLOOPAIR_PRO_TRIGGER_R, },
        { XBOX_ONE_TRIGGER_LB,          BLOOPAIR_PRO_TRIGGER_L, },

        { XBOX_ONE_BUTTON_Y,            BLOOPAIR_PRO_BUTTON_X, },
        { XBOX_ONE_BUTTON_X,            BLOOPAIR_PRO_BUTTON_Y, },
        { XBOX_ONE_BUTTON_B,            BLOOPAIR_PRO_BUTTON_A, },
        { XBOX_ONE_BUTTON_A,            BLOOPAIR_PRO_BUTTON_B, },

        { XBOX_ONE_BUTTON_RSTICK,       BLOOPAIR_PRO_BUTTON_STICK_R, },
        { XBOX_ONE_BUTTON_LSTICK,       BLOOPAIR_PRO_BUTTON_STICK_L, },

        { XBOX_ONE_BUTTON_DOWN,         BLOOPAIR_PRO_BUTTON_DOWN, },
        { XBOX_ONE_BUTTON_UP,           BLOOPAIR_PRO_BUTTON_UP },
        { XBOX_ONE_BUTTON_RIGHT,        BLOOPAIR_PRO_BUTTON_RIGHT, },
        { XBOX_ONE_BUTTON_LEFT,         BLOOPAIR_PRO_BUTTON_LEFT, },

        { XBOX_ONE_TRIGGER_L,           BLOOPAIR_PRO_TRIGGER_ZL, },
        { XBOX_ONE_TRIGGER_R,           BLOOPAIR_PRO_TRIGGER_ZR, },

        { XBOX_ONE_BUTTON_XBOX,         BLOOPAIR_PRO_BUTTON_HOME, },
        { XBOX_ONE_BUTTON_MENU,         BLOOPAIR_PRO_BUTTON_PLUS, },
        { XBOX_ONE_BUTTON_VIEW,         BLOOPAIR_PRO_BUTTON_MINUS, },
    },
};

static const uint32_t dpad_map[9] = {
    0,
    BTN(XBOX_ONE_BUTTON_UP),
    BTN(XBOX_ONE_BUTTON_UP)    | BTN(XBOX_ONE_BUTTON_RIGHT),
    BTN(XBOX_ONE_BUTTON_RIGHT),
    BTN(XBOX_ONE_BUTTON_RIGHT) | BTN(XBOX_ONE_BUTTON_DOWN),
    BTN(XBOX_ONE_BUTTON_DOWN),
    BTN(XBOX_ONE_BUTTON_DOWN)  | BTN(XBOX_ONE_BUTTON_LEFT),
    BTN(XBOX_ONE_BUTTON_LEFT),
    BTN(XBOX_ONE_BUTTON_LEFT)  | BTN(XBOX_ONE_BUTTON_UP)
};

void controllerData_xbox_one(Controller* controller, uint8_t* buf, uint16_t len)
{
    BloopairReportBuffer* rep = &controller->reportBuffer;

    if (buf[0] == XBOX_ONE_INPUT_REPORT_ID) {
        XboxOneInputReport* inRep = (XboxOneInputReport*) buf;

        rep->left_stick_x = scaleStickAxis(bswap16(inRep->left_stick_x), 65536);
        rep->left_stick_y = scaleStickAxis(bswap16(inRep->left_stick_y), 65536);
        rep->right_stick_x = scaleStickAxis(bswap16(inRep->right_stick_x), 65536);
        rep->right_stick_y = scaleStickAxis(bswap16(inRep->right_stick_y), 65536);

        // clear all buttons besides the xb button
        rep->buttons &= BTN(XBOX_ONE_BUTTON_XBOX);

        if (inRep->buttons.dpad < 9)
            rep->buttons |= dpad_map[inRep->buttons.dpad];

        // TODO trigger deadzone config?
        if (inRep->left_trigger)
            rep->buttons |= BTN(XBOX_ONE_TRIGGER_L);
        if (inRep->right_trigger)
            rep->buttons |= BTN(XBOX_ONE_TRIGGER_R);

        if (len >= 17) {
            // new format
            if (inRep->buttons.a)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_A);
            if (inRep->buttons.b)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_B);
            if (inRep->buttons.x)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_X);
            if (inRep->buttons.y)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_Y);
            if (inRep->buttons.lb)
                rep->buttons |= BTN(XBOX_ONE_TRIGGER_LB);
            if (inRep->buttons.rb)
                rep->buttons |= BTN(XBOX_ONE_TRIGGER_RB);
            if (inRep->buttons.menu)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_MENU);
            if (inRep->buttons.xbox)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_XBOX);
            else
                rep->buttons &= ~BTN(XBOX_ONE_BUTTON_XBOX);
            if (inRep->buttons.lstick)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_LSTICK);
            if (inRep->buttons.rstick)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_RSTICK);
            if (inRep->buttons.view)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_VIEW);
        } else {
            // old format
            if (inRep->buttons.old.a)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_A);
            if (inRep->buttons.old.b)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_B);
            if (inRep->buttons.old.x)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_X);
            if (inRep->buttons.old.y)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_Y);
            if (inRep->buttons.old.lb)
                rep->buttons |= BTN(XBOX_ONE_TRIGGER_LB);
            if (inRep->buttons.old.rb)
                rep->buttons |= BTN(XBOX_ONE_TRIGGER_RB);
            if (inRep->buttons.old.view)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_VIEW);
            if (inRep->buttons.old.menu)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_MENU);
            if (inRep->buttons.old.lstick)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_LSTICK);
            if (inRep->buttons.old.rstick)
                rep->buttons |= BTN(XBOX_ONE_BUTTON_RSTICK);
        }

        if (!controller->isReady) {
            controller->isReady = 1;
        }
    } else if (buf[0] == XBOX_ONE_XB_BUTTON_INPUT_REPORT_ID) {
        XboxOneXbButtonInputReport* inRep = (XboxOneXbButtonInputReport*) buf;

        if (inRep->xbox_button)
            rep->buttons |= BTN(XBOX_ONE_BUTTON_XBOX);
        else
            rep->buttons &= ~BTN(XBOX_ONE_BUTTON_XBOX);
    } else if (buf[0] == XBOX_ONE_BATTERY_INPUT_REPORT_ID) {
        XboxOneBatteryInputReport* inRep = (XboxOneBatteryInputReport*) buf;

        controller->battery = inRep->capacity + 1;
        controller->isCharging = inRep->charging;
    }
}

void controllerRumble_xbox_one(Controller* controller, uint8_t rumble)
{
    XboxOneOutputReport rep;
    rep.report_id = XBOX_ONE_OUTPUT_REPORT_ID;
    rep.motors_enable = XBOX_ONE_RUMBLE_MAIN;
    rep.magnitude_left = 0;
    rep.magnitude_right = 0;
    rep.magnitude_strong = rumble ? 35 : 0;
    rep.magnitude_weak = rumble ? 35 : 0;
    rep.pulse_sustain_10ms = 1;
    rep.pulse_release_10ms = 0;
    rep.loop_count = 0;

    sendOutputData(controller->handle, &rep, sizeof(rep));
}

void controllerDeinit_xbox_one(Controller* controller)
{
}

void controllerInit_xbox_one(Controller* controller)
{
    controller->setPlayerLed = NULL;
    controller->rumble = controllerRumble_xbox_one;
    controller->data = controllerData_xbox_one;
    controller->deinit = controllerDeinit_xbox_one;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->type = BLOOPAIR_CONTROLLER_XBOX_ONE;
    Configuration_GetAll(controller->type, controller->bda,
        &controller->commonConfig, &controller->mapping,
        &controller->customConfig, &controller->customConfigSize);
}

void controllerModuleInit_xbox_one(void)
{
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_XBOX_ONE, NULL, &default_xbox_one_mapping, NULL, 0);
}
