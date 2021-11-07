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

typedef struct {
    int16_t max;
    int16_t center;
    int16_t min;
} SwitchCalibration_t;

typedef struct {
    uint8_t first_report;
    uint8_t report_count;
    uint8_t device;
    uint8_t led;

    SwitchCalibration_t left_calib_x;
    SwitchCalibration_t right_calib_x;
    SwitchCalibration_t left_calib_y;
    SwitchCalibration_t right_calib_y;
} SwitchData_t;

#define AXIS_NORMALIZE_VALUE               3000
#define DPAD_EMULATION_DEAD_ZONE           500

#define SWITCH_FACTORY_CALIBRATION_ADDRESS 0x603d
#define SWITCH_FACTORY_CALIBRATION_SIZE    0x12

#define SWITCH_USER_CALIBRATION_ADDRESS    0x8010
#define SWITCH_USER_CALIBRATION_SIZE       0x16

enum {
    SWITCH_SUBCMD_REQUEST_DEVICE_INFO   = 0x02,
    SWITCH_SUBCMD_SET_INPUT_REPORT_MODE = 0x03,
    SWITCH_SUBCMD_SPI_FLASH_READ        = 0x10,
    SWITCH_SUBCMD_SET_PLAYER_LEDS       = 0x30,
    SWITCH_SUBCMD_ENABLE_VIBRATION      = 0x48,
};

// TODO figure out the other device types
enum {
    SWITCH_DEVICE_UNKNOWN,
    SWITCH_DEVICE_JOYCON_LEFT,
    SWITCH_DEVICE_JOYCON_RIGHT,
    SWITCH_DEVICE_PRO_CONTROLLER,
    SWITCH_DEVICE_N64_CONTROLLER = 12,
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

static int16_t calibrateStickAxis(SwitchCalibration_t* calib, int16_t value)
{
    int16_t ret;
    if (value > calib->center) {
        ret = ((value - calib->center) * AXIS_NORMALIZE_VALUE / 2) 
            / (calib->max - calib->center);
    }
    else {
        ret = ((calib->center - value) * -AXIS_NORMALIZE_VALUE / 2) 
            / (calib->center - calib->min);
    }

    return CLAMP(ret, -AXIS_NORMALIZE_VALUE / 2, AXIS_NORMALIZE_VALUE / 2);
}

static void requestDeviceInfo(Controller_t* controller)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    uint8_t data[11];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = (sdata->report_count++) & 0xf;

    data[10] = SWITCH_SUBCMD_REQUEST_DEVICE_INFO;
    sendOutputData(controller->handle, data, sizeof(data));
}

static void requestFactoryCalibration(Controller_t* controller)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    uint8_t data[16];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = (sdata->report_count++) & 0xf;

    data[10] = SWITCH_SUBCMD_SPI_FLASH_READ;
    data[11] = SWITCH_FACTORY_CALIBRATION_ADDRESS & 0xff;
    data[12] = (SWITCH_FACTORY_CALIBRATION_ADDRESS >> 8) & 0xff;
    data[13] = (SWITCH_FACTORY_CALIBRATION_ADDRESS >> 16) & 0xff;
    data[14] = (SWITCH_FACTORY_CALIBRATION_ADDRESS >> 24) & 0xff;
    data[15] = SWITCH_FACTORY_CALIBRATION_SIZE;

    sendOutputData(controller->handle, data, sizeof(data));
}

static void requestUserCalibration(Controller_t* controller)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    uint8_t data[16];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = (sdata->report_count++) & 0xf;

    data[10] = SWITCH_SUBCMD_SPI_FLASH_READ;
    data[11] = SWITCH_USER_CALIBRATION_ADDRESS & 0xff;
    data[12] = (SWITCH_USER_CALIBRATION_ADDRESS >> 8) & 0xff;
    data[13] = (SWITCH_USER_CALIBRATION_ADDRESS >> 16) & 0xff;
    data[14] = (SWITCH_USER_CALIBRATION_ADDRESS >> 24) & 0xff;
    data[15] = SWITCH_USER_CALIBRATION_SIZE;

    sendOutputData(controller->handle, data, sizeof(data));
}

static void setFullInputReportMode(Controller_t* controller)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    uint8_t data[12];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = (sdata->report_count++) & 0xf;

    data[10] = SWITCH_SUBCMD_SET_INPUT_REPORT_MODE;
    data[11] = 0x30; // 0x30 full reports / 0x3f simple reports
    sendOutputData(controller->handle, data, sizeof(data));
}

