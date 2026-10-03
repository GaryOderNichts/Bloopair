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
#include "switch2_controller.h"
#include "bleext/hid_switch2.h"
#include "bleext/smp.h"
#include "device_info.h"
#include "stack/btm.h"
#include "wud.h"
#include <bloopair/controllers/switch2_controller.h>

// Normalize value for switch -> wii u range
#define AXIS_NORMALIZE_VALUE 1140

// Values based on
// <https://github.com/libsdl-org/SDL/blob/4a17e772ea9a4ff77dd10fa6ea6f45fc6731eb01/src/joystick/hidapi/SDL_hidapi_switch2.c#L37>
// <https://github.com/libsdl-org/SDL/blob/4a17e772ea9a4ff77dd10fa6ea6f45fc6731eb01/src/joystick/hidapi/SDL_hidapi_switch2.c#L614-L615>
#define RUMBLE_HIGH_FREQUENCY 0x187
#define RUMBLE_HIGH_AMPLITUDE (29000 / 2)
#define RUMBLE_LOW_FREQUENCY  0x112
#define RUMBLE_LOW_AMPLITUDE  (29000 / 2)

static const MappingConfiguration default_joycon_left_mapping = {
    .num = 12,
    .mappings = {
        // map stick to dpad (assume a sideways joycon)
        { BLOOPAIR_PRO_STICK_L_DOWN,    BLOOPAIR_PRO_BUTTON_RIGHT, },
        { BLOOPAIR_PRO_STICK_L_UP,      BLOOPAIR_PRO_BUTTON_LEFT, },
        { BLOOPAIR_PRO_STICK_L_RIGHT,   BLOOPAIR_PRO_BUTTON_UP, },
        { BLOOPAIR_PRO_STICK_L_LEFT,    BLOOPAIR_PRO_BUTTON_DOWN, },

        { SWITCH2_BUTTON_MINUS,          BLOOPAIR_PRO_BUTTON_MINUS, },
        // left joy-con only has capture button, let's map it to home
        { SWITCH2_BUTTON_CAPTURE,        BLOOPAIR_PRO_BUTTON_HOME, },

        // map the dpad to abxy
        { SWITCH2_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_A, },
        { SWITCH2_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH2_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH2_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_B, },

        { SWITCH2_TRIGGER_SR_L,          BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH2_TRIGGER_SL_L,          BLOOPAIR_PRO_TRIGGER_L, },
    },
};

