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

#include "switch_controller.h"
#include <bloopair/controllers/switch_controller.h>

// Joystick center for basic reports
#define BASIC_JOYSTICK_CENTER              0x8000

// Normalize value for switch -> wii u range
#define AXIS_NORMALIZE_VALUE               1140

// These values are based on https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md
#define RUMBLE_HIGH_FREQUENCY 0x7400 // 150 Hz
#define RUMBLE_HIGH_AMPLITUDE 0xc8 // 1.003
#define RUMBLE_LOW_FREQUENCY 0x3d // 150 Hz
#define RUMBLE_LOW_AMPLITUDE 0x0072 // 1.003

static const SwitchConfiguration default_switch_configuration = {
    .disableCalibration = 0,
};

static const MappingConfiguration default_generic_mapping = {
    .num = 30,
    .mappings = {
        { BLOOPAIR_PRO_STICK_L_UP,      BLOOPAIR_PRO_STICK_L_UP, },
        { BLOOPAIR_PRO_STICK_L_DOWN,    BLOOPAIR_PRO_STICK_L_DOWN, },
        { BLOOPAIR_PRO_STICK_L_LEFT,    BLOOPAIR_PRO_STICK_L_LEFT, },
        { BLOOPAIR_PRO_STICK_L_RIGHT,   BLOOPAIR_PRO_STICK_L_RIGHT, },

        { BLOOPAIR_PRO_STICK_R_UP,      BLOOPAIR_PRO_STICK_R_UP, },
        { BLOOPAIR_PRO_STICK_R_DOWN,    BLOOPAIR_PRO_STICK_R_DOWN, },
        { BLOOPAIR_PRO_STICK_R_LEFT,    BLOOPAIR_PRO_STICK_R_LEFT, },
        { BLOOPAIR_PRO_STICK_R_RIGHT,   BLOOPAIR_PRO_STICK_R_RIGHT, },

        { SWITCH_BUTTON_Y,              BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH_BUTTON_X,              BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH_BUTTON_B,              BLOOPAIR_PRO_BUTTON_B, },
        { SWITCH_BUTTON_A,              BLOOPAIR_PRO_BUTTON_A, },

        { SWITCH_TRIGGER_R,             BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_TRIGGER_ZR,            BLOOPAIR_PRO_TRIGGER_ZR, },

        { SWITCH_BUTTON_MINUS,          BLOOPAIR_PRO_BUTTON_MINUS, },
        { SWITCH_BUTTON_PLUS,           BLOOPAIR_PRO_BUTTON_PLUS, },
        { SWITCH_BUTTON_STICK_R,        BLOOPAIR_PRO_BUTTON_STICK_R, },
        { SWITCH_BUTTON_STICK_L,        BLOOPAIR_PRO_BUTTON_STICK_L, },
        { SWITCH_BUTTON_HOME,           BLOOPAIR_PRO_BUTTON_HOME, },
        // map the capture button to the reserved button bit
        { SWITCH_BUTTON_CAPTURE,        BLOOPAIR_PRO_RESERVED, },

        { SWITCH_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_DOWN, },
        { SWITCH_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_UP },
        { SWITCH_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_RIGHT, },
        { SWITCH_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_LEFT, },

        { SWITCH_TRIGGER_L,             BLOOPAIR_PRO_TRIGGER_L, },
        { SWITCH_TRIGGER_ZL,            BLOOPAIR_PRO_TRIGGER_ZL, },

        { SWITCH_TRIGGER_SR_L,          BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_TRIGGER_SL_L,          BLOOPAIR_PRO_TRIGGER_L, },

        { SWITCH_TRIGGER_SR_R,          BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_TRIGGER_SL_R,          BLOOPAIR_PRO_TRIGGER_L, },
    },
};

