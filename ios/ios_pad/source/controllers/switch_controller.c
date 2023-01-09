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

#define AXIS_NORMALIZE_VALUE               1140
#define DPAD_EMULATION_DEAD_ZONE           500

// These values are based on https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md
#define RUMBLE_HIGH_FREQUENCY 0x7400 // 150 Hz
#define RUMBLE_HIGH_AMPLITUDE 0xc8 // 1.003
#define RUMBLE_LOW_FREQUENCY 0x3d // 150 Hz
#define RUMBLE_LOW_AMPLITUDE 0x0072 // 1.003

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

static void parseRawStickCalibration(SwitchData* sdata, SwitchRawStickCalibration* raw)
{
    sdata->left_calib_x.center = SWITCH_AXIS_X(raw->left_stick_center);
    sdata->left_calib_x.max = sdata->left_calib_x.center + SWITCH_AXIS_X(raw->left_stick_max);
    sdata->left_calib_x.min = sdata->left_calib_x.center - SWITCH_AXIS_X(raw->left_stick_min);

    sdata->left_calib_y.center = SWITCH_AXIS_Y(raw->left_stick_center);
    sdata->left_calib_y.max = sdata->left_calib_y.center + SWITCH_AXIS_Y(raw->left_stick_max);
    sdata->left_calib_y.min = sdata->left_calib_y.center - SWITCH_AXIS_Y(raw->left_stick_min);

    sdata->right_calib_x.center = SWITCH_AXIS_X(raw->right_stick_center);
    sdata->right_calib_x.max = sdata->right_calib_x.center + SWITCH_AXIS_X(raw->right_stick_max);
    sdata->right_calib_x.min = sdata->right_calib_x.center - SWITCH_AXIS_X(raw->right_stick_min);

    sdata->right_calib_y.center = SWITCH_AXIS_Y(raw->right_stick_center);
    sdata->right_calib_y.max = sdata->right_calib_y.center + SWITCH_AXIS_Y(raw->right_stick_max);
    sdata->right_calib_y.min = sdata->right_calib_y.center - SWITCH_AXIS_Y(raw->right_stick_min);
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
    if (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT) {
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

static void handle_command_response(Controller* controller, SwitchCommandResponse* resp)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;

    DEBUG("subcmd respone %d\n", resp->command);

    if ((resp->ack & 0x80) == 0) {
        DEBUG("switch subcmd %d failed\n", resp->command);
        return;
    }

    if (resp->command == SWITCH_COMMAND_REQUEST_DEVICE_INFO) {
        sdata->device = resp->device_info.device_type;
        DEBUG("device type %d\n", sdata->device);

        // set the leds now that we know the device type
        setPlayerLeds(controller);

        // enable rumble
        setVibration(controller, 1);

        if ((sdata->device == SWITCH_DEVICE_JOYCON_LEFT) ||
            (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT) ||
            (sdata->device == SWITCH_DEVICE_PRO_CONTROLLER) ||
            (sdata->device == SWITCH_DEVICE_N64_CONTROLLER)) {
            // Read the user calibration magic to check if user calibration exists
            readSpiFlash(controller, SWITCH_USER_CALIBRATION_MAGIC_ADDRESS, 2);
        } else {
            // Don't need to wait for calibration, controller is ready now
            if (!controller->isReady) {
                controller->isReady = 1;
            }
        }
    } else if (resp->command == SWITCH_COMMAND_SPI_FLASH_READ) {
        uint32_t address = bswap32(resp->spi_flash_read.address);

        switch (address) {
        case SWITCH_USER_CALIBRATION_MAGIC_ADDRESS:
            // Check for user calibration
            if (resp->spi_flash_read.data[0] == 0xb2 && resp->spi_flash_read.data[1] == 0xa1) {
                // Read user calibration
                readSpiFlash(controller, SWITCH_USER_CALIBRATION_ADDRESS, sizeof(SwitchRawStickCalibration));
            } else {
                // Fall back to factory calibration
                readSpiFlash(controller, SWITCH_FACTORY_CALIBRATION_ADDRESS, sizeof(SwitchRawStickCalibration));
            }
            break;
        case SWITCH_FACTORY_CALIBRATION_ADDRESS:
        case SWITCH_USER_CALIBRATION_ADDRESS:
            // parse the calibration data
            parseRawStickCalibration(sdata, (SwitchRawStickCalibration*) resp->spi_flash_read.data);

            // we can now enable full reports
            setInputReportMode(controller, SWITCH_INPUT_REPORT_ID);
            break;
        default:
            DEBUG("switch: unknown SPI read from %lx size %d\n", address, resp->spi_flash_read.size);
            break;
        }
    }
}

static void handle_input_report(Controller* controller, SwitchInputReport* inRep)
{
    SwitchData* sdata = (SwitchData*) controller->additionalData;
    ReportBuffer* rep = &controller->reportBuffer;

    controller->battery = inRep->battery_level;
    controller->isCharging = inRep->battery_charging;

    rep->buttons = 0;

    if (sdata->device == SWITCH_DEVICE_JOYCON_LEFT) {
        int16_t left_stick_x = calibrateStickAxis(&sdata->left_calib_x, SWITCH_AXIS_X(inRep->left_stick));
        int16_t left_stick_y = calibrateStickAxis(&sdata->left_calib_y, SWITCH_AXIS_Y(inRep->left_stick));

        // map stick to dpad
        if (left_stick_x < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (left_stick_x > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (left_stick_y < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (left_stick_y > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;

        if (inRep->buttons.minus)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (inRep->buttons.plus)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        // left joy-con only has capture button, let's map it to home
        if (inRep->buttons.capture)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;

        // map the dpad to abxy
        if (inRep->buttons.down)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (inRep->buttons.up)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (inRep->buttons.right)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (inRep->buttons.left)
            rep->buttons |= WPAD_PRO_BUTTON_B;

        if (inRep->buttons.sr_l)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (inRep->buttons.sl_l)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
    } else if (sdata->device == SWITCH_DEVICE_JOYCON_RIGHT) {
        int16_t right_stick_x = calibrateStickAxis(&sdata->right_calib_x, SWITCH_AXIS_X(inRep->right_stick));
        int16_t right_stick_y = calibrateStickAxis(&sdata->right_calib_y, SWITCH_AXIS_Y(inRep->right_stick));

        // map stick to dpad
        if (right_stick_x > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (right_stick_x < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (right_stick_y > DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (right_stick_y < -DPAD_EMULATION_DEAD_ZONE)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;

        // rotate abxy for sidewise joy-con
        if (inRep->buttons.y)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (inRep->buttons.x)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (inRep->buttons.b)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (inRep->buttons.a)
            rep->buttons |= WPAD_PRO_BUTTON_B;

        if (inRep->buttons.sr_r)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (inRep->buttons.sl_r)
            rep->buttons |= WPAD_PRO_TRIGGER_L;

        if (inRep->buttons.plus)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (inRep->buttons.home)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
    } else if (sdata->device == SWITCH_DEVICE_PRO_CONTROLLER) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_calib_x, SWITCH_AXIS_X(inRep->left_stick));
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_calib_y, SWITCH_AXIS_Y(inRep->left_stick));
        rep->right_stick_x = calibrateStickAxis(&sdata->right_calib_x, SWITCH_AXIS_X(inRep->right_stick));
        rep->right_stick_y = -calibrateStickAxis(&sdata->right_calib_y, SWITCH_AXIS_Y(inRep->right_stick));

        if (inRep->buttons.y)
            rep->buttons |= WPAD_PRO_BUTTON_Y;
        if (inRep->buttons.x)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (inRep->buttons.b)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (inRep->buttons.a)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (inRep->buttons.r)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (inRep->buttons.zr)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (inRep->buttons.minus)
            rep->buttons |= WPAD_PRO_BUTTON_MINUS;
        if (inRep->buttons.plus)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (inRep->buttons.rstick)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
        if (inRep->buttons.lstick)
            rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
        if (inRep->buttons.home)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        // map the capture button to the reserved button bit
        if (inRep->buttons.capture)
            rep->buttons |= WPAD_PRO_RESERVED;
        if (inRep->buttons.down)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (inRep->buttons.up)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (inRep->buttons.right)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (inRep->buttons.left)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;
        if (inRep->buttons.l)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (inRep->buttons.zl)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
    } else if (sdata->device == SWITCH_DEVICE_N64_CONTROLLER) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_calib_x, SWITCH_AXIS_X(inRep->left_stick));
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_calib_y, SWITCH_AXIS_Y(inRep->left_stick));
        rep->right_stick_y = 0;
        rep->right_stick_x = 0;

        // map the C buttons to the right analog stick
        if (inRep->buttons.y)
            rep->right_stick_y -= AXIS_NORMALIZE_VALUE;
        if (inRep->buttons.x)
            rep->right_stick_x -= AXIS_NORMALIZE_VALUE;
        if (inRep->buttons.minus)
            rep->right_stick_x += AXIS_NORMALIZE_VALUE;
        if (inRep->buttons.zr)
            rep->right_stick_y += AXIS_NORMALIZE_VALUE;

        if (inRep->buttons.b)
            rep->buttons |= WPAD_PRO_BUTTON_B;
        if (inRep->buttons.a)
            rep->buttons |= WPAD_PRO_BUTTON_A;
        if (inRep->buttons.r)
            rep->buttons |= WPAD_PRO_TRIGGER_R;
        if (inRep->buttons.plus)
            rep->buttons |= WPAD_PRO_BUTTON_PLUS;
        if (inRep->buttons.lstick)
            rep->buttons |= WPAD_PRO_TRIGGER_ZR;
        if (inRep->buttons.home)
            rep->buttons |= WPAD_PRO_BUTTON_HOME;
        // map capture button to X
        if (inRep->buttons.capture)
            rep->buttons |= WPAD_PRO_BUTTON_X;
        if (inRep->buttons.down)
            rep->buttons |= WPAD_PRO_BUTTON_DOWN;
        if (inRep->buttons.up)
            rep->buttons |= WPAD_PRO_BUTTON_UP;
        if (inRep->buttons.right)
            rep->buttons |= WPAD_PRO_BUTTON_RIGHT;
        if (inRep->buttons.left)
            rep->buttons |= WPAD_PRO_BUTTON_LEFT;
        if (inRep->buttons.l)
            rep->buttons |= WPAD_PRO_TRIGGER_L;
        if (inRep->buttons.zl)
            rep->buttons |= WPAD_PRO_TRIGGER_ZL;
    }

    if (!controller->isReady) {
        controller->isReady = 1;
    }
}

static void handle_basic_input_report(Controller* controller, SwitchBasicInputReport* inRep)
{
    ReportBuffer* rep = &controller->reportBuffer;

    rep->left_stick_x = scaleStickAxis(bswap16(inRep->left_stick_x), 65536);
    rep->right_stick_x = scaleStickAxis(bswap16(inRep->right_stick_x), 65536);
    rep->left_stick_y = scaleStickAxis(bswap16(inRep->left_stick_y), 65536);
    rep->right_stick_y = scaleStickAxis(bswap16(inRep->right_stick_y), 65536);

    rep->buttons = 0;

    if (inRep->buttons.dpad < 9)
        rep->buttons |= dpad_map[inRep->buttons.dpad];

    if (inRep->buttons.b)
        rep->buttons |= WPAD_PRO_BUTTON_B;
    if (inRep->buttons.a)
        rep->buttons |= WPAD_PRO_BUTTON_A;
    if (inRep->buttons.y)
        rep->buttons |= WPAD_PRO_BUTTON_Y;
    if (inRep->buttons.x)
        rep->buttons |= WPAD_PRO_BUTTON_X;
    if (inRep->buttons.l)
        rep->buttons |= WPAD_PRO_TRIGGER_L;
    if (inRep->buttons.r)
        rep->buttons |= WPAD_PRO_TRIGGER_R;
    if (inRep->buttons.zl)
        rep->buttons |= WPAD_PRO_TRIGGER_ZL;
    if (inRep->buttons.zr)
        rep->buttons |= WPAD_PRO_TRIGGER_ZR;
    if (inRep->buttons.minus)
        rep->buttons |= WPAD_PRO_BUTTON_MINUS;
    if (inRep->buttons.plus)
        rep->buttons |= WPAD_PRO_BUTTON_PLUS;
    if (inRep->buttons.lstick)
        rep->buttons |= WPAD_PRO_BUTTON_STICK_L;
    if (inRep->buttons.rstick)
        rep->buttons |= WPAD_PRO_BUTTON_STICK_R;
    if (inRep->buttons.home)
        rep->buttons |= WPAD_PRO_BUTTON_HOME;
    // if (inRep->buttons.capture)
    //     rep->buttons |= WPAD_PRO_BUTTON_;
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

    controller->additionalData = IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(SwitchData));
    memset(controller->additionalData, 0, sizeof(SwitchData));

    // start controller initialization by requesting device info
    requestDeviceInfo(controller);
}