static const MappingConfiguration default_joycon_right_mapping = {
    .num = 13,
    .mappings = {
        // map stick to dpad (assume a sideways joycon)
        { BLOOPAIR_PRO_STICK_R_DOWN,    BLOOPAIR_PRO_BUTTON_LEFT, },
        { BLOOPAIR_PRO_STICK_R_UP,      BLOOPAIR_PRO_BUTTON_RIGHT, },
        { BLOOPAIR_PRO_STICK_R_RIGHT,   BLOOPAIR_PRO_BUTTON_DOWN, },
        { BLOOPAIR_PRO_STICK_R_LEFT,    BLOOPAIR_PRO_BUTTON_UP, },

        // rotate abxy for sidewise joy-con
        { SWITCH2_BUTTON_Y,              BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH2_BUTTON_X,              BLOOPAIR_PRO_BUTTON_A, },
        { SWITCH2_BUTTON_B,              BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH2_BUTTON_A,              BLOOPAIR_PRO_BUTTON_B, },

        { SWITCH2_TRIGGER_SR_R,          BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH2_TRIGGER_SL_R,          BLOOPAIR_PRO_TRIGGER_L, },

        { SWITCH2_BUTTON_PLUS,           BLOOPAIR_PRO_BUTTON_PLUS, },
        { SWITCH2_BUTTON_HOME,           BLOOPAIR_PRO_BUTTON_HOME, },

        // not sure what to do with the C button
        { SWITCH2_BUTTON_C,              BLOOPAIR_PRO_RESERVED, },
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

        { SWITCH2_BUTTON_Y,              BLOOPAIR_PRO_BUTTON_Y, },
        { SWITCH2_BUTTON_X,              BLOOPAIR_PRO_BUTTON_X, },
        { SWITCH2_BUTTON_B,              BLOOPAIR_PRO_BUTTON_B, },
        { SWITCH2_BUTTON_A,              BLOOPAIR_PRO_BUTTON_A, },

        { SWITCH2_TRIGGER_R,             BLOOPAIR_PRO_TRIGGER_R, },
        { SWITCH2_TRIGGER_ZR,            BLOOPAIR_PRO_TRIGGER_ZR, },

        { SWITCH2_BUTTON_MINUS,          BLOOPAIR_PRO_BUTTON_MINUS, },
        { SWITCH2_BUTTON_PLUS,           BLOOPAIR_PRO_BUTTON_PLUS, },
        { SWITCH2_BUTTON_STICK_R,        BLOOPAIR_PRO_BUTTON_STICK_R, },
        { SWITCH2_BUTTON_STICK_L,        BLOOPAIR_PRO_BUTTON_STICK_L, },
        { SWITCH2_BUTTON_HOME,           BLOOPAIR_PRO_BUTTON_HOME, },
        // map the capture button to the reserved button bit
        { SWITCH2_BUTTON_CAPTURE,        BLOOPAIR_PRO_RESERVED, },

        { SWITCH2_BUTTON_DOWN,           BLOOPAIR_PRO_BUTTON_DOWN, },
        { SWITCH2_BUTTON_UP,             BLOOPAIR_PRO_BUTTON_UP },
        { SWITCH2_BUTTON_RIGHT,          BLOOPAIR_PRO_BUTTON_RIGHT, },
        { SWITCH2_BUTTON_LEFT,           BLOOPAIR_PRO_BUTTON_LEFT, },

        { SWITCH2_TRIGGER_L,             BLOOPAIR_PRO_TRIGGER_L, },
        { SWITCH2_TRIGGER_ZL,            BLOOPAIR_PRO_TRIGGER_ZL, },
    },
};

typedef struct {
    int16_t neutral;
    int16_t max;
    int16_t min;
} Switch2StickCalibrationAxis;

typedef struct {
    Switch2StickCalibrationAxis x, y;
} Switch2StickCalibration;

typedef struct {
    Switch2StickCalibration left_stick_calib, right_stick_calib;

    uint8_t vibration_counter;

    uint8_t pairing_retry_count;
    uint8_t pairing_host_key[16];
    uint8_t pairing_ltk[16];
    uint8_t pairing_challenge[16];
} Switch2Data;

/*------------------*/
/* Helper functions */
/*------------------*/

static BloopairControllerType getControllerType(uint16_t vid, uint16_t pid)
{
    if (vid == 0x057e && pid == 0x2066) {
        return BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_RIGHT;
    } else if (vid == 0x057e && pid == 0x2067) {
        return BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_LEFT;
    } else if (vid == 0x057e && pid == 0x2069) {
        return BLOOPAIR_CONTROLLER_SWITCH2_PRO;
    } else if (vid == 0x057e && pid == 0x2073) {
        return BLOOPAIR_CONTROLLER_SWITCH2_GAMECUBE;
    }

    // TODO handle this properly
    DEBUG_PRINT("SW2: Unknown controller %x:%x\n", vid, pid);
    return (BloopairControllerType) -1;
};

static uint8_t verifyChallengeResponse(Switch2Data* sdata, uint8_t response[16])
{
    uint8_t reversed_ltk[16];
    uint8_t reversed_challenge[16];
    for (int i = 0; i < 16; i++) {
        reversed_ltk[i] = sdata->pairing_ltk[15 - i];
        reversed_challenge[i] = sdata->pairing_challenge[15 - i];
    }

    int* handle = createIOSCAesKeyHandle(reversed_ltk, 16);
    if (handle) {
        aesEcbEncrypt(handle, reversed_challenge, 16, reversed_challenge, 16);
        destroyIOSCAesKeyHandle(handle);
    }

    return memcmp(reversed_challenge, response, 16) == 0;
}

