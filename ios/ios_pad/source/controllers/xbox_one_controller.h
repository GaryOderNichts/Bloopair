/*
 *   Copyright (C) 2023 GaryOderNichts
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

#include <controllers.h>

// Info about the reports can be found here:
// - <https://github.com/atar-axis/xpadneo/blob/master/hid-xpadneo/src/hid-xpadneo.c>
// - <https://github.com/atar-axis/xpadneo/blob/master/hid-xpadneo/src/xpadneo.h>

enum {
    XBOX_ONE_RUMBLE_WEAK          = 1 << 0,
    XBOX_ONE_RUMBLE_STRONG        = 1 << 1,
    XBOX_ONE_RUMBLE_RIGHT         = 1 << 2,
    XBOX_ONE_RUMBLE_LEFT          = 1 << 3,

    XBOX_ONE_RUMBLE_MAIN          = XBOX_ONE_RUMBLE_WEAK | XBOX_ONE_RUMBLE_STRONG,
    XBOX_ONE_RUMBLE_TRIGGERS      = XBOX_ONE_RUMBLE_RIGHT | XBOX_ONE_RUMBLE_LEFT,
    XBOX_ONE_RUMBLE_ALL           = XBOX_ONE_RUMBLE_MAIN | XBOX_ONE_RUMBLE_TRIGGERS,
};

#define XBOX_ONE_INPUT_REPORT_ID 0x01

typedef struct PACKED {
    uint8_t report_id;

    uint16_t left_stick_x;
    uint16_t left_stick_y;
    uint16_t right_stick_x;
    uint16_t right_stick_y;

    uint16_t left_trigger;
    uint16_t right_trigger;

    struct PACKED {
        uint8_t : 4;
        uint8_t dpad : 4;

        union {
            struct {
                uint8_t rb : 1;
                uint8_t lb : 1;
                uint8_t : 1;
                uint8_t y : 1;
                uint8_t x : 1;
                uint8_t : 1;
                uint8_t b : 1;
                uint8_t a : 1;

                uint8_t : 1;
                uint8_t rstick : 1;
                uint8_t lstick : 1;
                uint8_t xbox : 1;
                uint8_t menu : 1;
                uint8_t : 3;

                uint8_t : 7;
                uint8_t view : 1;
            };
            struct {
                uint8_t menu : 1;
                uint8_t view : 1;
                uint8_t rb : 1;
                uint8_t lb : 1;
                uint8_t y : 1;
                uint8_t x : 1;
                uint8_t b : 1;
                uint8_t a : 1;

                uint8_t : 6;
                uint8_t rstick : 1;
                uint8_t lstick : 1;
            } old;
        };
    } buttons;
} XboxOneInputReport;

#define XBOX_ONE_XB_BUTTON_INPUT_REPORT_ID 0x02

typedef struct PACKED {
    uint8_t report_id;
    uint8_t : 7;
    uint8_t xbox_button : 1;
} XboxOneXbButtonInputReport;
CHECK_SIZE(XboxOneXbButtonInputReport, 0x2);

#define XBOX_ONE_OUTPUT_REPORT_ID 0x03

typedef struct PACKED {
    uint8_t report_id;
    uint8_t motors_enable;
	uint8_t magnitude_left;
	uint8_t magnitude_right;
	uint8_t magnitude_strong;
	uint8_t magnitude_weak;
	uint8_t pulse_sustain_10ms;
	uint8_t pulse_release_10ms;
	uint8_t loop_count;
} XboxOneOutputReport;
CHECK_SIZE(XboxOneOutputReport, 0x9);

#define XBOX_ONE_BATTERY_INPUT_REPORT_ID 0x04

typedef struct PACKED {
    uint8_t report_id;
    uint8_t online : 1;
    uint8_t : 2;
    uint8_t charging : 1;
    uint8_t mode : 2;
    uint8_t capacity : 2;
} XboxOneBatteryInputReport;
CHECK_SIZE(XboxOneBatteryInputReport, 0x2);
