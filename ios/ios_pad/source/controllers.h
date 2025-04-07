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

#pragma once

#include <imports.h>
#include "main.h"
#include "bta/bta_hh.h"
#include "wiimote_crypto.h"
#include "configuration.h"
#include <bloopair/controllers/common.h>

// Information about button bits and the report can be found here:
// - <https://github.com/devkitPro/wut/blob/master/include/padscore/wpad.h>
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-wiimote-modules.c#L1627>

typedef struct PACKED {
    uint8_t report_id;
    struct PACKED {
        uint16_t left_stick_x;
        uint16_t right_stick_x;
        uint16_t left_stick_y;
        uint16_t right_stick_y;
        uint16_t buttons;
        uint8_t battery : 4;
        uint8_t usb_connected : 1;
        uint8_t charging : 1;
        uint8_t stick_buttons : 2;
        uint8_t unused[10];
    } data;
} WPADProReport;
CHECK_SIZE(WPADProReport, 22);

typedef struct Controller Controller;

typedef void (*ControllerDeinitFn)(Controller* controller);
typedef void (*ControllerDataFn)(Controller* controller, uint8_t* data, uint16_t len);
typedef void (*ControllerSetPlayerLedFn)(Controller* controller, uint8_t led);
typedef void (*ControllerRumbleFn)(Controller* controller, uint8_t rumble);
typedef void (*ControllerUpdateFn)(Controller* controller);

// associated with a connected controller
struct Controller {
    // was this controller successfully initialized
    uint8_t isInitialized;
    // type of the controller
    BloopairControllerType type;
    // vid and pid
    uint16_t vendor_id;
    uint16_t product_id;
    // bluetooth device address
    uint8_t bda[6];
    // hid handle
    uint8_t handle;
    // is ir enabled
    uint8_t irEnabled;
    // data reporting mode (should be 0x3d for pro controller)
    uint8_t dataReportingMode;
    // is the controller ready to send data
    uint8_t isReady;
    // called when the controller is disconnected
    ControllerDeinitFn deinit;
    // called when hid data is received
    ControllerDataFn data;
    // called to set the player led
    ControllerSetPlayerLedFn setPlayerLed;
    // called when rumble state changes
    ControllerRumbleFn rumble;
    // called every time input is updated
    ControllerUpdateFn update;
    // encryption state
    uint8_t extensionKey[16];
    CryptoState cryptoState;
    // battery level (0 - 4)
    uint8_t battery;
    uint8_t isCharging;
    // report data for continuous reports
    BloopairReportBuffer reportBuffer;
    // controller mapping
    MappingConfiguration* mapping;
    // data that can be allocated for a controller feature
    void* additionalData;
    // Common configuration
    BloopairCommonConfiguration* commonConfig;
    // configuration data pointer for a specific controller type
    void* customConfig;
    // size of custom config for IPC passing
    uint32_t customConfigSize;
};

extern Controller controllers[BTA_HH_MAX_KNOWN];

void initReportThread(void);

void deinitReportThread(void);

int isOfficialName(const char* name);

int isSwitchControllerName(const char* name);

int initController(uint8_t* bda, uint8_t handle);

void sendControllerInput(Controller* controller);

void mapControllerInput(Controller* controller, BloopairReportBuffer* in, BloopairReportBuffer* out);

uint8_t ledMaskToPlayerNum(uint8_t mask);

int16_t scaleStickAxis(uint32_t val, uint32_t range);

int16_t remapStickAxis(int32_t val, int32_t valMin, int32_t valMax);