static void parseStickCalibration(Switch2StickCalibration* calib, Switch2RawStickCalibration* raw)
{
    calib->x.neutral = SWITCH2_AXIS_X(raw->neutral);
    calib->y.neutral = SWITCH2_AXIS_Y(raw->neutral);
    calib->x.max = SWITCH2_AXIS_X(raw->max);
    calib->x.min = SWITCH2_AXIS_X(raw->min);
    calib->y.max = SWITCH2_AXIS_Y(raw->max);
    calib->y.min = SWITCH2_AXIS_Y(raw->min);
}

static int16_t calibrateStickAxis(Switch2StickCalibrationAxis* calib, uint32_t value)
{
    int32_t calibrated = (int32_t) value - calib->neutral;
    if (calibrated < 0) {
        calibrated = (calibrated * AXIS_NORMALIZE_VALUE) / calib->min;
    } else {
        calibrated = (calibrated * AXIS_NORMALIZE_VALUE) / calib->max;
    }
    return (int16_t) CLAMP(calibrated, -AXIS_NORMALIZE_VALUE, AXIS_NORMALIZE_VALUE);
}

/*-------------------*/
/* Command functions */
/*-------------------*/

static void initCommand(Switch2CommandRequest* request, uint8_t command, uint8_t subcommand, uint8_t length)
{
    memset(request, 0, sizeof(Switch2CommandRequest));
    request->header.command = command;
    request->header.direction = SWITCH2_DIRECTION_REQUEST;
    request->header.transport = SWITCH2_TRANSPORT_BLE;
    request->header.subcommand = subcommand;
    request->header.length = length;
}

static void sendCommand(Controller* controller, Switch2CommandRequest* request)
{
    HID_SW2_SendData(controller->handle, SWITCH2_OUTPUT_REPORT_CMD_HANDLE, request,
                     sizeof(Switch2CommandHeader) + request->header.length);
}

static void flashReadBlock(Controller* controller, uint32_t address)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_FLASH, SWITCH2_SUBCOMMAND_FLASH_READ_BLOCK, sizeof(Switch2FlashReadRequest));
    command.flash.read.address = bswap32(address);
    sendCommand(controller, &command);
}

#if 0
static void flashRead(Controller* controller, uint32_t address, uint8_t length)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_FLASH, SWITCH2_SUBCOMMAND_FLASH_READ, sizeof(Switch2FlashReadRequest));
    command.flash.read.length = length;
    command.flash.read.unknown = 0x7E;
    command.flash.read.address = bswap32(address);
    sendCommand(controller, &command);
}
#endif

static void setLedsPattern(Controller* controller, uint8_t pattern)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_LEDS, SWITCH2_SUBCOMMAND_LEDS_SET_PATTERN,
                sizeof(Switch2LEDSetPatternRequest));
    command.leds.pattern.pattern = pattern;
    sendCommand(controller, &command);
}

#if 0
static void setFeatureMask(Controller* controller, uint8_t features)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_FEATURE, SWITCH2_SUBCOMMAND_FEATURE_SET_MASK,
                sizeof(Switch2FeatureFlagsRequest));
    command.feature.set_mask.flags = features;
    sendCommand(controller, &command);
}

static void enableFeatures(Controller* controller, uint8_t features)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_FEATURE, SWITCH2_SUBCOMMAND_FEATURE_ENABLE,
                sizeof(Switch2FeatureFlagsRequest));
    command.feature.enable.flags = features;
    sendCommand(controller, &command);
}

static void configureFeatures(Controller* controller, uint8_t features)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_FEATURE, SWITCH2_SUBCOMMAND_FEATURE_CONFIGURE,
                sizeof(Switch2FeatureFlagsRequest));
    command.feature.configure.flags = features;
    sendCommand(controller, &command);
}
#endif