static const MappingConfiguration default_joycon_left_mapping = {
    .num = 12,
    .mappings = {
        // map stick to dpad (assume a sideways joycon)
        { BLOOPAIR_PRO_STICK_L_DOWN,    BLOOPAIR_PRO_BUTTON_RIGHT, },
        { BLOOPAIR_PRO_STICK_L_UP,      BLOOPAIR_PRO_BUTTON_LEFT, },
        { BLOOPAIR_PRO_STICK_L_RIGHT,   BLOOPAIR_PRO_BUTTON_UP, },
        { BLOOPAIR_PRO_STICK_L_LEFT,    BLOOPAIR_PRO_BUTTON_DOWN, },

        { SWITCH_BUTTON_MINUS,          BLOOPAIR_PRO_BUTTON_MINUS },
        // left joy-con only has capture button, let's map it to home
        { SWITCH_BUTTON_CAPTURE,        BLOOPAIR_PRO_BUTTON_HOME },

        // map the dpad to abxy
        { SWITCH_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_A, },
        { SWITCH_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_B, },

        { SWITCH_TRIGGER_SR_L,          BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_TRIGGER_SL_L,          BLOOPAIR_PRO_TRIGGER_L, },
    },
};

static const MappingConfiguration default_joycon_right_mapping = {
    .num = 12,
    .mappings = {
        // map stick to dpad (assume a sideways joycon)
        { BLOOPAIR_PRO_STICK_R_DOWN,    BLOOPAIR_PRO_BUTTON_LEFT, },
        { BLOOPAIR_PRO_STICK_R_UP,      BLOOPAIR_PRO_BUTTON_RIGHT, },
        { BLOOPAIR_PRO_STICK_R_RIGHT,   BLOOPAIR_PRO_BUTTON_DOWN, },
        { BLOOPAIR_PRO_STICK_R_LEFT,    BLOOPAIR_PRO_BUTTON_UP, },

        // rotate abxy for sidewise joy-con
        { SWITCH_BUTTON_Y,              BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH_BUTTON_X,              BLOOPAIR_PRO_BUTTON_A, },
        { SWITCH_BUTTON_B,              BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH_BUTTON_A,              BLOOPAIR_PRO_BUTTON_B, },

        { SWITCH_TRIGGER_SR_R,          BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_TRIGGER_SL_R,          BLOOPAIR_PRO_TRIGGER_L, },

        { SWITCH_BUTTON_PLUS,           BLOOPAIR_PRO_BUTTON_PLUS },
        { SWITCH_BUTTON_HOME,           BLOOPAIR_PRO_BUTTON_HOME },
    },
};

static const MappingConfiguration default_pro_controller_mapping = {
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

        { SWITCH_BUTTON_Y,              BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH_BUTTON_X,              BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH_BUTTON_B,              BLOOPAIR_PRO_BUTTON_B, },
        { SWITCH_BUTTON_A,              BLOOPAIR_PRO_BUTTON_A, },

        { SWITCH_TRIGGER_R,             BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_TRIGGER_ZR,            BLOOPAIR_PRO_TRIGGER_ZR, },

        { SWITCH_BUTTON_MINUS,          BLOOPAIR_PRO_BUTTON_MINUS, },
        { SWITCH_BUTTON_PLUS,           BLOOPAIR_PRO_BUTTON_PLUS, },
        { SWITCH_BUTTON_STICK_R,        BLOOPAIR_PRO_BUTTON_STICK_R, },
        { SWITCH_BUTTON_STICK_L,        BLOOPAIR_PRO_BUTTON_STICK_L, },
        { SWITCH_BUTTON_HOME,           BLOOPAIR_PRO_BUTTON_HOME, },
        // map the capture button to the reserved button bit
        { SWITCH_BUTTON_CAPTURE,        BLOOPAIR_PRO_RESERVED, },

        { SWITCH_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_DOWN, },
        { SWITCH_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_UP },
        { SWITCH_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_RIGHT, },
        { SWITCH_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_LEFT, },

        { SWITCH_TRIGGER_L,             BLOOPAIR_PRO_TRIGGER_L, },
        { SWITCH_TRIGGER_ZL,            BLOOPAIR_PRO_TRIGGER_ZL, },
    },
};

