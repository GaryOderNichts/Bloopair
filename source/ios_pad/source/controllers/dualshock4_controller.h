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

// Information about the reports can be found here:
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-sony.c>
// - <https://github.com/ndeadly/MissionControl/blob/master/mc_mitm/source/controllers/dualshock4_controller.hpp>

typedef struct {
    uint8_t led_color[3];
    uint8_t rumble;
} Dualshock4Data;

#define DUALSHOCK4_INPUT_REPORT_ID 0x11

typedef struct PACKED {
    uint8_t report_id;
    uint8_t unk[2];

    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;

    struct {
        uint8_t triangle : 1;
        uint8_t circle : 1;
        uint8_t cross : 1;
        uint8_t square : 1;
        uint8_t dpad : 4;
        
        uint8_t r3 : 1;
        uint8_t l3 : 1;
        uint8_t options : 1;
        uint8_t create : 1;
        uint8_t r2 : 1;
        uint8_t l2 : 1;
        uint8_t r1 : 1;
        uint8_t l1 : 1;

        uint8_t : 6;
        uint8_t touchpad : 1;
        uint8_t ps_home : 1;
    } buttons;

    uint8_t left_trigger;
    uint8_t right_trigger;

    uint16_t timestamp;
    uint8_t unk1;

    uint16_t gyro[3];
    uint16_t accel[3];
    uint8_t unk2[5];

    uint8_t : 3;
    uint8_t cable : 1;
    uint8_t battery_level : 4;
    uint8_t unk3[2];

    uint8_t num_touches;
    uint8_t touch_data[36];

    uint8_t unk4[2];

    uint32_t crc32;
} Dualshock4InputReport;
CHECK_SIZE(Dualshock4InputReport, 0x4e);

#define DUALSHOCK4_OUTPUT_REPORT_ID 0x11

#define DUALSHOCK4_OUTPUT_REPORT_SEED 0xa2

#define DUALSHOCK4_OUTPUT_FLAG_MOTOR        (1 << 0)
#define DUALSHOCK4_OUTPUT_FLAG_LED_COLOR    (1 << 1)
#define DUALSHOCK4_OUTPUT_FLAG_LED_DELAY    (1 << 2)

typedef struct PACKED {
    uint8_t report_id;

    uint8_t report_mode : 4;
    uint8_t report_rate : 4;
    uint8_t unk;
    uint8_t flags;
    uint8_t unk1[2];

    uint8_t motor_right;
    uint8_t motor_left;

    uint8_t led_red;
    uint8_t led_green;
    uint8_t led_blue;

    uint8_t led_delay_on;
    uint8_t led_delay_off;

    uint8_t unk2[61];

    uint32_t crc;
} Dualshock4OutputReport;
CHECK_SIZE(Dualshock4OutputReport, 0x4e);
