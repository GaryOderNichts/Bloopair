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
    DUALSHOCK3_BUTTON_LEFT,
    DUALSHOCK3_BUTTON_DOWN,
    DUALSHOCK3_BUTTON_RIGHT,
    DUALSHOCK3_BUTTON_UP,

    DUALSHOCK3_BUTTON_START,
    DUALSHOCK3_BUTTON_R3,
    DUALSHOCK3_BUTTON_L3,
    DUALSHOCK3_BUTTON_SELECT,

    DUALSHOCK3_BUTTON_SQUARE,
    DUALSHOCK3_BUTTON_CROSS,
    DUALSHOCK3_BUTTON_CIRCLE,
    DUALSHOCK3_BUTTON_TRIANGLE,

    DUALSHOCK3_TRIGGER_R1,
    DUALSHOCK3_TRIGGER_L1,
    DUALSHOCK3_TRIGGER_R2,
    DUALSHOCK3_TRIGGER_L2,
    
    DUALSHOCK3_BUTTON_PS_HOME,
};

typedef struct {
    uint8_t motorForce;
    uint8_t motorDuration;
} Dualshock3Configuration;