static const MappingConfiguration default_n64_mapping = {
    .num = 21,
    .mappings = {
        { BLOOPAIR_PRO_STICK_L_UP,      BLOOPAIR_PRO_STICK_L_UP, },
        { BLOOPAIR_PRO_STICK_L_DOWN,    BLOOPAIR_PRO_STICK_L_DOWN, },
        { BLOOPAIR_PRO_STICK_L_LEFT,    BLOOPAIR_PRO_STICK_L_LEFT, },
        { BLOOPAIR_PRO_STICK_L_RIGHT,   BLOOPAIR_PRO_STICK_L_RIGHT, },

        // map the C buttons to the right analog stick
        { SWITCH_N64_C_UP,              BLOOPAIR_PRO_STICK_R_UP, },
        { SWITCH_N64_C_DOWN,            BLOOPAIR_PRO_STICK_R_DOWN, },
        { SWITCH_N64_C_LEFT,            BLOOPAIR_PRO_STICK_R_LEFT, },
        { SWITCH_N64_C_RIGHT,           BLOOPAIR_PRO_STICK_R_RIGHT, },

        { SWITCH_BUTTON_B,              BLOOPAIR_PRO_BUTTON_B, },
        { SWITCH_BUTTON_A,              BLOOPAIR_PRO_BUTTON_A, },

        { SWITCH_TRIGGER_R,             BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH_BUTTON_PLUS,           BLOOPAIR_PRO_BUTTON_PLUS, },

        { SWITCH_N64_ZR,                BLOOPAIR_PRO_TRIGGER_ZR, },
        { SWITCH_BUTTON_HOME,           BLOOPAIR_PRO_BUTTON_HOME, },

        // map the capture button to the reserved button bit
        { SWITCH_BUTTON_CAPTURE,        BLOOPAIR_PRO_RESERVED, },

        { SWITCH_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_DOWN, },
        { SWITCH_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_UP },
        { SWITCH_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_RIGHT, },
        { SWITCH_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_LEFT, },

        { SWITCH_TRIGGER_L,             BLOOPAIR_PRO_TRIGGER_L, },
        { SWITCH_TRIGGER_ZL,            BLOOPAIR_PRO_TRIGGER_ZL, },
    },
};

static const uint32_t dpad_map[9] = {
    BTN(SWITCH_BUTTON_UP),
    BTN(SWITCH_BUTTON_UP)    | BTN(SWITCH_BUTTON_RIGHT),
    BTN(SWITCH_BUTTON_RIGHT),
    BTN(SWITCH_BUTTON_RIGHT) | BTN(SWITCH_BUTTON_DOWN),
    BTN(SWITCH_BUTTON_DOWN),
    BTN(SWITCH_BUTTON_DOWN)  | BTN(SWITCH_BUTTON_LEFT),
    BTN(SWITCH_BUTTON_LEFT),
    BTN(SWITCH_BUTTON_LEFT)  | BTN(SWITCH_BUTTON_UP),
    0,
};

static inline BloopairControllerType switchDeviceToControllerType(uint8_t device)
{
    switch (device) {
    case SWITCH_DEVICE_JOYCON_LEFT:
    case SWITCH_DEVICE_TP_JOYCON_LEFT:
        return BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT;
    case SWITCH_DEVICE_JOYCON_RIGHT:
    case SWITCH_DEVICE_TP_JOYCON_RIGHT:
        return BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT;
    case SWITCH_DEVICE_PRO:
        return BLOOPAIR_CONTROLLER_SWITCH_PRO;
    case SWITCH_DEVICE_N64:
        return BLOOPAIR_CONTROLLER_SWITCH_N64;
    }

    return BLOOPAIR_CONTROLLER_SWITCH_GENERIC;
}

static void finalizeStickCalibration(SwitchStickCalibration* calibration)
{
    DEBUG_PRINT("StickCalibration center: %d min: %d max: %d\n",
        calibration->center, calibration->min, calibration->max);

    // Some controllers only store partial calibration data, in which case the raw data is just 0xfff
    if (calibration->center == 0xfff) {
        calibration->center = 2048;
    }

    if (calibration->max == 0xfff) {
        calibration->max = 1434; // calibration->center * 0.7f;
    }

    if (calibration->min == 0xfff) {
        calibration->min = 1434; // calibration->center * 0.7f;
    }

    calibration->max = calibration->center + calibration->max;
    calibration->min = calibration->center - calibration->min;
}

