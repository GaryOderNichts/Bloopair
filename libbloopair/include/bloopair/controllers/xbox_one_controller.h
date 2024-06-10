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
    XBOX_ONE_BUTTON_UP,
    XBOX_ONE_BUTTON_DOWN,
    XBOX_ONE_BUTTON_LEFT,
    XBOX_ONE_BUTTON_RIGHT,

    XBOX_ONE_TRIGGER_RB,
    XBOX_ONE_TRIGGER_LB,

    XBOX_ONE_BUTTON_Y,
    XBOX_ONE_BUTTON_X,
    XBOX_ONE_BUTTON_B,
    XBOX_ONE_BUTTON_A,

    XBOX_ONE_BUTTON_RSTICK,
    XBOX_ONE_BUTTON_LSTICK,
    XBOX_ONE_BUTTON_XBOX,
    XBOX_ONE_BUTTON_MENU,
    XBOX_ONE_BUTTON_VIEW,

    // technically not button bits, but mapped to those from analog triggers
    XBOX_ONE_TRIGGER_R,
    XBOX_ONE_TRIGGER_L,
};

typedef struct {

} XboxOneConfiguration;