static void pairingExchangeAddress(Controller* controller, uint8_t count, BD_ADDR addr[])
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_PAIRING, SWITCH2_SUBCOMMAND_PAIRING_EXCHANGE_ADDR,
                sizeof(Switch2PairingAddressExchangeRequest) + sizeof(BD_ADDR) * count);
    command.pairing.address_exchange.count = count;
    for (uint8_t i = 0; i < count; i++) {
        reverseBDA(command.pairing.address_exchange.addresses[i], addr[i]);
    }
    sendCommand(controller, &command);
}

static void pairingConfirmLTK(Controller* controller, uint8_t challenge[16])
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_PAIRING, SWITCH2_SUBCOMMAND_PAIRING_CONFIRM_LTK,
                sizeof(Switch2PairingConfirmLTKRequest));
    memcpy(command.pairing.ltk_confirm.challenge, challenge, 16);
    sendCommand(controller, &command);
}

static void pairingFinalize(Controller* controller)
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_PAIRING, SWITCH2_SUBCOMMAND_PAIRING_FINALIZE, 1);
    sendCommand(controller, &command);
}

static void pairingExchangeKeys(Controller* controller, uint8_t key[16])
{
    Switch2CommandRequest command;
    initCommand(&command, SWITCH2_COMMAND_PAIRING, SWITCH2_SUBCOMMAND_PAIRING_EXCHANGE_KEYS,
                sizeof(Switch2PairingKeysExchangeRequest));
    memcpy(command.pairing.keys_exchange.key, key, 16);
    sendCommand(controller, &command);
}

/*----------------------*/
/* Encryption callbacks */
/*----------------------*/

static void smpCallback(BD_ADDR addr, SMPEvent event, SMPBondingData* bondingData, void* userPointer)
{
    Controller* controller = (Controller*) userPointer;
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;

    switch (event) {
    case SMP_EVENT_ENCRYPT:
        DEBUG_PRINT("SW2: Link encrypted\n");

        // Restart background connections here now, since we nop'ed out the immediate restart in btm_acl_created
        btm_ble_resume_bg_conn(NULL, 1);

        // Continue reading calibrations now
        flashReadBlock(controller, 0x13080);
        break;
    case SMP_EVENT_FAILURE:
        DEBUG_PRINT("SW2: Failed to start encryption?\n");

        if (sdata->pairing_retry_count < 2) {
            // Not sure what the best way to handle this is, let's just start another pairing sequence?
            pairingExchangeAddress(controller, 1, &gWBC.hostAddress);
        } else {
            HID_SW2_Close(controller->handle);
        }
        sdata->pairing_retry_count++;
        break;

    default:
        break;
    }
}

static void startEncryption(Controller* controller)
{
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;

    // We use our SMP implementation to start encryption, even if we don't do anything of the actual SMP pairing
    SMPBondingData data;
    memcpy(data.ltk, sdata->pairing_ltk, sizeof(data.ltk));
    memset(data.rand, 0, sizeof(data.rand));
    data.ediv = 0;
    SMP_StartEncrypt(controller->bda, &smpCallback, &data, controller);
}

/*----------------------*/
/* Controller callbacks */
/*----------------------*/

