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

// Information about the reports can be found here:
// - <https://github.com/ndeadly/switch2_controller_research/blob/master/hid_reports.md>
// - <https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md>
// - <https://github.com/ndeadly/switch2_controller_research/blob/master/bluetooth_interface.md>
// - <https://gist.github.com/ndeadly/7d27aa63e2f653a902a2474dbcbc08b3>
// -
// <https://github.com/libsdl-org/SDL/blob/56a72f8c869fd5ebd69c1bd4ba7abcdf0b7b83b4/src/joystick/hidapi/SDL_hidapi_switch2.c>

#define SWITCH2_OUTPUT_REPORT_CMD_HANDLE 0x0014

#define SWITCH2_DIRECTION_REQUEST  0x91
#define SWITCH2_DIRECTION_RESPONSE 0x01

#define SWITCH2_TRANSPORT_USB 0x00
#define SWITCH2_TRANSPORT_BLE 0x01

enum {
    SWITCH2_COMMAND_NFC             = 0x01,
    SWITCH2_COMMAND_FLASH           = 0x02,
    SWITCH2_COMMAND_INIT            = 0x03,
    SWITCH2_COMMAND_CHARGING_GRIP   = 0x08,
    SWITCH2_COMMAND_LEDS            = 0x09,
    SWITCH2_COMMAND_VIBRATION       = 0x0A,
    SWITCH2_COMMAND_BATTERY         = 0x0B,
    SWITCH2_COMMAND_FEATURE         = 0x0C,
    SWITCH2_COMMAND_FIRMWARE_UPDATE = 0x0D,
    SWITCH2_COMMAND_FIRMWARE_INFO   = 0x10,
    SWITCH2_COMMAND_PAIRING         = 0x15,
};

enum {
    // Flash
    SWITCH2_SUBCOMMAND_FLASH_READ_BLOCK     = 0x01,
    SWITCH2_SUBCOMMAND_FLASH_WRITE_BLOCK    = 0x02,
    SWITCH2_SUBCOMMAND_FLASH_ERASE_BLOCK    = 0x03,
    SWITCH2_SUBCOMMAND_FLASH_READ           = 0x04,
    SWITCH2_SUBCOMMAND_FLASH_WRITE          = 0x05,

    // Player LEDs
    SWITCH2_SUBCOMMAND_LEDS_SET_PLAYER_1    = 0x01,
    SWITCH2_SUBCOMMAND_LEDS_SET_PLAYER_2    = 0x02,
    SWITCH2_SUBCOMMAND_LEDS_SET_PLAYER_3    = 0x03,
    SWITCH2_SUBCOMMAND_LEDS_SET_PLAYER_4    = 0x04,
    SWITCH2_SUBCOMMAND_LEDS_SET_ALL_ON      = 0x05,
    SWITCH2_SUBCOMMAND_LEDS_SET_ALL_OFF     = 0x06,
    SWITCH2_SUBCOMMAND_LEDS_SET_PATTERN     = 0x07,
    SWITCH2_SUBCOMMAND_LEDS_FLASH           = 0x08,

    // Feature
    SWITCH2_SUBCOMMAND_FEATURE_GET_INFO     = 0x01,
    SWITCH2_SUBCOMMAND_FEATURE_SET_MASK     = 0x02,
    SWITCH2_SUBCOMMAND_FEATURE_CLEAR_MASK   = 0x03,
    SWITCH2_SUBCOMMAND_FEATURE_ENABLE       = 0x04,
    SWITCH2_SUBCOMMAND_FEATURE_DISABLE      = 0x05,
    SWITCH2_SUBCOMMAND_FEATURE_CONFIGURE    = 0x06,

    // Pairing
    SWITCH2_SUBCOMMAND_PAIRING_EXCHANGE_ADDR    = 0x01,
    SWITCH2_SUBCOMMAND_PAIRING_CONFIRM_LTK      = 0x02,
    SWITCH2_SUBCOMMAND_PAIRING_FINALIZE         = 0x03,
    SWITCH2_SUBCOMMAND_PAIRING_EXCHANGE_KEYS    = 0x04,
};

typedef struct PACKED {
    uint8_t command;
    uint8_t direction;
    uint8_t transport;
    uint8_t subcommand;
    uint8_t unknown;
    uint8_t length;
    uint16_t padding;
} Switch2CommandHeader;
CHECK_SIZE(Switch2CommandHeader, 0x8);

typedef struct PACKED {
    uint8_t length;
    uint8_t unknown;
    uint16_t padding;
    uint32_t address;
} Switch2FlashReadRequest;
CHECK_SIZE(Switch2FlashReadRequest, 0x08);

typedef struct PACKED {
    uint8_t pattern;
    uint8_t unknown[7];
} Switch2LEDSetPatternRequest;
CHECK_SIZE(Switch2LEDSetPatternRequest, 0x8);

typedef struct PACKED {
    uint8_t flags;
    uint8_t unknown[3];
} Switch2FeatureFlagsRequest;
CHECK_SIZE(Switch2FeatureFlagsRequest, 0x4);

typedef struct PACKED {
    uint8_t unknown;
    uint8_t count;
    BD_ADDR addresses[0];
} Switch2PairingAddressExchangeRequest;
CHECK_SIZE(Switch2PairingAddressExchangeRequest, 0x02);

typedef struct PACKED {
    uint8_t unknown;
    uint8_t challenge[0x10];
} Switch2PairingConfirmLTKRequest;
CHECK_SIZE(Switch2PairingConfirmLTKRequest, 0x11);

typedef struct PACKED {
    uint8_t unknown;
    uint8_t key[0x10];
} Switch2PairingKeysExchangeRequest;
CHECK_SIZE(Switch2PairingKeysExchangeRequest, 0x11);

