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
// - <https://github.com/torvalds/linux/blob/master/drivers/hid/hid-nintendo.c>
// - <https://github.com/ndeadly/MissionControl/blob/master/mc_mitm/source/controllers/switch_controller.hpp>
// - <https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering>
// - <https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_switch.c>

typedef struct {
    int16_t max;
    int16_t center;
    int16_t min;
} SwitchStickCalibration;

typedef struct {
    int16_t max;
    int16_t min;
} SwitchStickExtent;

typedef struct {
    uint8_t first_report;
    uint8_t report_count;
    uint8_t device;
    uint8_t led;

    // Calibration for full reports
    uint8_t has_left_calib;
    SwitchStickCalibration left_calib_x;
    SwitchStickCalibration left_calib_y;
    uint8_t has_right_calib;
    SwitchStickCalibration right_calib_x;
    SwitchStickCalibration right_calib_y;

    // Extents for basic reports
    SwitchStickExtent left_extent_x;
    SwitchStickExtent right_extent_x;
    SwitchStickExtent left_extent_y;
    SwitchStickExtent right_extent_y;
} SwitchData;

enum {
    SWITCH_COMMAND_REQUEST_DEVICE_INFO   = 0x02,
    SWITCH_COMMAND_SET_INPUT_REPORT_MODE = 0x03,
    SWITCH_COMMAND_SPI_FLASH_READ        = 0x10,
    SWITCH_COMMAND_SET_PLAYER_LEDS       = 0x30,
    SWITCH_COMMAND_ENABLE_VIBRATION      = 0x48,
};

// https://switchbrew.org/wiki/Joy-Con_Firmware#Type
enum {
    SWITCH_DEVICE_UNKNOWN,
    SWITCH_DEVICE_JOYCON_LEFT,
    SWITCH_DEVICE_JOYCON_RIGHT,
    SWITCH_DEVICE_PRO,
    SWITCH_DEVICE_TP_JOYCON_LEFT,
    SWITCH_DEVICE_TP_JOYCON_RIGHT,
    SWITCH_DEVICE_TP_PRO,
    SWITCH_DEVICE_FAMICON_LEFT,
    SWITCH_DEVICE_FAMICON_RIGHT,
    SWITCH_DEVICE_NES_LEFT,
    SWITCH_DEVICE_NES_RIGHT,
    SWITCH_DEVICE_SNES,
    SWITCH_DEVICE_N64,
    SWITCH_DEVICE_GENESIS,
};

#define SWITCH_AXIS_X(data) (data[0] | ((data[1] & 0xf) << 8))
#define SWITCH_AXIS_Y(data) ((data[1] >> 4) | (data[2] << 4))
typedef uint8_t SwitchAxis[3];

// https://github.com/ndeadly/MissionControl/blob/c3157607eb1dedf78fc23835d7d0af4fe05a513a/mc_mitm/source/controllers/emulated_switch_controller.cpp#L274-L281
#define SWITCH_FACTORY_CALIBRATION_ADDRESS      0x603d
#define SWITCH_USER_CALIBRATION_ADDRESS         0x8010

#define SWITCH_USER_CALIBRATION_MAGIC           0xb2a1

typedef struct PACKED {
    SwitchAxis max;
    SwitchAxis center;
    SwitchAxis min;
} SwitchRawStickCalibrationLeft;
CHECK_SIZE(SwitchRawStickCalibrationLeft, 0x09);

typedef struct PACKED {
    SwitchAxis center;
    SwitchAxis min;
    SwitchAxis max;
} SwitchRawStickCalibrationRight;
CHECK_SIZE(SwitchRawStickCalibrationRight, 0x09);

typedef struct PACKED {
    uint16_t left_magic;
    SwitchRawStickCalibrationLeft left_calibration;
    uint16_t right_magic;
    SwitchRawStickCalibrationRight right_calibration;
} SwitchRawUserStickCalibration;
CHECK_SIZE(SwitchRawUserStickCalibration, 0x16);