static void parseLeftRawStickCalibration(SwitchData* sdata, SwitchRawStickCalibrationLeft* raw)
{
    sdata->left_calib_x.center = SWITCH_AXIS_X(raw->center);
    sdata->left_calib_x.max = SWITCH_AXIS_X(raw->max);
    sdata->left_calib_x.min = SWITCH_AXIS_X(raw->min);
    finalizeStickCalibration(&sdata->left_calib_x);

    sdata->left_calib_y.center = SWITCH_AXIS_Y(raw->center);
    sdata->left_calib_y.max = SWITCH_AXIS_Y(raw->max);
    sdata->left_calib_y.min = SWITCH_AXIS_Y(raw->min);
    finalizeStickCalibration(&sdata->left_calib_y);
}

static void parseRightRawStickCalibration(SwitchData* sdata, SwitchRawStickCalibrationRight* raw)
{
    sdata->right_calib_x.center = SWITCH_AXIS_X(raw->center);
    sdata->right_calib_x.max = SWITCH_AXIS_X(raw->max);
    sdata->right_calib_x.min = SWITCH_AXIS_X(raw->min);
    finalizeStickCalibration(&sdata->right_calib_x);

    sdata->right_calib_y.center = SWITCH_AXIS_Y(raw->center);
    sdata->right_calib_y.max = SWITCH_AXIS_Y(raw->max);
    sdata->right_calib_y.min = SWITCH_AXIS_Y(raw->min);
    finalizeStickCalibration(&sdata->right_calib_y);
}

static int16_t calibrateStickAxis(SwitchStickCalibration* calib, uint32_t value)
{
    int32_t calibrated;
    if (value > calib->center) {
        calibrated = (value - calib->center) * AXIS_NORMALIZE_VALUE;
        calibrated /= calib->max - calib->center;
    } else {
        calibrated = (calib->center - value) * -AXIS_NORMALIZE_VALUE;
        calibrated /= calib->center - calib->min;
    }

    return (int16_t) CLAMP(calibrated, -AXIS_NORMALIZE_VALUE, AXIS_NORMALIZE_VALUE);
}

static int16_t remapBasicStickAxis(SwitchStickExtent* extent, uint16_t rawValue)
{
    int16_t value = (int16_t) rawValue - BASIC_JOYSTICK_CENTER;

    // Dynamically adjust extents
    if (value > extent->max) {
        extent->max = value;
    }
    if (value < extent->min) {
        extent->min = value;
    }

    return remapStickAxis(value, extent->min, extent->max);
}

static void sendCommand(Controller* controller, SwitchCommandRequest* req, uint32_t req_data_size)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;

    SwitchCommandOutputReport rep;
    memset(&rep, 0, sizeof(rep));
    rep.output.report_id = SWITCH_COMMAND_OUTPUT_REPORT_ID;
    rep.output.counter = (sdata->report_count++) & 0xf;

    rep.request.command = req->command;
    memcpy(rep.request.data, req->data, req_data_size);

    sendOutputData(controller->handle, &rep, sizeof(rep.output) + sizeof(rep.request.command) + req_data_size);
}

static void requestDeviceInfo(Controller* controller)
{
    SwitchCommandRequest req;
    req.command = SWITCH_COMMAND_REQUEST_DEVICE_INFO;

    sendCommand(controller, &req, 0);
}

static void readSpiFlash(Controller* controller, uint32_t address, uint8_t size)
{
    SwitchCommandRequest req;
    req.command = SWITCH_COMMAND_SPI_FLASH_READ;

    req.spi_flash_read.address = bswap32(address);
    req.spi_flash_read.size = size;

    sendCommand(controller, &req, sizeof(req.spi_flash_read));
}

static void setInputReportMode(Controller* controller, uint8_t report_mode)
{
    SwitchCommandRequest req;
    req.command = SWITCH_COMMAND_SET_INPUT_REPORT_MODE;

    req.report_mode = report_mode;

    sendCommand(controller, &req, sizeof(req.report_mode));
}

static void setVibration(Controller* controller, uint8_t enabled)
{
    SwitchCommandRequest req;
    req.command = SWITCH_COMMAND_ENABLE_VIBRATION;

    req.vibration_enabled = enabled;

    sendCommand(controller, &req, sizeof(req.vibration_enabled));
}

static void setPlayerLeds(Controller* controller)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;

    uint8_t led = sdata->led;

    // if this is the right joycon swap led order
    if (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT || sdata->device == SWITCH_DEVICE_TP_JOYCON_RIGHT) {
        led = ((led & 1) << 3) | ((led & 2) << 1) | ((led & 4) >> 1) | ((led & 8) >> 3);
    }

    SwitchCommandRequest req;
    req.command = SWITCH_COMMAND_SET_PLAYER_LEDS;

    req.leds = led;

    sendCommand(controller, &req, sizeof(req.leds));
}