static void handleFlashResponse(Controller* controller, Switch2CommandResponse* rsp)
{
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;

    switch (rsp->header.subcommand) {
    case SWITCH2_SUBCOMMAND_FLASH_READ_BLOCK:
        if (rsp->flash.read.address == bswap32(0x13080)) { // Left Factory Calibration
            // Parse stick configuration
            parseStickCalibration(&sdata->left_stick_calib, (Switch2RawStickCalibration*) &rsp->flash.read.data[0x28]);

            // Read next stick calibration
            flashReadBlock(controller, 0x130C0);
        } else if (rsp->flash.read_block.address == bswap32(0x130C0)) { // Right Factory Calibration
            // Parse stick configuration
            parseStickCalibration(&sdata->right_stick_calib, (Switch2RawStickCalibration*) &rsp->flash.read.data[0x28]);

            flashReadBlock(controller, 0x1FC040);
        } else if (rsp->flash.read_block.address == bswap32(0x1FC040)) { // Left User Calibration
            if (rsp->flash.read.data[0] == 0xb2 && rsp->flash.read.data[1] == 0xa1) {
                parseStickCalibration(&sdata->left_stick_calib,
                                      (Switch2RawStickCalibration*) &rsp->flash.read.data[0x02]);
            }

            flashReadBlock(controller, 0x1FC080);
        } else if (rsp->flash.read_block.address == bswap32(0x1FC080)) { // Right User Calibration
            if (rsp->flash.read.data[0] == 0xb2 && rsp->flash.read.data[1] == 0xa1) {
                parseStickCalibration(&sdata->right_stick_calib,
                                      (Switch2RawStickCalibration*) &rsp->flash.read.data[0x02]);
            }

            // Enable input report 0x05
            HID_SW2_EnableReport(controller->handle, SWITCH2_INPUT_REPORT_0x05_CONF);
        }
        break;

    default:
        break;
    }
}

static void handleFeatureResponse(Controller* controller, Switch2CommandResponse* rsp)
{
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;

    switch (rsp->header.subcommand) {
    case SWITCH2_SUBCOMMAND_FEATURE_SET_MASK:
        break;
    case SWITCH2_SUBCOMMAND_FEATURE_CONFIGURE:
        break;
    case SWITCH2_SUBCOMMAND_FEATURE_ENABLE:
        break;
    default:
        break;
    };
}

static void handlePairingResponse(Controller* controller, Switch2CommandResponse* rsp)
{
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;

    switch (rsp->header.subcommand) {
    case SWITCH2_SUBCOMMAND_PAIRING_EXCHANGE_ADDR:
        DEBUG_PRINT("SW2: Device address:\n");
        dumpHex(rsp->pairing.address_exchange.addresses, sizeof(BD_ADDR));

        // Generate a random host key
        generateRandom(sdata->pairing_host_key, sizeof(sdata->pairing_host_key));

        DEBUG_PRINT("SW2: Host key:\n");
        dumpHex(sdata->pairing_host_key, sizeof(sdata->pairing_host_key));

        // Continue with key exchange
        pairingExchangeKeys(controller, sdata->pairing_host_key);
        break;
    case SWITCH2_SUBCOMMAND_PAIRING_EXCHANGE_KEYS:
        DEBUG_PRINT("SW2: Device key:\n");
        dumpHex(rsp->pairing.keys_exchange.key, 16);

        // Calculate LTK
        for (int i = 0; i < 16; i++) {
            sdata->pairing_ltk[i] = sdata->pairing_host_key[i] ^ rsp->pairing.keys_exchange.key[i];
        }

        DEBUG_PRINT("SW2: LTK:\n");
        dumpHex(sdata->pairing_ltk, 16);

        // Get random challenge
        generateRandom(sdata->pairing_challenge, sizeof(sdata->pairing_challenge));

        DEBUG_PRINT("SW2: Challenge:\n");
        dumpHex(sdata->pairing_challenge, 16);

        // Send challenge
        pairingConfirmLTK(controller, sdata->pairing_challenge);
        break;
    case SWITCH2_SUBCOMMAND_PAIRING_CONFIRM_LTK:
        DEBUG_PRINT("SW2: Challenge response:\n");
        dumpHex(rsp->pairing.ltk_confirm.response, 16);

        // Verify challenge response
        // Just print the result, we continue on anyways
        if (verifyChallengeResponse(sdata, rsp->pairing.ltk_confirm.response)) {
            DEBUG_PRINT("SW2: Pairing challenge succeeded\n");
        } else {
            DEBUG_PRINT("SW2: Pairing challenge failed\n");
        }

        // We can finalize the pairing now
        pairingFinalize(controller);
        break;
    case SWITCH2_SUBCOMMAND_PAIRING_FINALIZE: {
        // Pairing is complete, store LTK
        DeviceInfo* info = DeviceInfo_GetOrAllocate(controller->bda);
        if (info) {
            info->magic = MAGIC_SWITCH2;
            info->flush = 1;
            memcpy(info->switch2.ltk, sdata->pairing_ltk, sizeof(sdata->pairing_ltk));

            // Enable bgconn for this device
            DeviceInfo_AddToBgConn(info);
        }

        // Start encrypted link
        startEncryption(controller);
        break;
    }
    default:
        break;
    }
}

