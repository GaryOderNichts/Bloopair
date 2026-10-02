/*
 *   Copyright (C) 2026 GaryOderNichts
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
#include "stadia_controller.h"
#include <bloopair/controllers/stadia_controller.h>

static const MappingConfiguration default_stadia_mapping = {
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

        { STADIA_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_UP, },
        { STADIA_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_DOWN, },
        { STADIA_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_LEFT, },
        { STADIA_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_RIGHT, },

        { STADIA_BUTTON_STADIA,         BLOOPAIR_PRO_BUTTON_HOME, },
        { STADIA_BUTTON_MENU,           BLOOPAIR_PRO_BUTTON_PLUS, },
        { STADIA_BUTTON_OPTIONS,        BLOOPAIR_PRO_BUTTON_MINUS, },

        { STADIA_BUTTON_R3,             BLOOPAIR_PRO_BUTTON_STICK_R, },
        { STADIA_BUTTON_L3,             BLOOPAIR_PRO_BUTTON_STICK_L, },

        { STADIA_BUTTON_X,              BLOOPAIR_PRO_BUTTON_Y, },
        { STADIA_BUTTON_B,              BLOOPAIR_PRO_BUTTON_A, },
        { STADIA_BUTTON_A,              BLOOPAIR_PRO_BUTTON_B, },
        { STADIA_BUTTON_Y,              BLOOPAIR_PRO_BUTTON_X, },

        { STADIA_TRIGGER_L1,            BLOOPAIR_PRO_TRIGGER_L, },
        { STADIA_TRIGGER_R1,            BLOOPAIR_PRO_TRIGGER_R, },
        { STADIA_TRIGGER_L2,            BLOOPAIR_PRO_TRIGGER_ZL, },
        { STADIA_TRIGGER_R2,            BLOOPAIR_PRO_TRIGGER_ZR, },

        // Map the capture button to the reserved button bit
        { STADIA_BUTTON_CAPTURE,        BLOOPAIR_PRO_RESERVED, },
    },
};

static const uint32_t dpad_map[9] = {
    BTN(STADIA_BUTTON_UP),
    BTN(STADIA_BUTTON_UP) | BTN(STADIA_BUTTON_RIGHT),
    BTN(STADIA_BUTTON_RIGHT),
    BTN(STADIA_BUTTON_RIGHT) | BTN(STADIA_BUTTON_DOWN),
    BTN(STADIA_BUTTON_DOWN),
    BTN(STADIA_BUTTON_DOWN) | BTN(STADIA_BUTTON_LEFT),
    BTN(STADIA_BUTTON_LEFT),
    BTN(STADIA_BUTTON_LEFT) | BTN(STADIA_BUTTON_UP),
    0,
};

void controllerData_stadia(Controller* controller, uint8_t* buf, uint16_t len)
{
    BloopairReportBuffer* rep = &controller->reportBuffer;

    if (buf[0] == STADIA_INPUT_REPORT_ID && len >= sizeof(StadiaInputReport)) {
        StadiaInputReport* inRep = (StadiaInputReport*) buf;

        rep->buttons = 0;

        if (inRep->buttons.dpad < 9)
            rep->buttons |= dpad_map[inRep->buttons.dpad];

        if (inRep->buttons.stadia)
            rep->buttons |= BTN(STADIA_BUTTON_STADIA);
        if (inRep->buttons.menu)
            rep->buttons |= BTN(STADIA_BUTTON_MENU);
        if (inRep->buttons.options)
            rep->buttons |= BTN(STADIA_BUTTON_OPTIONS);
        if (inRep->buttons.r3)
            rep->buttons |= BTN(STADIA_BUTTON_R3);
        if (inRep->buttons.capture)
            rep->buttons |= BTN(STADIA_BUTTON_CAPTURE);
        if (inRep->buttons.assistant)
            rep->buttons |= BTN(STADIA_BUTTON_ASSISTANT);
        if (inRep->buttons.l2)
            rep->buttons |= BTN(STADIA_TRIGGER_L2);
        if (inRep->buttons.r2)
            rep->buttons |= BTN(STADIA_TRIGGER_R2);
        if (inRep->buttons.x)
            rep->buttons |= BTN(STADIA_BUTTON_X);
        if (inRep->buttons.b)
            rep->buttons |= BTN(STADIA_BUTTON_B);
        if (inRep->buttons.a)
            rep->buttons |= BTN(STADIA_BUTTON_A);
        if (inRep->buttons.l3)
            rep->buttons |= BTN(STADIA_BUTTON_L3);
        if (inRep->buttons.r1)
            rep->buttons |= BTN(STADIA_TRIGGER_R1);
        if (inRep->buttons.l1)
            rep->buttons |= BTN(STADIA_TRIGGER_L1);
        if (inRep->buttons.y)
            rep->buttons |= BTN(STADIA_BUTTON_Y);

        rep->left_stick_x = scaleStickAxis(inRep->left_stick_x, 256);
        rep->left_stick_y = scaleStickAxis(inRep->left_stick_y, 256);
        rep->right_stick_x = scaleStickAxis(inRep->right_stick_x, 256);
        rep->right_stick_y = scaleStickAxis(inRep->right_stick_y, 256);

        if (!controller->isReady) {
            controller->isReady = 1;
        }
    }
}

void controllerRumble_stadia(Controller* controller, uint8_t rumble)
{
    StadiaOutputReport output;
    output.report_id = STADIA_OUTPUT_REPORT_ID;
    output.low_rumble = output.high_rumble = rumble ? bswap16(32767) : 0;
    sendOutputData(controller->handle, &output, sizeof(output));
}

void controllerDeinit_stadia(Controller* controller)
{
}

void controllerInit_stadia(Controller* controller)
{
    controller->setPlayerLed = NULL;
    controller->rumble = controllerRumble_stadia;
    controller->data = controllerData_stadia;
    controller->deinit = controllerDeinit_stadia;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->type = BLOOPAIR_CONTROLLER_STADIA;
    Configuration_GetAll(controller->type, controller->bda, &controller->commonConfig, &controller->mapping,
                         &controller->customConfig, &controller->customConfigSize);
}

void controllerModuleInit_stadia(void)
{
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_STADIA, NULL, &default_stadia_mapping, NULL, 0);
}