void controllerRumble_switch(Controller* controller, uint8_t rumble)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;
    if (sdata->device == SWITCH_DEVICE_UNKNOWN) {
        return;
    }

    SwitchOutputReport rep;
    rep.report_id = SWITCH_OUTPUT_REPORT_ID;
    rep.counter = (sdata->report_count++) & 0xf;

    if (rumble) {
        rep.left_motor[0] = (RUMBLE_HIGH_FREQUENCY >> 8) & 0xff;
        rep.left_motor[1] = (RUMBLE_HIGH_FREQUENCY & 0xff) + RUMBLE_HIGH_AMPLITUDE;
        rep.left_motor[2] = RUMBLE_LOW_FREQUENCY + ((RUMBLE_LOW_AMPLITUDE >> 8) & 0xff);
        rep.left_motor[3] = RUMBLE_LOW_AMPLITUDE & 0xff;

        rep.right_motor[0] = (RUMBLE_HIGH_FREQUENCY >> 8) & 0xff;
        rep.right_motor[1] = (RUMBLE_HIGH_FREQUENCY & 0xff) + RUMBLE_HIGH_AMPLITUDE;
        rep.right_motor[2] = RUMBLE_LOW_FREQUENCY + ((RUMBLE_LOW_AMPLITUDE >> 8) & 0xff);
        rep.right_motor[3] = RUMBLE_LOW_AMPLITUDE & 0xff;
    } else {
        rep.left_motor[0] = 0x00;
        rep.left_motor[1] = 0x01;
        rep.left_motor[2] = 0x40;
        rep.left_motor[3] = 0x40;

        rep.right_motor[0] = 0x00;
        rep.right_motor[1] = 0x01;
        rep.right_motor[2] = 0x40;
        rep.right_motor[3] = 0x40;
    }

    sendOutputData(controller->handle, &rep, sizeof(rep));
}

void controllerSetLed_switch(Controller* controller, uint8_t led)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;

    sdata->led = led;

    // if the device isn't known yet don't set the leds
    // they will be set after the dev info has been read
    if (sdata->device != SWITCH_DEVICE_UNKNOWN) {
        setPlayerLeds(controller);
    }
}

static int switchConfigCalibrationEnabled(Controller* controller)
{
    if (!controller->customConfig) {
        return 0;
    }

    SwitchConfiguration* config = (SwitchConfiguration*) controller->customConfig;
    return !config->disableCalibration;
}