static void enableVibration(Controller_t* controller)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    uint8_t data[12];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = (sdata->report_count++) & 0xf;

    data[10] = SWITCH_SUBCMD_ENABLE_VIBRATION;
    data[11] = 1; // 1 enable, 0 disable
    sendOutputData(controller->handle, data, sizeof(data));
}

static void setPlayerLeds(Controller_t* controller)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    uint8_t led = sdata->led;

    // if this is the right joycon swap led order
    if (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT) {
        led = ((led & 1) << 3) | ((led & 2) << 1) | ((led & 4) >> 1) | ((led & 8) >> 3);
    }

    uint8_t data[12];
    _memset(&data, 0, sizeof(data));
    data[0] = 0x01;
    data[1] = (sdata->report_count++) & 0xf;

    data[10] = SWITCH_SUBCMD_SET_PLAYER_LEDS;
    data[11] = led;
    sendOutputData(controller->handle, data, sizeof(data));
}

// These values are hardcoded based on https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md
void controllerRumble_switch(Controller_t* controller, uint8_t rumble)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;
    if (sdata->device == SWITCH_DEVICE_UNKNOWN) {
        return;
    }

    uint8_t data[12];
    _memset(data, 0, sizeof(data));
    data[0] = 0x10;
    data[1] = (sdata->report_count++) & 0xf;

    if (rumble) {
        data[2] = (0x5000 >> 8) & 0xff;
        data[3] = (0x5000 & 0xFF) + 0x8a;
        data[4] = 0x34 + ((0x8062 >> 8) & 0xff);
        data[5] = 0x8062 & 0xff;

        data[6] = (0x5000 >> 8) & 0xff;
        data[7] = (0x5000 & 0xFF) + 0x8a;
        data[8] = 0x34 + ((0x8062 >> 8) & 0xff);
        data[9] = 0x8062 & 0xff;
    }
    else {
        data[2] = 0x00;
        data[3] = 0x01;
        data[4] = 0x40;
        data[5] = 0x40;

        data[6] = 0x00;
        data[7] = 0x01;
        data[8] = 0x40;
        data[9] = 0x40;
    }

    sendOutputData(controller->handle, data, sizeof(data));
}

void controllerSetLed_switch(Controller_t* controller, uint8_t led)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    sdata->led = led;

    // if the device isn't known yet don't set the leds
    // they will be set after the dev info has been read
    if (sdata->device != SWITCH_DEVICE_UNKNOWN) {
        setPlayerLeds(controller);
    }
}

// subcmd response
static void handle_report_0x21(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;

    DEBUG("subcmd respone %d\n", buf[14]);

    uint8_t battery = buf[2] >> 4;
    controller->battery = battery >> 1;
    controller->isCharging = battery & 0x1;

    if ((buf[13] & 0x80) == 0) {
        DEBUG("switch subcmd %d failed\n", buf[14]);
        return;
    }

    if (buf[14] == SWITCH_SUBCMD_REQUEST_DEVICE_INFO) {
        sdata->device = buf[17];
        DEBUG("device type %d\n", sdata->device);

        // set the leds now that we know the device type
        setPlayerLeds(controller);

        // enable rumble
        enableVibration(controller);

        if ((sdata->device == SWITCH_DEVICE_JOYCON_LEFT) ||
            (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT) ||
            (sdata->device == SWITCH_DEVICE_PRO_CONTROLLER) ||
            (sdata->device == SWITCH_DEVICE_N64_CONTROLLER)) {
            // read the factory calibration
            requestFactoryCalibration(controller);
        }
    }
    else if (buf[14] == SWITCH_SUBCMD_SPI_FLASH_READ) {
        uint32_t size = buf[19];
        uint32_t address = buf[15] | buf[16] << 8 | buf[17] << 16 | buf[18] << 24;

        switch (address) {
        case SWITCH_FACTORY_CALIBRATION_ADDRESS:
            if (size < SWITCH_FACTORY_CALIBRATION_SIZE) {
                DEBUG("switch: invalid factory calibration size\n");
                break;
            }

            sdata->left_calib_x.center = buf[23] | ((buf[24] & 0xf) << 8);
            sdata->left_calib_y.center = (buf[24] >> 4) | (buf[25] << 4);

            sdata->left_calib_x.max = sdata->left_calib_x.center + (buf[20] | ((buf[21] & 0xf) << 8));
            sdata->left_calib_y.max = sdata->left_calib_y.center + ((buf[21] >> 4) | (buf[22] << 4));
            sdata->left_calib_x.min = sdata->left_calib_x.center - (buf[26] | ((buf[27] & 0xf) << 8));
            sdata->left_calib_y.min = sdata->left_calib_y.center - ((buf[27] >> 4) | (buf[28] << 4));

            sdata->right_calib_x.center = buf[29] | ((buf[30] & 0xf) << 8);
            sdata->right_calib_y.center = (buf[30] >> 4) | (buf[31] << 4);

            sdata->right_calib_x.min = sdata->right_calib_x.center - (buf[32] | ((buf[33] & 0xf) << 8));
            sdata->right_calib_y.min = sdata->right_calib_y.center - ((buf[33] >> 4) | (buf[34] << 4));
            sdata->right_calib_x.max = sdata->right_calib_x.center + (buf[35] | ((buf[36] & 0xf) << 8));
            sdata->right_calib_y.max = sdata->right_calib_y.center + ((buf[36] >> 4) | (buf[37] << 4));

            // now we can enable full reports
            setFullInputReportMode(controller);
            break;
        case SWITCH_USER_CALIBRATION_ADDRESS:
            // TODO user calibration
            break;
        default:
            DEBUG("switch: unknown SPI read from %x size %d\n", address, size);
            break;
        }
    }
}