static void handleCommandResponse(Controller* controller, Switch2CommandResponse* rsp)
{
    DEBUG_PRINT("SW2: handleCommandResponse 0x%x 0x%x ack 0x%x\n", rsp->header.command, rsp->header.subcommand,
                rsp->header.direction);

    // "direction" contains the result
    if (rsp->header.direction != SWITCH2_DIRECTION_RESPONSE) {
        DEBUG_PRINT("SW2: Command failure?\n");
    }

    switch (rsp->header.command) {
    case SWITCH2_COMMAND_FLASH:
        handleFlashResponse(controller, rsp);
        break;
    case SWITCH2_COMMAND_FEATURE:
        handleFeatureResponse(controller, rsp);
        break;
    case SWITCH2_COMMAND_PAIRING:
        handlePairingResponse(controller, rsp);
        break;
    default:
        break;
    }
}

static void handleInputReport0x05(Controller* controller, Switch2InputReport0x05* inRep)
{
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;
    BloopairReportBuffer* rep = &controller->reportBuffer;

    rep->buttons = 0;

    if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_PRO ||
        controller->type == BLOOPAIR_CONTROLLER_SWITCH2_GAMECUBE) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_stick_calib.x, SWITCH2_AXIS_X(inRep->left_stick));
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_stick_calib.y, SWITCH2_AXIS_Y(inRep->left_stick));
        rep->right_stick_x = calibrateStickAxis(&sdata->right_stick_calib.x, SWITCH2_AXIS_X(inRep->right_stick));
        rep->right_stick_y = -calibrateStickAxis(&sdata->right_stick_calib.y, SWITCH2_AXIS_Y(inRep->right_stick));
    } else if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_LEFT) {
        rep->left_stick_x = calibrateStickAxis(&sdata->left_stick_calib.x, SWITCH2_AXIS_X(inRep->left_stick));
        rep->left_stick_y = -calibrateStickAxis(&sdata->left_stick_calib.y, SWITCH2_AXIS_Y(inRep->left_stick));
    } else if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_RIGHT) {
        // "Left" calibration is the calibration of the primary stick
        rep->right_stick_x = calibrateStickAxis(&sdata->left_stick_calib.x, SWITCH2_AXIS_X(inRep->right_stick));
        rep->right_stick_y = -calibrateStickAxis(&sdata->left_stick_calib.y, SWITCH2_AXIS_Y(inRep->right_stick));
    }

    if (inRep->buttons.y)
        rep->buttons |= BTN(SWITCH2_BUTTON_Y);
    if (inRep->buttons.x)
        rep->buttons |= BTN(SWITCH2_BUTTON_X);
    if (inRep->buttons.b)
        rep->buttons |= BTN(SWITCH2_BUTTON_B);
    if (inRep->buttons.a)
        rep->buttons |= BTN(SWITCH2_BUTTON_A);
    if (inRep->buttons.r)
        rep->buttons |= BTN(SWITCH2_TRIGGER_R);
    if (inRep->buttons.zr)
        rep->buttons |= BTN(SWITCH2_TRIGGER_ZR);
    if (inRep->buttons.minus)
        rep->buttons |= BTN(SWITCH2_BUTTON_MINUS);
    if (inRep->buttons.plus)
        rep->buttons |= BTN(SWITCH2_BUTTON_PLUS);
    if (inRep->buttons.rstick)
        rep->buttons |= BTN(SWITCH2_BUTTON_STICK_R);
    if (inRep->buttons.lstick)
        rep->buttons |= BTN(SWITCH2_BUTTON_STICK_L);
    if (inRep->buttons.home)
        rep->buttons |= BTN(SWITCH2_BUTTON_HOME);
    if (inRep->buttons.c)
        rep->buttons |= BTN(SWITCH2_BUTTON_C);
    if (inRep->buttons.capture)
        rep->buttons |= BTN(SWITCH2_BUTTON_CAPTURE);
    if (inRep->buttons.down)
        rep->buttons |= BTN(SWITCH2_BUTTON_DOWN);
    if (inRep->buttons.up)
        rep->buttons |= BTN(SWITCH2_BUTTON_UP);
    if (inRep->buttons.right)
        rep->buttons |= BTN(SWITCH2_BUTTON_RIGHT);
    if (inRep->buttons.left)
        rep->buttons |= BTN(SWITCH2_BUTTON_LEFT);
    if (inRep->buttons.l)
        rep->buttons |= BTN(SWITCH2_TRIGGER_L);
    if (inRep->buttons.zl)
        rep->buttons |= BTN(SWITCH2_TRIGGER_ZL);
    if (inRep->buttons.sl_r)
        rep->buttons |= BTN(SWITCH2_TRIGGER_SL_R);
    if (inRep->buttons.sr_r)
        rep->buttons |= BTN(SWITCH2_TRIGGER_SR_R);
    if (inRep->buttons.sl_l)
        rep->buttons |= BTN(SWITCH2_TRIGGER_SL_L);
    if (inRep->buttons.sr_l)
        rep->buttons |= BTN(SWITCH2_TRIGGER_SR_L);
    if (inRep->buttons.gl)
        rep->buttons |= BTN(SWITCH2_BUTTON_GL);
    if (inRep->buttons.gr)
        rep->buttons |= BTN(SWITCH2_BUTTON_GR);

    if (!controller->isReady) {
        controller->isReady = 1;
    }
}