static void handle_command_response(Controller* controller, SwitchCommandResponse* resp)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;

    DEBUG_PRINT("subcmd respone %d\n", resp->command);

    if ((resp->ack & 0x80) == 0) {
        DEBUG_PRINT("switch subcmd %d failed\n", resp->command);
        // if we failed during one of the stages, just fall back to simple input
        controller->isReady = 1;
        return;
    }

    if (resp->command == SWITCH_COMMAND_REQUEST_DEVICE_INFO) {
        sdata->device = resp->device_info.device_type;
        DEBUG_PRINT("device type %d\n", sdata->device);

        // Get the configuration again, now that we have the actual device type
        controller->type = switchDeviceToControllerType(sdata->device);
        Configuration_GetAll(controller->type, controller->bda,
            &controller->commonConfig, &controller->mapping,
            &controller->customConfig, &controller->customConfigSize);

        // set the leds now that we know the device type
        setPlayerLeds(controller);

        // enable rumble
        setVibration(controller, 1);

                                                         /* Calibration causes issues for third-party controllers */
        if ((sdata->device == SWITCH_DEVICE_JOYCON_LEFT || /*sdata->device == SWITCH_DEVICE_TP_JOYCON_LEFT ||*/
             sdata->device == SWITCH_DEVICE_JOYCON_RIGHT || /*sdata->device == SWITCH_DEVICE_TP_JOYCON_RIGHT ||*/
             sdata->device == SWITCH_DEVICE_PRO || /*sdata->device == SWITCH_DEVICE_TP_PRO ||*/
             sdata->device == SWITCH_DEVICE_N64) && switchConfigCalibrationEnabled(controller)) {
            // Start by reading the left user calibration magic to check if user calibration exists
            readSpiFlash(controller, SWITCH_LEFT_USER_CALIBRATION_MAGIC_ADDRESS, 2);
        } else {
            // Don't need to wait for calibration, controller is ready now
            controller->isReady = 1;
        }
    } else if (resp->command == SWITCH_COMMAND_SPI_FLASH_READ) {
        uint32_t address = bswap32(resp->spi_flash_read.address);

        switch (address) {
        case SWITCH_LEFT_USER_CALIBRATION_MAGIC_ADDRESS:
            // Check for user calibration
            if (resp->spi_flash_read.data[0] == 0xb2 && resp->spi_flash_read.data[1] == 0xa1) {
                // Read user calibration
                readSpiFlash(controller, SWITCH_LEFT_USER_CALIBRATION_ADDRESS, sizeof(SwitchRawStickCalibrationLeft));
            } else {
                // Fall back to factory calibration
                readSpiFlash(controller, SWITCH_LEFT_FACTORY_CALIBRATION_ADDRESS, sizeof(SwitchRawStickCalibrationLeft));
            }
            break;
        case SWITCH_LEFT_FACTORY_CALIBRATION_ADDRESS:
        case SWITCH_LEFT_USER_CALIBRATION_ADDRESS:
            // parse the calibration data
            parseLeftRawStickCalibration(sdata, (SwitchRawStickCalibrationLeft*) resp->spi_flash_read.data);

            // continue with reading the right calibration magic
            readSpiFlash(controller, SWITCH_RIGHT_USER_CALIBRATION_MAGIC_ADDRESS, 2);
            break;
        case SWITCH_RIGHT_USER_CALIBRATION_MAGIC_ADDRESS:
            // Check for user calibration
            if (resp->spi_flash_read.data[0] == 0xb2 && resp->spi_flash_read.data[1] == 0xa1) {
                // Read user calibration
                readSpiFlash(controller, SWITCH_RIGHT_USER_CALIBRATION_ADDRESS, sizeof(SwitchRawStickCalibrationRight));
            } else {
                // Fall back to factory calibration
                readSpiFlash(controller, SWITCH_RIGHT_FACTORY_CALIBRATION_ADDRESS, sizeof(SwitchRawStickCalibrationRight));
            }
            break;
        case SWITCH_RIGHT_FACTORY_CALIBRATION_ADDRESS:
        case SWITCH_RIGHT_USER_CALIBRATION_ADDRESS:
            // parse the calibration data
            parseRightRawStickCalibration(sdata, (SwitchRawStickCalibrationRight*) resp->spi_flash_read.data);

            // we can now enable full reports
            setInputReportMode(controller, SWITCH_INPUT_REPORT_ID);
            break;
        default:
            DEBUG_PRINT("switch: unknown SPI read from %lx size %d\n", address, resp->spi_flash_read.size);
            break;
        }
    }
}

