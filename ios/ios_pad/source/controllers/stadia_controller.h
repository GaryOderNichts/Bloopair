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
#pragma once
#include <controllers.h>

#define STADIA_INPUT_REPORT_ID 0x03

typedef struct PACKED {
    uint8_t report_id;

    struct PACKED {
        uint8_t      : 4;
        uint8_t dpad : 4;

        struct {
            uint8_t r3        : 1;
            uint8_t options   : 1;
            uint8_t menu      : 1;
            uint8_t stadia    : 1;
            uint8_t r2        : 1;
            uint8_t l2        : 1;
            uint8_t assistant : 1;
            uint8_t capture   : 1;

            uint8_t    : 1;
            uint8_t a  : 1;
            uint8_t b  : 1;
            uint8_t x  : 1;
            uint8_t y  : 1;
            uint8_t l1 : 1;
            uint8_t r1 : 1;
            uint8_t l3 : 1;
        };
    } buttons;

    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    uint8_t left_bumper;
    uint8_t right_bumper;
    uint8_t unk;
} StadiaInputReport;
CHECK_SIZE(StadiaInputReport, 11);

#define STADIA_OUTPUT_REPORT_ID 0x05

typedef struct PACKED {
    uint8_t report_id;

    uint16_t low_rumble;
    uint16_t high_rumble;
} StadiaOutputReport;
CHECK_SIZE(StadiaOutputReport, 5);