static void handle_report_0x30(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;
    ReportBuffer_t* rep = controller->reportData;

    uint8_t battery = buf[2] >> 4;
    controller->battery = battery >> 1;
    controller->isCharging = battery & 0x1;

    rep->buttons = 0;

    if (sdata->device == SWITCH_DEVICE_JOYCON_LEFT) {
        int16_t left_stick_x = calibrateStickAxis(&sdata->left_calib_x, buf[6] | (buf[7] & 0xf) << 8);
        int16_t left_stick_y = calibrateStickAxis(&sdata->left_calib_y, (buf[7] >> 4) | (buf[8] << 4));

        if (left_stick_x < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (left_stick_x > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (left_stick_y < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (left_stick_y > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;

        if (buf[4] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[4] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[4] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        if (buf[5] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[5] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[5] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[5] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[5] & 0x10)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[5] & 0x20)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
    }
    else if (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT) {
        int16_t right_stick_x = calibrateStickAxis(&sdata->right_calib_x, buf[9] | ((buf[10] & 0xf) << 8));
        int16_t right_stick_y = calibrateStickAxis(&sdata->right_calib_y, (buf[10] >> 4) | (buf[11] << 4));

        if (right_stick_x > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (right_stick_x < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (right_stick_y > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (right_stick_y < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;

        if (buf[3] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[3] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[3] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[3] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[3] & 0x10)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[3] & 0x20)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[4] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[4] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
    }
    else if (sdata->device == SWITCH_DEVICE_PRO_CONTROLLER) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_calib_x, buf[6] | (buf[7] & 0xf) << 8);
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_calib_y, (buf[7] >> 4) | (buf[8] << 4));
        rep->right_stick_x = calibrateStickAxis(&sdata->right_calib_x, buf[9] | ((buf[10] & 0xf) << 8));
        rep->right_stick_y = -calibrateStickAxis(&sdata->right_calib_y, (buf[10] >> 4) | (buf[11] << 4));

        if (buf[3] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (buf[3] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[3] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[3] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[3] & 0x40)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[3] & 0x80)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[4] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (buf[4] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[4] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (buf[4] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (buf[4] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        // capture button
        // if (buf[4] & 0x20)
        //     rep->buttons |= WPAD_PRO_BUTTON_;
        if (buf[5] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (buf[5] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (buf[5] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (buf[5] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;
        if (buf[5] & 0x40)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[5] & 0x80)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
    }
    else if (sdata->device == SWITCH_DEVICE_N64_CONTROLLER) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_calib_x, buf[6] | (buf[7] & 0xf) << 8);
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_calib_y, (buf[7] >> 4) | (buf[8] << 4));
        rep->right_stick_y = 0;
        rep->right_stick_x = 0;

        // map the C buttons to the right analog stick
        if (buf[3] & 0x01)
            rep->right_stick_y -= 1140;
        if (buf[3] & 0x02)
            rep->right_stick_x -= 1140;
        if (buf[4] & 0x01)
            rep->right_stick_x += 1140;
        if (buf[3] & 0x80)
            rep->right_stick_y += 1140;

        if (buf[3] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (buf[3] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (buf[3] & 0x40)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (buf[4] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (buf[4] & 0x08)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (buf[4] & 0x10)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        // map capture button to X
        if (buf[4] & 0x20)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (buf[5] & 0x01)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (buf[5] & 0x02)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (buf[5] & 0x04)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (buf[5] & 0x08)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;
        if (buf[5] & 0x40)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (buf[5] & 0x80)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
    }
}

static void handle_report_0x3f(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    SwitchData_t* sdata = (SwitchData_t*) controller->additionalData;
    ReportBuffer_t* rep = controller->reportData;

    // the pro controller sends weird stick data in the first report
    // which completely messes with the start calibration, so we drop that report
    if (sdata->first_report) {
        sdata->first_report = 0;
        return;
    }

    rep->left_stick_x = ((buf[5] << 8) | buf[4]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;
    rep->right_stick_x = ((buf[9] << 8) | buf[8]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;
    rep->left_stick_y = ((buf[7] << 8) | buf[6]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;
    rep->right_stick_y = ((buf[11] << 8) | buf[10]) * AXIS_NORMALIZE_VALUE / 65536 - AXIS_NORMALIZE_VALUE / 2;

    rep->buttons = 0;

    if ((buf[3] & 0xf) < 9)
        rep->buttons |= dpad_map[buf[3] & 0xf];

    if (buf[1] & 0x01)
        rep->buttons |= WPAD_PRO_BUTTON_B;
    if (buf[1] & 0x02)
        rep->buttons |= WPAD_PRO_BUTTON_A;
    if (buf[1] & 0x04)
        rep->buttons |= WPAD_PRO_BUTTON_Y;
    if (buf[1] & 0x08)
        rep->buttons |= WPAD_PRO_BUTTON_X;
    if (buf[1] & 0x10)
        rep->buttons |= WPAD_PRO_TRIGGER_L;
    if (buf[1] & 0x20)
        rep->buttons |= WPAD_PRO_TRIGGER_R;
    if (buf[1] & 0x40)
        rep->buttons |= WPAD_PRO_TRIGGER_ZL;
    if (buf[1] & 0x80)
        rep->buttons |= WPAD_PRO_TRIGGER_ZR;
    if (buf[2] & 0x01)
        rep->buttons |= WPAD_PRO_BUTTON_MINUS;
    if (buf[2] & 0x02)
        rep->buttons |= WPAD_PRO_BUTTON_PLUS;
    if (buf[2] & 0x04)
        rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
    if (buf[2] & 0x08)
        rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
    if (buf[2] & 0x10)
        rep->buttons |= WPAD_PRO_BUTTON_HOME;
    // capture button
    // if (buf[2] & 0x20)
    //     rep->buttons |= WPAD_PRO_BUTTON_;
}

void controllerData_switch(Controller_t* controller, uint8_t* buf, uint16_t len)
{
    if (buf[0] == 0x21) {
        handle_report_0x21(controller, buf, len);
    }
    else if (buf[0] == 0x30) {
        handle_report_0x30(controller, buf, len);
    }
    else if (buf[0] == 0x3f) {
        handle_report_0x3f(controller, buf, len);
    }
}

void controllerDeinit_switch(Controller_t* controller)
{
    deinitContinuousReports(controller);

    IOS_Free(0xcaff, controller->additionalData);
}

void controllerInit_switch(Controller_t* controller)
{
    initContinuousReports(controller);

    controller->data = controllerData_switch;
    controller->setPlayerLed = controllerSetLed_switch;
    controller->rumble = controllerRumble_switch;
    controller->deinit = controllerDeinit_switch;

    controller->battery = 4;
    controller->isCharging = 0;

    SwitchData_t* data = IOS_Alloc(0xcaff, sizeof(SwitchData_t));
    _memset(data, 0, sizeof(SwitchData_t));
    data->first_report = 1;

    controller->additionalData = data;

    requestDeviceInfo(controller);
}