static void handle_input_report(Controller* controller, SwitchInputReport* inRep)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;
    BloopairReportBuffer* rep = &controller->reportBuffer;

    controller->battery = inRep->battery_level;
    controller->isCharging = inRep->battery_charging;

    rep->buttons = 0;

    // Make sure the controller actually has the stick in question, to avoid invalid data
    if (sdata->device != SWITCH_DEVICE_JOYCON_RIGHT && sdata->device != SWITCH_DEVICE_TP_JOYCON_RIGHT) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_calib_x, SWITCH_AXIS_X(inRep->left_stick));
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_calib_y, SWITCH_AXIS_Y(inRep->left_stick));
    }

    if (sdata->device != SWITCH_DEVICE_JOYCON_LEFT && sdata->device != SWITCH_DEVICE_TP_JOYCON_LEFT && sdata->device != SWITCH_DEVICE_N64) {
        rep->right_stick_x = calibrateStickAxis(&sdata->right_calib_x, SWITCH_AXIS_X(inRep->right_stick));
        rep->right_stick_y = -calibrateStickAxis(&sdata->right_calib_y, SWITCH_AXIS_Y(inRep->right_stick));
    }

    if (inRep->buttons.y)
        rep->buttons |= BTN(SWITCH_BUTTON_Y);
    if (inRep->buttons.x)
        rep->buttons |= BTN(SWITCH_BUTTON_X);
    if (inRep->buttons.b)
        rep->buttons |= BTN(SWITCH_BUTTON_B);
    if (inRep->buttons.a)
        rep->buttons |= BTN(SWITCH_BUTTON_A);
    if (inRep->buttons.r)
        rep->buttons |= BTN(SWITCH_TRIGGER_R);
    if (inRep->buttons.zr)
        rep->buttons |= BTN(SWITCH_TRIGGER_ZR);
    if (inRep->buttons.minus)
        rep->buttons |= BTN(SWITCH_BUTTON_MINUS);
    if (inRep->buttons.plus)
        rep->buttons |= BTN(SWITCH_BUTTON_PLUS);
    if (inRep->buttons.rstick)
        rep->buttons |= BTN(SWITCH_BUTTON_STICK_R);
    if (inRep->buttons.lstick)
        rep->buttons |= BTN(SWITCH_BUTTON_STICK_L);
    if (inRep->buttons.home)
        rep->buttons |= BTN(SWITCH_BUTTON_HOME);
    if (inRep->buttons.capture)
        rep->buttons |= BTN(SWITCH_BUTTON_CAPTURE);
    if (inRep->buttons.down)
        rep->buttons |= BTN(SWITCH_BUTTON_DOWN);
    if (inRep->buttons.up)
        rep->buttons |= BTN(SWITCH_BUTTON_UP);
    if (inRep->buttons.right)
        rep->buttons |= BTN(SWITCH_BUTTON_RIGHT);
    if (inRep->buttons.left)
        rep->buttons |= BTN(SWITCH_BUTTON_LEFT);
    if (inRep->buttons.l)
        rep->buttons |= BTN(SWITCH_TRIGGER_L);
    if (inRep->buttons.zl)
        rep->buttons |= BTN(SWITCH_TRIGGER_ZL);
    if (inRep->buttons.sl_r)
        rep->buttons |= BTN(SWITCH_TRIGGER_SL_R);
    if (inRep->buttons.sr_r)
        rep->buttons |= BTN(SWITCH_TRIGGER_SR_R);
    if (inRep->buttons.sl_l)
        rep->buttons |= BTN(SWITCH_TRIGGER_SL_L);
    if (inRep->buttons.sr_l)
        rep->buttons |= BTN(SWITCH_TRIGGER_SR_L);

    if (!controller->isReady) {
        controller->isReady = 1;
    }
}

static void handle_basic_input_report(Controller* controller, SwitchBasicInputReport* inRep)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData; 
    BloopairReportBuffer* rep = &controller->reportBuffer;

    // the pro controller sends weird stick data in the first report
    // which completely messes with the start calibration, so we drop that report
    if (sdata->first_report) {
        sdata->first_report = 0;
        return;
    }

    // Make sure the controller actually has the stick in question, to avoid invalid data
    if (sdata->device != SWITCH_DEVICE_JOYCON_RIGHT && sdata->device != SWITCH_DEVICE_TP_JOYCON_RIGHT) {
        rep->left_stick_x = remapBasicStickAxis(&sdata->left_extent_x, bswap16(inRep->left_stick_x));
        rep->left_stick_y = remapBasicStickAxis(&sdata->left_extent_y, bswap16(inRep->left_stick_y));
    }


    if (sdata->device != SWITCH_DEVICE_JOYCON_LEFT && sdata->device != SWITCH_DEVICE_TP_JOYCON_LEFT && sdata->device != SWITCH_DEVICE_N64) {
        rep->right_stick_x = remapBasicStickAxis(&sdata->right_extent_x, bswap16(inRep->right_stick_x));
        rep->right_stick_y = remapBasicStickAxis(&sdata->right_extent_y, bswap16(inRep->right_stick_y));
    }

    rep->buttons = 0;

    if (inRep->buttons.dpad < 9)
        rep->buttons |= dpad_map[inRep->buttons.dpad];

    if (inRep->buttons.b)
        rep->buttons |= BTN(SWITCH_BUTTON_B);
    if (inRep->buttons.a)
        rep->buttons |= BTN(SWITCH_BUTTON_A);
    if (inRep->buttons.y)
        rep->buttons |= BTN(SWITCH_BUTTON_Y);
    if (inRep->buttons.x)
        rep->buttons |= BTN(SWITCH_BUTTON_X);
    if (inRep->buttons.l)
        rep->buttons |= BTN(SWITCH_TRIGGER_L);
    if (inRep->buttons.r)
        rep->buttons |= BTN(SWITCH_TRIGGER_R);
    if (inRep->buttons.zl)
        rep->buttons |= BTN(SWITCH_TRIGGER_ZL);
    if (inRep->buttons.zr)
        rep->buttons |= BTN(SWITCH_TRIGGER_ZR);
    if (inRep->buttons.minus)
        rep->buttons |= BTN(SWITCH_BUTTON_MINUS);
    if (inRep->buttons.plus)
        rep->buttons |= BTN(SWITCH_BUTTON_PLUS);
    if (inRep->buttons.lstick)
        rep->buttons |= BTN(SWITCH_BUTTON_STICK_L);
    if (inRep->buttons.rstick)
        rep->buttons |= BTN(SWITCH_BUTTON_STICK_R);
    if (inRep->buttons.home)
        rep->buttons |= BTN(SWITCH_BUTTON_HOME);
    if (inRep->buttons.capture)
        rep->buttons |= BTN(SWITCH_BUTTON_CAPTURE);
}