void controllerData_switch2(Controller* controller, uint8_t* buf, uint16_t len)
{
    // hid_switch2 prepends reports with handle
    uint16_t handle = buf[0] << 8 | buf[1];

    if (handle == SWITCH2_INPUT_REPORT_CMD_RESP_HANDLE) {
        handleCommandResponse(controller, (Switch2CommandResponse*) (buf + 2));
    } else if (handle == SWITCH2_INPUT_REPORT_0x05_HANDLE) {
        handleInputReport0x05(controller, (Switch2InputReport0x05*) (buf + 2));
    }
}

void controllerSetPlayerLed_switch2(Controller* controller, uint8_t led)
{
    // If this is the right joycon swap led order
    if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_RIGHT) {
        led = ((led & 1) << 3) | ((led & 2) << 1) | ((led & 4) >> 1) | ((led & 8) >> 3);
    }

    setLedsPattern(controller, led);
}

void controllerRumble_switch2(Controller* controller, uint8_t rumble)
{
    Switch2Data* sdata = (Switch2Data*) controller->additionalData;

    static const uint8_t rumble_data[5] = {
        (uint8_t) (RUMBLE_HIGH_FREQUENCY & 0xFF),
        (uint8_t) (((RUMBLE_HIGH_AMPLITUDE >> 4) & 0xfc) | ((RUMBLE_HIGH_FREQUENCY >> 8) & 0x03)),
        (uint8_t) ((RUMBLE_HIGH_AMPLITUDE >> 12) | (RUMBLE_LOW_FREQUENCY << 4)),
        (uint8_t) ((RUMBLE_LOW_AMPLITUDE & 0xc0) | ((RUMBLE_LOW_FREQUENCY >> 4) & 0x3f)),
        (uint8_t) (RUMBLE_LOW_AMPLITUDE >> 8),
    };

    Switch2OutputReportRumble rumbleReport;
    memset(&rumbleReport, 0, sizeof(rumbleReport));

    if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_LEFT ||
        controller->type == BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_RIGHT) {
        rumbleReport.report_id = 0x01;
        if (rumble) {
            memcpy(rumbleReport.joycon.rumble.rumble_data, rumble_data, sizeof(rumble_data));
        } else {
            memset(rumbleReport.joycon.rumble.rumble_data, 0, sizeof(rumbleReport.pro.left_rumble.rumble_data));
        }
        rumbleReport.joycon.rumble.vibration_counter = (0x05 << 4) | (sdata->vibration_counter & 0xf);
    } else if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_PRO) {
        rumbleReport.report_id = 0x02;
        if (rumble) {
            memcpy(rumbleReport.pro.left_rumble.rumble_data, rumble_data, sizeof(rumble_data));
            memcpy(rumbleReport.pro.right_rumble.rumble_data, rumble_data, sizeof(rumble_data));
        } else {
            memset(rumbleReport.pro.left_rumble.rumble_data, 0, sizeof(rumbleReport.pro.left_rumble.rumble_data));
            memset(rumbleReport.pro.right_rumble.rumble_data, 0, sizeof(rumbleReport.pro.right_rumble.rumble_data));
        }
        rumbleReport.pro.left_rumble.vibration_counter = (0x05 << 4) | (sdata->vibration_counter & 0xf);
        rumbleReport.pro.right_rumble.vibration_counter = (0x05 << 4) | (sdata->vibration_counter & 0xf);
    } else if (controller->type == BLOOPAIR_CONTROLLER_SWITCH2_GAMECUBE) {
        rumbleReport.report_id = 0x03;
        rumbleReport.gc.vibration_counter = (0x05 << 4) | (sdata->vibration_counter & 0xf);
        rumbleReport.gc.rumble[0] = rumble ? 1 : 2; // ???
    }

    HID_SW2_SendData(controller->handle, SWITCH2_OUTPUT_REPORT_RUMBLE_HANDLE, &rumbleReport, sizeof(rumbleReport));
    sdata->vibration_counter++;
}