typedef struct PACKED {
    Switch2CommandHeader header;

    union {
        union {
            Switch2FlashReadRequest read_block, read;
        } flash;
        union {
            Switch2LEDSetPatternRequest pattern;
        } leds;
        union {
            Switch2FeatureFlagsRequest info, set_mask, clear, enable, disable, configure;
        } feature;
        union {
            Switch2PairingAddressExchangeRequest address_exchange;
            Switch2PairingConfirmLTKRequest ltk_confirm;
            uint8_t finalize;
            Switch2PairingKeysExchangeRequest keys_exchange;
        } pairing;
    };
} Switch2CommandRequest;

typedef struct PACKED {
    uint8_t length;
    uint8_t unknown;
    uint16_t padding;
    uint32_t address;
    uint8_t data[0];
} Switch2FlashReadResponse;

typedef struct PACKED {
    uint8_t unknown0x00;
    uint8_t unknown0x01;
    uint8_t count;
    BD_ADDR addresses[0];
} Switch2PairingAddressExchangeResponse;
CHECK_SIZE(Switch2PairingAddressExchangeResponse, 0x03);

typedef struct PACKED {
    uint8_t unknown;
    uint8_t response[0x10];
} Switch2PairingConfirmLTKResponse;
CHECK_SIZE(Switch2PairingConfirmLTKResponse, 0x11);

typedef struct PACKED {
    uint8_t unknown;
    uint8_t key[0x10];
} Switch2PairingKeysExchangeResponse;
CHECK_SIZE(Switch2PairingKeysExchangeResponse, 0x11);

#define SWITCH2_INPUT_REPORT_CMD_RESP_HANDLE 0x001a
#define SWITCH2_INPUT_REPORT_CMD_RESP_CONF   0x001b

typedef struct PACKED {
    Switch2CommandHeader header;

    union {
        union {
            Switch2FlashReadResponse read_block;
            Switch2FlashReadResponse read;
        } flash;
        union {
            Switch2PairingAddressExchangeResponse address_exchange;
            Switch2PairingConfirmLTKResponse ltk_confirm;
            uint8_t finalize;
            Switch2PairingKeysExchangeResponse keys_exchange;
        } pairing;
    };
} Switch2CommandResponse;

#define SWITCH2_AXIS_X(data) (data[0] | ((data[1] & 0xf) << 8))
#define SWITCH2_AXIS_Y(data) ((data[1] >> 4) | (data[2] << 4))
typedef uint8_t Switch2Axis[3];

// Stick calibration data, as stored in flash
typedef struct PACKED {
    Switch2Axis neutral;
    Switch2Axis max;
    Switch2Axis min;
} Switch2RawStickCalibration;
CHECK_SIZE(Switch2RawStickCalibration, 0x09);

typedef struct PACKED {
    uint8_t zr   : 1;
    uint8_t r    : 1;
    uint8_t sl_r : 1;
    uint8_t sr_r : 1;
    uint8_t a    : 1;
    uint8_t b    : 1;
    uint8_t x    : 1;
    uint8_t y    : 1;

    uint8_t         : 1;
    uint8_t c       : 1;
    uint8_t capture : 1;
    uint8_t home    : 1;
    uint8_t lstick  : 1;
    uint8_t rstick  : 1;
    uint8_t plus    : 1;
    uint8_t minus   : 1;

    uint8_t zl    : 1;
    uint8_t l     : 1;
    uint8_t sl_l  : 1;
    uint8_t sr_l  : 1;
    uint8_t left  : 1;
    uint8_t right : 1;
    uint8_t up    : 1;
    uint8_t down  : 1;

    uint8_t         : 3;
    uint8_t headset : 1;
    uint8_t         : 2;
    uint8_t gl      : 1;
    uint8_t gr      : 1;
} Switch2Buttons;
CHECK_SIZE(Switch2Buttons, 0x04);

#define SWITCH2_INPUT_REPORT_0x05_HANDLE 0x000a
#define SWITCH2_INPUT_REPORT_0x05_CONF   0x000b

typedef struct PACKED {
    uint32_t counter;
    Switch2Buttons buttons;
    uint16_t unknown0x08;
    Switch2Axis left_stick;
    Switch2Axis right_stick;
    uint8_t mouse[0x8];
    uint8_t unknown0x18;
    uint8_t magnetometer[0x6];
    uint16_t battery_voltage;
    uint8_t charging_state;
    uint16_t battery_current;
    uint8_t unknown0x24[0x6];
    uint8_t motion[0x12];
    uint8_t left_trigger;
    uint8_t right_trigger;
    uint8_t reserved;
} Switch2InputReport0x05;
CHECK_SIZE(Switch2InputReport0x05, 0x3F);

typedef struct PACKED {
    uint8_t vibration_counter;
    uint8_t rumble_data[5];
    uint8_t unknown[10];
} Switch2RumbleData;
CHECK_SIZE(Switch2RumbleData, 0x10);

#define SWITCH2_OUTPUT_REPORT_RUMBLE_HANDLE 0x0012

typedef struct PACKED {
    uint8_t report_id;
    union {
        struct {
            Switch2RumbleData rumble;
            uint8_t reserved[0x19];
        } joycon;
        struct {
            Switch2RumbleData left_rumble;
            Switch2RumbleData right_rumble;
            uint8_t reserved[0x9];
        } pro;
        struct {
            uint8_t vibration_counter;
            uint8_t rumble[0x3];
            uint8_t reserved[0x25];
        } gc;
    };
} Switch2OutputReportRumble;
CHECK_SIZE(Switch2OutputReportRumble, 0x2a);