void controllerData_switch(Controller* controller, uint8_t* buf, uint16_t len)
{
    if (buf[0] == SWITCH_COMMAND_INPUT_REPORT_ID) {
        SwitchCommandInputReport* rep = (SwitchCommandInputReport*) buf;
        if (controller->isReady) {
            handle_input_report(controller, &rep->input);
        }
        handle_command_response(controller, &rep->response);
    } else if (buf[0] == SWITCH_INPUT_REPORT_ID) {
        handle_input_report(controller, (SwitchInputReport*) buf);
    } else if (buf[0] == SWITCH_BASIC_INPUT_REPORT_ID) {
        handle_basic_input_report(controller, (SwitchBasicInputReport*) buf);
    }
}

void controllerDeinit_switch(Controller* controller)
{
    IOS_Free(LOCAL_PROCESS_HEAP_ID, controller->additionalData);
}

void controllerInit_switch(Controller* controller)
{
    controller->data = controllerData_switch;
    controller->setPlayerLed = controllerSetLed_switch;
    controller->rumble = controllerRumble_switch;
    controller->deinit = controllerDeinit_switch;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    SwitchData* sdata = (SwitchData*) IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(SwitchData));
    memset(sdata, 0, sizeof(SwitchData));
    sdata->first_report = 1;
 
    // Initial basic extents (start with the partial stick range, which gets dynamically extended)
    // Note that these values shouldn't be too small, otherwise the resting values get scaled by a big factor, messing with the initial calibration
    sdata->left_extent_x.max = sdata->right_extent_x.max = sdata->left_extent_y.max = sdata->right_extent_y.max = 26214; // 32767 * 0.8f;
    sdata->left_extent_x.min = sdata->right_extent_x.min = sdata->left_extent_y.min = sdata->right_extent_y.min = -26214; //-32768 * 0.8f;

    controller->additionalData = sdata;

    // start as a generic controller until we figure out the type
    controller->type = BLOOPAIR_CONTROLLER_SWITCH_GENERIC;
    Configuration_GetAll(controller->type, controller->bda,
        &controller->commonConfig, &controller->mapping,
        &controller->customConfig, &controller->customConfigSize);

    // start controller initialization by requesting device info
    requestDeviceInfo(controller);
}

void controllerModuleInit_switch(void)
{
    // set default mappings
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH_GENERIC, NULL, &default_generic_mapping, &default_switch_configuration, sizeof(default_switch_configuration));
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT, NULL, &default_joycon_left_mapping, &default_switch_configuration, sizeof(default_switch_configuration));
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT, NULL, &default_joycon_right_mapping, &default_switch_configuration, sizeof(default_switch_configuration));
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH_PRO, NULL, &default_pro_controller_mapping, &default_switch_configuration, sizeof(default_switch_configuration));
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH_N64, NULL, &default_n64_mapping, &default_switch_configuration, sizeof(default_switch_configuration));
}
