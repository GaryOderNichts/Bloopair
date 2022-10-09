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
#include "crypto.h"

//! Pro Controller buttons.
enum WPADProButton {
   //! The up button of the D-pad.
   WPAD_PRO_BUTTON_UP                  = 0x0001,
   //! The left button of the D-pad.
   WPAD_PRO_BUTTON_LEFT                = 0x0002,
   //! The ZR button.
   WPAD_PRO_TRIGGER_ZR                 = 0x0004,
   //! The X button.
   WPAD_PRO_BUTTON_X                   = 0x0008,
   //! The A button.
   WPAD_PRO_BUTTON_A                   = 0x0010,
   //! The Y button.
   WPAD_PRO_BUTTON_Y                   = 0x0020,
   //! The B button.
   WPAD_PRO_BUTTON_B                   = 0x0040,
   //! The ZL button.
   WPAD_PRO_TRIGGER_ZL                 = 0x0080,
   //! Reserved.
   WPAD_PRO_RESERVED                   = 0x0100,
   //! The right trigger button.
   WPAD_PRO_TRIGGER_R                  = 0x0200,
   //! The + button.
   WPAD_PRO_BUTTON_PLUS                = 0x0400,
   //! The HOME button.
   WPAD_PRO_BUTTON_HOME                = 0x0800,
   //! The - button.
   WPAD_PRO_BUTTON_MINUS               = 0x1000,
   //! The left trigger button.
   WPAD_PRO_TRIGGER_L                  = 0x2000,
   //! The down button of the D-pad.
   WPAD_PRO_BUTTON_DOWN                = 0x4000,
   //! The right button of the D-pad.
   WPAD_PRO_BUTTON_RIGHT               = 0x8000,
   //! The right stick button.
   WPAD_PRO_BUTTON_STICK_R             = 0x10000,
   //! The left stick button.
   WPAD_PRO_BUTTON_STICK_L             = 0x20000,
};

typedef struct Controller_t Controller_t;

typedef void (*ControllerDeinitFn)(Controller_t* controller);
typedef void (*ControllerDataFn)(Controller_t* controller, uint8_t* data, uint16_t len);
typedef void (*ControllerSetPlayerLedFn)(Controller_t* controller, uint8_t led);
typedef void (*ControllerRumbleFn)(Controller_t* controller, uint8_t rumble);
typedef void (*ControllerUpdateFn)(Controller_t* controller);

// report data for continuous reports
typedef struct {
    uint32_t buttons;
    int16_t left_stick_x;
    int16_t right_stick_x;
    int16_t left_stick_y;
    int16_t right_stick_y;
} ReportBuffer_t;

// associated with a connected controller
struct Controller_t {
    // is this an officially supported controller
    uint8_t isOfficialController;
    // was this controller successfully initialized
    uint8_t isInitialized;
    // hid handle
    uint8_t handle;
    // hid index
    uint8_t index;
    // is ir enabled
    uint8_t irEnabled;
    // data reporting mode (should be 0x3d for pro controller)
    uint8_t dataReportingMode;
    // called when the controller is disconnected
    ControllerDeinitFn deinit;
    // called when hid data is received
    ControllerDataFn data;
    // called to set the player led
    ControllerSetPlayerLedFn setPlayerLed;
    // called when rumble state changes
    ControllerRumbleFn rumble;
    // called every time input is updated (requires continous reports)
    ControllerUpdateFn update;
    // encryption state
    uint8_t key[16];
    CryptoState crypto;
    // battery level (0 - 4)
    uint8_t battery;
    uint8_t isCharging;
    // report data for continuous reports
    ReportBuffer_t* reportData;
    // data that can be allocated for a controller feature
    void* additionalData;
};

extern Controller_t controllers[BTA_HH_MAX_KNOWN];

void initReportThread(void);

void deinitReportThread(void);

int isOfficialName(const char* name);

int isSwitchControllerName(const char* name);

int initController(uint8_t* bda, uint8_t handle);

void sendControllerInput(Controller_t* controller, uint32_t buttons, int16_t left_stick_x, int16_t right_stick_x, int16_t left_stick_y, int16_t right_stick_y);

void initContinuousReports(Controller_t* controller);

void deinitContinuousReports(Controller_t* controller);

uint8_t ledMaskToPlayerNum(uint8_t mask);
