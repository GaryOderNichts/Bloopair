/*
 *   Copyright (C) 2022 GaryOderNichts
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
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c>

typedef struct {
    uint8_t player_leds;
    uint8_t led_color[3];
    uint8_t rumble;
    uint8_t output_seq;
} DualsenseData;

enum {
    DUALSENSE_BUTTON_TRIANGLE,
    DUALSENSE_BUTTON_CIRCLE,
    DUALSENSE_BUTTON_CROSS,
    DUALSENSE_BUTTON_SQUARE,

    DUALSENSE_BUTTON_UP,
    DUALSENSE_BUTTON_DOWN,
    DUALSENSE_BUTTON_LEFT,
    DUALSENSE_BUTTON_RIGHT,

    DUALSENSE_BUTTON_R3,
    DUALSENSE_BUTTON_L3,
    DUALSENSE_BUTTON_OPTIONS,
    DUALSENSE_BUTTON_CREATE,

    DUALSENSE_TRIGGER_R2,
    DUALSENSE_TRIGGER_L2,
    DUALSENSE_TRIGGER_R1,
    DUALSENSE_TRIGGER_L1,

    DUALSENSE_STICK_L_UP,
    DUALSENSE_STICK_L_DOWN,
    DUALSENSE_STICK_L_LEFT,
    DUALSENSE_STICK_L_RIGHT,

    DUALSENSE_STICK_R_UP,
    DUALSENSE_STICK_R_DOWN,
    DUALSENSE_STICK_R_LEFT,
    DUALSENSE_STICK_R_RIGHT,
};

#define DUALSENSE_INPUT_REPORT_ID 0x31

typedef struct PACKED {
    uint8_t report_id;
    uint8_t unk;

    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    uint8_t left_trigger;
    uint8_t right_trigger;
    uint8_t seq_number;

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

        uint8_t : 5;
        uint8_t mute : 1;
        uint8_t touchpad : 1;
        uint8_t ps_home : 1;
    } buttons;

    uint8_t unk1[5];

    uint16_t gyro[3];
    uint16_t accel[3];
    uint32_t sensor_timestamp;
    uint8_t unk2;

    uint32_t touch_points[2];
    uint8_t unk3[12];

    uint8_t battery_status : 4;
    uint8_t battery_level : 4;
    uint8_t unk4[19];

    uint32_t crc;
} DualsenseInputReport;
CHECK_SIZE(DualsenseInputReport, 0x4e);

#define DUALSENSE_OUTPUT_REPORT_ID 0x31

#define DUALSENSE_OUTPUT_REPORT_TAG     0x10
#define DUALSENSE_OUTPUT_REPORT_SEED    0xa2

#define DUALSENSE_VF0_COMPATIBLE_VIBRATION              (1 << 0)
#define DUALSENSE_VF0_HAPTICS_SELECT                    (1 << 1)

#define DUALSENSE_VF1_MIC_MUTE_LED_CONTROL_ENABLE       (1 << 0)
#define DUALSENSE_VF1_POWER_SAVE_CONTROL_ENABLE         (1 << 1)
#define DUALSENSE_VF1_LIGHTBAR_CONTROL_ENABLE           (1 << 2)
#define DUALSENSE_VF1_RELEASE_LEDS                      (1 << 3)
#define DUALSENSE_VF1_PLAYER_INDICATOR_CONTROL_ENABLE   (1 << 4)

#define DUALSENSE_VF2_LIGHTBAR_SETUP_CONTROL_ENABLE     (1 << 1)
#define DUALSENSE_VF2_COMPATIBLE_VIBRATION2             (1 << 2)

#define DUALSENSE_LIGHTBAR_SETUP_LIGHT_OUT              (1 << 1)

typedef struct PACKED {
    uint8_t report_id;
    uint8_t seq_tag;
    uint8_t tag;

    uint8_t valid_flag0;
    uint8_t valid_flag1;

    uint8_t motor_right;
    uint8_t motor_left;
    
    uint8_t reserved[4];
    uint8_t mute_button_led;
    
    uint8_t power_save_control;
    uint8_t reserved2[28];

    uint8_t valid_flag2;
    uint8_t reserved3[2];
    uint8_t lightbar_setup;
    uint8_t led_brightness;
    uint8_t player_leds;
    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;

    uint8_t reserved4[24];
    uint32_t crc;
} DualsenseOutputReport;
CHECK_SIZE(DualsenseOutputReport, 0x4e);