void controllerDeinit_switch2(Controller* controller)
{
    IOS_Free(LOCAL_PROCESS_HEAP_ID, controller->additionalData);
}

void controllerInit_switch2(Controller* controller)
{
    controller->data = controllerData_switch2;
    controller->setPlayerLed = controllerSetPlayerLed_switch2;
    controller->rumble = controllerRumble_switch2;
    controller->deinit = controllerDeinit_switch2;
    controller->update = NULL;

    controller->battery = 4;
    controller->isCharging = 0;

    Switch2Data* sdata = (Switch2Data*) IOS_Alloc(LOCAL_PROCESS_HEAP_ID, sizeof(Switch2Data));
    memset(sdata, 0, sizeof(Switch2Data));

    controller->additionalData = sdata;

    controller->type = getControllerType(controller->vendor_id, controller->product_id);
    Configuration_GetAll(controller->type, controller->bda, &controller->commonConfig, &controller->mapping,
                         &controller->customConfig, &controller->customConfigSize);

    // Enable command responses
    HID_SW2_EnableReport(controller->handle, SWITCH2_INPUT_REPORT_CMD_RESP_CONF);

    DeviceInfo* info = DeviceInfo_Get(controller->bda);
    if (!info) {
        return;
    }

    if (info->switch2.pairing_complete) {
        // Copy over LTK if we already paired
        memcpy(sdata->pairing_ltk, info->switch2.ltk, sizeof(info->switch2.ltk));

        // Start encryption
        startEncryption(controller);
    } else {
        // Start pairing sequence by exchanging host address to controller
        pairingExchangeAddress(controller, 1, &gWBC.hostAddress);
    }
}

void controllerModuleInit_switch2(void)
{
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_LEFT, NULL, &default_joycon_left_mapping, NULL, 0);
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH2_JOYCON_RIGHT, NULL, &default_joycon_right_mapping, NULL, 0);
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH2_PRO, NULL, &default_pro_controller_mapping, NULL, 0);
    // Use the pro controller mapping for the gamecube controller, close enough
    Configuration_SetFallback(BLOOPAIR_CONTROLLER_SWITCH2_GAMECUBE, NULL, &default_pro_controller_mapping, NULL, 0);
}
