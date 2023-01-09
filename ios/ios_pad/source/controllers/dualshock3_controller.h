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
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-sony.c>
// - <https://github.com/ros-drivers/joystick_drivers/blob/master/ps3joy/scripts/ps3joy_node.py>

typedef struct {
    uint8_t led_update;
    uint8_t led_mask;
    uint8_t rumble;
} Dualshock3Data;

#define DUALSHOCK3_INPUT_REPORT_ID 0x01

typedef struct PACKED {
    uint8_t report_id;
    uint8_t unk;

    struct {
        uint8_t left : 1;
        uint8_t down : 1;
        uint8_t right : 1;
        uint8_t up : 1;
        uint8_t start : 1;
        uint8_t r3 : 1;
        uint8_t l3 : 1;
        uint8_t select : 1;

        uint8_t square : 1;
        uint8_t cross : 1;
        uint8_t circle : 1;
        uint8_t triangle : 1;
        uint8_t r1 : 1;
        uint8_t l1 : 1;
        uint8_t r2 : 1;
        uint8_t l2 : 1;
        
        uint8_t : 7;
        uint8_t ps_home : 1;
    } buttons;
    uint8_t unk1;

    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    uint8_t unk2[4];

    // analog values for the dpad
    uint8_t up;
    uint8_t right;
    uint8_t down;
    uint8_t left;

    // analog values for the triggers
    uint8_t l2;
    uint8_t r2;
    uint8_t l1;
    uint8_t r1;

    // analog values for the face buttons
    uint8_t triangle;
    uint8_t circle;
    uint8_t cross;
    uint8_t square;
    uint8_t unk3[3];

    uint8_t battery_status;
    uint8_t battery_level;
    uint8_t connection_type;
    uint8_t unk4[9];

    uint16_t accel[3];
    uint16_t gyro;
} Dualshock3InputReport;
CHECK_SIZE(Dualshock3InputReport, 0x31);

#define DUALSHOCK3_OUTPUT_REPORT_ID 0x01

typedef struct PACKED {
	uint8_t time_enabled;
	uint8_t duty_length;
	uint8_t enabled;
	uint8_t duty_off;
	uint8_t duty_on;
} Dualshock3LedConfig;
CHECK_SIZE(Dualshock3LedConfig, 0x05);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t reserved;

    uint8_t right_motor_duration;
    uint8_t right_motor_force;
    uint8_t left_motor_duration;
    uint8_t left_motor_force;
    uint8_t reserved2[4];

    uint8_t led_mask;
    Dualshock3LedConfig leds[5];
} Dualshock3OutputReport;
CHECK_SIZE(Dualshock3OutputReport, 0x24);
