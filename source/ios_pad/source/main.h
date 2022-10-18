/*
 *   Copyright (C) 2021 GaryOderNichts
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

#include "imports.h"
#include "utils.h"

// Info about the reports can be found here:
// - <https://wiibrew.org/wiki/Wiimote>

enum {
    WM_REPORT_ID_RUMBLE         = 0x10,
    WM_REPORT_ID_LED            = 0x11,
    WM_REPORT_ID_REPORT_MODE    = 0x12,
    WM_REPORT_ID_IR_ENABLE_1    = 0x13,
    WM_REPORT_ID_SPEAKER_ENABLE = 0x14,
    WM_REPORT_ID_REQUEST_STATUS = 0x15,
    WM_REPORT_ID_MEMORY_WRITE   = 0x16,
    WM_REPORT_ID_MEMORY_READ    = 0x17,
    WM_REPORT_ID_SPEAKER_DATA   = 0x18,
    WM_REPORT_ID_SPEAKER_MUTE   = 0x19,
    WM_REPORT_ID_IR_ENABLE_2    = 0x1a,

    WM_REPORT_ID_STATUS         = 0x20,
    WM_REPORT_ID_MEMORY_DATA    = 0x21,
    WM_REPORT_ID_ACKNOWLEDGE    = 0x22,

    WM_REPORT_ID_EXTENSION_DATA_REPORT = 0x3d,
};

typedef struct PACKED {
    uint8_t report_id;
    uint8_t : 7;
    uint8_t rumble : 1;
} WMRumbleReport;
CHECK_SIZE(WMRumbleReport, 2);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t led_mask : 4;
    uint8_t : 4;
} WMLEDReport;
CHECK_SIZE(WMLEDReport, 2);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t continous;
    uint8_t mode;
} WMReportModeReport;
CHECK_SIZE(WMReportModeReport, 3);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t ir_enable;
} WMIREnableReport;
CHECK_SIZE(WMIREnableReport, 2);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t speaker_enable;
} WMSpeakerEnableReport;
CHECK_SIZE(WMSpeakerEnableReport, 2);

typedef struct PACKED {
    uint8_t report_id;
    uint32_t address;
    uint8_t size;
    uint8_t data[16];
} WMMemoryWriteReport;
CHECK_SIZE(WMMemoryWriteReport, 22);

typedef struct PACKED {
    uint8_t report_id;
    uint32_t address;
    uint16_t size;
} WMMemoryReadReport;
CHECK_SIZE(WMMemoryReadReport, 7);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t size;
    uint8_t data[20];
} WMSpeakerDataReport;
CHECK_SIZE(WMSpeakerDataReport, 22);

typedef struct PACKED {
    uint8_t report_id;
    uint8_t mute;
} WMSpeakerMuteReport;
CHECK_SIZE(WMSpeakerMuteReport, 2);

typedef struct PACKED {
    uint8_t report_id;
    uint16_t core_buttons;
    uint8_t led_mask : 4;
    uint8_t flags : 4;
    uint16_t unused;
    uint8_t battery_level;
} WMStatusReport;
CHECK_SIZE(WMStatusReport, 7);

typedef struct PACKED {
    uint8_t report_id;
    uint16_t core_buttons;
    uint8_t size : 4;
    uint8_t result : 4;
    uint16_t offset;
    uint8_t data[16];
} WMMemoryDataReport;
CHECK_SIZE(WMMemoryDataReport, 22);

typedef struct PACKED {
    uint8_t report_id;
    uint16_t core_buttons;
    uint8_t ack_report;
    uint8_t result;
} WMAcknowledgeReport;
CHECK_SIZE(WMAcknowledgeReport, 5);

typedef union {
    uint8_t report_id;
    WMRumbleReport rumble;
    WMLEDReport led;
    WMReportModeReport report_mode;
    WMIREnableReport ir_enable;
    WMSpeakerEnableReport speaker_enable;
    WMMemoryWriteReport memory_write;
    WMMemoryReadReport memory_read;
    WMSpeakerDataReport speaker_data;
    WMSpeakerMuteReport speaker_mute;
    uint8_t data[0x78];
} WMReport;
CHECK_SIZE(WMReport, 0x78);

typedef struct PACKED {
    uint8_t dev_handle;
    uint8_t mode;
    uint8_t sub_class;
    uint8_t app_id;
    uint16_t length;
    uint8_t data[58];
} SMDInputMessage;
CHECK_SIZE(SMDInputMessage, 0x40);

typedef struct PACKED {
    uint8_t smd_header[8];
    uint16_t __unk1;
    uint16_t length;
    uint16_t __unk2;
    uint8_t dev_handle;
    uint8_t __unk3;
    WMReport report;
} SMDOutputMessage;
CHECK_SIZE(SMDOutputMessage, 0x88);

void sendInputData(uint8_t dev_handle, const void* data, uint16_t len);

void sendOutputData(uint8_t dev_handle, const void* data, uint16_t len);

void setReport(uint8_t dev_handle, uint8_t type, const void* data, uint16_t len);