typedef struct PACKED {
    SwitchRawStickCalibrationLeft left_calibration;
    SwitchRawStickCalibrationRight right_calibration;
} SwitchRawFactoryStickCalibration;
CHECK_SIZE(SwitchRawFactoryStickCalibration, 0x12);

typedef struct PACKED {
    uint8_t command;
    union {
        uint8_t data[0x26];

        uint8_t report_mode;

        struct PACKED {
            uint32_t address;
            uint8_t size;
        } spi_flash_read;

        uint8_t leds;

        uint8_t vibration_enabled;
    };
} SwitchCommandRequest;

typedef struct PACKED {
    uint8_t ack;
    uint8_t command;

    union {
        struct PACKED {
            uint16_t fw_version;
            uint8_t device_type;
            uint8_t unk0;
            uint8_t bda[6];
            uint8_t unk1;
            uint8_t unk2;
        } device_info;

        struct PACKED {
            uint32_t address;
            uint8_t size;
            uint8_t data[];
        } spi_flash_read;
    };
} SwitchCommandResponse;

#define SWITCH_INPUT_REPORT_ID 0x30

typedef struct PACKED {
    uint8_t report_id;
    uint8_t timer;

    uint8_t battery_level : 3;
    uint8_t battery_charging : 1;
    uint8_t connection_status : 4;

    struct {
        uint8_t zr : 1;
        uint8_t r : 1;
        uint8_t sl_r : 1;
        uint8_t sr_r : 1;
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;

        uint8_t : 2;
        uint8_t capture : 1;
        uint8_t home : 1;
        uint8_t lstick : 1;
        uint8_t rstick : 1;
        uint8_t plus : 1;
        uint8_t minus : 1;

        uint8_t zl : 1;
        uint8_t l : 1;
        uint8_t sl_l : 1;
        uint8_t sr_l : 1;
        uint8_t left : 1;
        uint8_t right : 1;
        uint8_t up : 1;
        uint8_t down : 1;
    } buttons;

    SwitchAxis left_stick;
    SwitchAxis right_stick;

    uint8_t vibrator;
} SwitchInputReport;
CHECK_SIZE(SwitchInputReport, 0xd);

#define SWITCH_COMMAND_INPUT_REPORT_ID 0x21

typedef struct PACKED {
    SwitchInputReport input;
    SwitchCommandResponse response;
} SwitchCommandInputReport;

#define SWITCH_OUTPUT_REPORT_ID 0x10

typedef struct PACKED {
    uint8_t report_id;
    uint8_t counter;
    uint8_t left_motor[4];
    uint8_t right_motor[4];
} SwitchOutputReport;
CHECK_SIZE(SwitchOutputReport, 0xa);

#define SWITCH_COMMAND_OUTPUT_REPORT_ID 0x01

typedef struct PACKED {
    SwitchOutputReport output;
    SwitchCommandRequest request;
} SwitchCommandOutputReport;

#define SWITCH_BASIC_INPUT_REPORT_ID 0x3f

typedef struct PACKED {
    uint8_t report_id;

    struct {
        uint8_t zr : 1;
        uint8_t zl : 1;
        uint8_t r : 1;
        uint8_t l : 1;
        uint8_t x : 1;
        uint8_t y : 1;
        uint8_t a : 1;
        uint8_t b : 1;

        uint8_t : 2;
        uint8_t capture : 1;
        uint8_t home : 1;
        uint8_t rstick : 1;
        uint8_t lstick : 1;
        uint8_t plus : 1;
        uint8_t minus : 1;

        uint8_t : 4;
        uint8_t dpad : 4;
    } buttons;

    uint16_t left_stick_x;
    uint16_t left_stick_y;
    uint16_t right_stick_x;
    uint16_t right_stick_y;
} SwitchBasicInputReport;
CHECK_SIZE(SwitchBasicInputReport, 0xc);
