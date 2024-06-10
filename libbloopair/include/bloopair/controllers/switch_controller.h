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

#include "common.h"

enum {
    SWITCH_TRIGGER_ZR,
    SWITCH_TRIGGER_R,
    SWITCH_TRIGGER_SL_R,
    SWITCH_TRIGGER_SR_R,
    SWITCH_BUTTON_A,
    SWITCH_BUTTON_B,
    SWITCH_BUTTON_X,
    SWITCH_BUTTON_Y,

    SWITCH_BUTTON_CAPTURE,
    SWITCH_BUTTON_HOME,
    SWITCH_BUTTON_STICK_L,
    SWITCH_BUTTON_STICK_R,
    SWITCH_BUTTON_PLUS,
    SWITCH_BUTTON_MINUS,

    SWITCH_TRIGGER_ZL,
    SWITCH_TRIGGER_L,
    SWITCH_TRIGGER_SL_L,
    SWITCH_TRIGGER_SR_L,
    SWITCH_BUTTON_LEFT,
    SWITCH_BUTTON_RIGHT,
    SWITCH_BUTTON_UP,
    SWITCH_BUTTON_DOWN,

    SWITCH_N64_C_LEFT   = SWITCH_BUTTON_X,
    SWITCH_N64_C_RIGHT  = SWITCH_BUTTON_MINUS,
    SWITCH_N64_C_UP     = SWITCH_TRIGGER_ZR,
    SWITCH_N64_C_DOWN   = SWITCH_BUTTON_Y,
};

typedef struct {
    uint8_t disableCalibration;
} SwitchConfiguration;
