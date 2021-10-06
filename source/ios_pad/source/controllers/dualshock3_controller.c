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

typedef struct {
    uint8_t led_update;
    uint8_t led_mask;
    uint8_t rumble;
} Dualshock3Data_t;

static const uint8_t led_config[] = { 0xff, 0x27, 0x10, 0x00, 0x32 };

static const uint8_t enable_payload[] = { 0xf4, 0x42, 0x03, 0x00, 0x00 };

static void sendRumbleLedState(Controller_t* controller)
{
    Dualshock3Data_t* ds_data = (Dualshock3Data_t*) controller->additionalData;

    uint8_t data[36];
    _memset(data, 0, 36);

    data[0] = 0x01;

    data[2] = 1; // right rumble duration
    data[3] = ds_data->rumble * 64;
    data[4] = 1; // left rumble duration
    data[5] = ds_data->rumble * 64;

    data[10] = ds_data->led_mask << 1;
    _memcpy(&data[11], led_config, sizeof(led_config));
    _memcpy(&data[16], led_config, sizeof(led_config));
    _memcpy(&data[21], led_config, sizeof(led_config));
    _memcpy(&data[26], led_config, sizeof(led_config));

    setReport(controller->handle, BTA_HH_RPTT_OUTPUT, data, sizeof(data));
}

void controllerRumble_dualshock3(Controller_t* controller, uint8_t rumble)
{
    Dualshock3Data_t* ds_data = (Dualshock3Data_t*) controller->additionalData;

    ds_data->rumble = rumble;

    sendRumbleLedState(controller);
}

void controllerSetLed_dualshock3(Controller_t* controller, uint8_t led)
{
    Dualshock3Data_t* ds_data = (Dualshock3Data_t*) controller->additionalData;

    ds_data->led_mask = led;

    // we can only set the leds once we've received at least one report
    ds_data->led_update = 1;
}

#define AXIS_NORMALIZE_VALUE (1140 * 2)

void controllerData_dualshock3(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    Dualshock3Data_t* ds_data = (Dualshock3Data_t*) controller->additionalData;

    if (ds_data->led_update) {
        sendRumbleLedState(controller);
        ds_data->led_update = 0;
    }

    if (buf[0] == 0x01) {
        ReportBuffer_t* rep = controller->reportData;

        rep->left_stick_x = (buf[6] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->left_stick_y = (buf[7] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_x = (buf[8] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;
        rep->right_stick_y = (buf[9] - 256 / 2) * AXIS_NORMALIZE_VALUE / 256;

        rep->buttons = 0;

        if (buf[2] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[2] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (buf[2] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (buf[2] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[2] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (buf[2] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (buf[2] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (buf[2] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;
        if (buf[3] & 0x01)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
        if (buf[3] & 0x02)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[3] & 0x04)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[3] & 0x08)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[3] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[3] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[3] & 0x40)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[3] & 0x80)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[4] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
    }
}

void controllerDeinit_dualshock3(Controller_t* controller)
{
    deinitContinuousReports(controller);
}

void controllerInit_dualshock3(Controller_t* controller)
{
    initContinuousReports(controller);

    controller->data = controllerData_dualshock3;
    controller->setPlayerLed = controllerSetLed_dualshock3;
    controller->rumble = controllerRumble_dualshock3;
    controller->deinit = controllerDeinit_dualshock3;

    controller->battery = 4;
    controller->isCharging = 0;

    controller->additionalData = IOS_Alloc(0xcaff, sizeof(Dualshock3Data_t));
    _memset(controller->additionalData, 0, sizeof(Dualshock3Data_t));

    // enable the controller so it sends reports
    setReport(controller->handle, BTA_HH_RPTT_FEATURE, enable_payload, sizeof(enable_payload));
}
