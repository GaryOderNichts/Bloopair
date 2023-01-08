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

void controllerData_xbox_one(Controller* controller, uint8_t* buf, uint16_t len)
{
    ReportBuffer* rep = &controller->reportBuffer;

    if (buf[0] == XBOX_ONE_INPUT_REPORT_ID) {
        XboxOneInputReport* inRep = (XboxOneInputReport*) buf;

        rep->left_stick_x = scaleStickAxis(bswap16(inRep->left_stick_x), 65536);
        rep->left_stick_y = scaleStickAxis(bswap16(inRep->left_stick_y), 65536);
        rep->right_stick_x = scaleStickAxis(bswap16(inRep->right_stick_x), 65536);
        rep->right_stick_y = scaleStickAxis(bswap16(inRep->right_stick_y), 65536);

        // clear all buttons besides home
        rep->buttons &= WPAD_PRO_BUTTON_HOME;

        if (inRep->buttons.dpad < 9)
            rep->buttons |= dpad_map[inRep->buttons.dpad];

        if (inRep->left_trigger)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (inRep->right_trigger)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;

        if (len >= 17) {
            // new format
            if (inRep->buttons.a)
                rep->buttons |= WPAD_PRO_BUTTON_B;
            if (inRep->buttons.b)
                rep->buttons |= WPAD_PRO_BUTTON_A;
            if (inRep->buttons.x)
                rep->buttons |= WPAD_PRO_BUTTON_Y;
            if (inRep->buttons.y)
                rep->buttons |= WPAD_PRO_BUTTON_X;
            if (inRep->buttons.lb)
                rep->buttons |= WPAD_PRO_TRIGGER_L;
            if (inRep->buttons.rb)
                rep->buttons |= WPAD_PRO_TRIGGER_R;
            if (inRep->buttons.menu)
                rep->buttons |= WPAD_PRO_BUTTON_PLUS;
            if (inRep->buttons.xbox)
                rep->buttons |= WPAD_PRO_BUTTON_HOME;
            else
                rep->buttons &= ~WPAD_PRO_BUTTON_HOME;
            if (inRep->buttons.lstick)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
            if (inRep->buttons.rstick)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
            if (inRep->buttons.view)
                rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        } else {
            // old format
            if (inRep->buttons.old.a)
                rep->buttons |= WPAD_PRO_BUTTON_B;
            if (inRep->buttons.old.b)
                rep->buttons |= WPAD_PRO_BUTTON_A;
            if (inRep->buttons.old.x)
                rep->buttons |= WPAD_PRO_BUTTON_Y;
            if (inRep->buttons.old.y)
                rep->buttons |= WPAD_PRO_BUTTON_X;
            if (inRep->buttons.old.lb)
                rep->buttons |= WPAD_PRO_TRIGGER_L;
            if (inRep->buttons.old.rb)
                rep->buttons |= WPAD_PRO_TRIGGER_R;
            if (inRep->buttons.old.view)
                rep->buttons |= WPAD_PRO_BUTTON_MINUS;
            if (inRep->buttons.old.menu)
                rep->buttons |= WPAD_PRO_BUTTON_PLUS;
            if (inRep->buttons.old.lstick)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
            if (inRep->buttons.old.rstick)
                rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        }

        if (!controller->isReady) {
            controller->isReady = 1;
        }
    } else if (buf[0] == XBOX_ONE_XB_BUTTON_INPUT_REPORT_ID) {
        XboxOneXbButtonInputReport* inRep = (XboxOneXbButtonInputReport*) buf;

        if (inRep->xbox_button)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        else
            rep->buttons &= ~WPAD_PRO_BUTTON_HOME;
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
}
