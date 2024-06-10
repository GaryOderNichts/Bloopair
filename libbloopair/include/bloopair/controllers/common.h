/*
 *   Copyright (C) 2024 GaryOderNichts
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

#include <stdint.h>

#define BTN(x) (1 << (x))

typedef enum {
    BLOOPAIR_CONTROLLER_INVALID,
    BLOOPAIR_CONTROLLER_OFFICIAL,

    BLOOPAIR_CONTROLLER_DUALSENSE            = 0x10,
    BLOOPAIR_CONTROLLER_DUALSHOCK3,
    BLOOPAIR_CONTROLLER_DUALSHOCK4,

    BLOOPAIR_CONTROLLER_SWITCH_GENERIC       = 0x20,
    BLOOPAIR_CONTROLLER_SWITCH_JOYCON_LEFT,
    BLOOPAIR_CONTROLLER_SWITCH_JOYCON_RIGHT,
    BLOOPAIR_CONTROLLER_SWITCH_JOYCON_DUAL,
    BLOOPAIR_CONTROLLER_SWITCH_PRO,
    BLOOPAIR_CONTROLLER_SWITCH_N64,

    BLOOPAIR_CONTROLLER_XBOX_ONE             = 0x30,
} BloopairControllerType;

//! Bloopair Pro Controller buttons.
enum BloopairProButton {
    //! The up button of the D-pad.
    BLOOPAIR_PRO_BUTTON_UP,
    //! The left button of the D-pad.
    BLOOPAIR_PRO_BUTTON_LEFT,
    //! The ZR button.
    BLOOPAIR_PRO_TRIGGER_ZR,
    //! The X button.
    BLOOPAIR_PRO_BUTTON_X,
    //! The A button.
    BLOOPAIR_PRO_BUTTON_A,
    //! The Y button.
    BLOOPAIR_PRO_BUTTON_Y,
    //! The B button.
    BLOOPAIR_PRO_BUTTON_B,
    //! The ZL button.
    BLOOPAIR_PRO_TRIGGER_ZL,
    //! Reserved.
    BLOOPAIR_PRO_RESERVED,
    //! The right trigger button.
    BLOOPAIR_PRO_TRIGGER_R,
    //! The + button.
    BLOOPAIR_PRO_BUTTON_PLUS,
    //! The HOME button.
    BLOOPAIR_PRO_BUTTON_HOME,
    //! The - button.
    BLOOPAIR_PRO_BUTTON_MINUS,
    //! The left trigger button.
    BLOOPAIR_PRO_TRIGGER_L,
    //! The down button of the D-pad.
    BLOOPAIR_PRO_BUTTON_DOWN,
    //! The right button of the D-pad.
    BLOOPAIR_PRO_BUTTON_RIGHT,
    //! The right stick button.
    BLOOPAIR_PRO_BUTTON_STICK_R,
    //! The left stick button.
    BLOOPAIR_PRO_BUTTON_STICK_L,

    //! These aren't part of the button bitfield and only exist for mapping analog sticks
    BLOOPAIR_PRO_STICK_MIN = 32,
    BLOOPAIR_PRO_STICK_L_UP = BLOOPAIR_PRO_STICK_MIN,
    BLOOPAIR_PRO_STICK_L_DOWN,
    BLOOPAIR_PRO_STICK_L_LEFT,
    BLOOPAIR_PRO_STICK_L_RIGHT,
    BLOOPAIR_PRO_STICK_R_UP,
    BLOOPAIR_PRO_STICK_R_DOWN,
    BLOOPAIR_PRO_STICK_R_LEFT,
    BLOOPAIR_PRO_STICK_R_RIGHT,

    BLOOPAIR_PRO_BUTTON_MAX,
};

typedef struct {
    uint16_t stickAsButtonDeadzone;
} BloopairCommonConfiguration;

typedef struct {
    //! One of the controller specific buttons or `BLOOPAIR_PRO_STICK`s
    uint8_t from;
    //! One of the `BloopairProButton`s
    uint8_t to;
} BloopairMappingEntry;

typedef struct {
    uint32_t buttons;
    int16_t left_stick_x;
    int16_t right_stick_x;
    int16_t left_stick_y;
    int16_t right_stick_y;
} BloopairReportBuffer;
