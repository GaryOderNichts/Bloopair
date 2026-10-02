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

#include "bt_api.h"

// Table 3.2: SMP Command Codes
enum {
    SMP_CMD_RESERVED                      = 0x00,
    SMP_CMD_PAIRING_REQUEST               = 0x01,
    SMP_CMD_PAIRING_RESPONSE              = 0x02,
    SMP_CMD_PAIRING_CONFIRM               = 0x03,
    SMP_CMD_PAIRING_RANDOM                = 0x04,
    SMP_CMD_PAIRING_FAILED                = 0x05,
    SMP_CMD_ENCRYPTION_INFORMATION        = 0x06,
    SMP_CMD_MASTER_IDENTIFICATION         = 0x07,
    SMP_CMD_IDENTITY_INFORMATION          = 0x08,
    SMP_CMD_IDENTITY_ADDRESS_INFORMATION  = 0x09,
    SMP_CMD_SIGNING_INFORMATION           = 0x0A,
    SMP_CMD_SECURITY_REQUEST              = 0x0B,
};

// Table 3.3: IO Capability Values
enum {
    SMP_IO_CAP_DISPLAYONLY     = 0x00,
    SMP_IO_CAP_DISPLAYYESNO    = 0x01,
    SMP_IO_CAP_KEYBOARDONLY    = 0x02,
    SMP_IO_CAP_NOINPUTNOOUTPUT = 0x03,
    SMP_IO_CAP_KEYBOARDDISPLAY = 0x04,
};

// Table 3.4: OOB Data Present Values
enum {
    SMP_OOB_DATA_NONE    = 0x00,
    SMP_OOB_DATA_PRESENT = 0x01,
};

// Table 3.5: Bonding Flags
enum {
    SMP_BONDING_FLAG_NO_BONDING = 0b00,
    SMP_BONDING_FLAG_BONDING    = 0b01,
};

// Figure 3.8: LE Key Distribution Format
enum {
    SMP_KEY_DIST_ENC_KEY = (1u << 0),
    SMP_KEY_DIST_ID_KEY  = (1u << 1),
    SMP_KEY_DIST_SIGN    = (1u << 2),
};

typedef enum {
    SMP_EVENT_ENCRYPT,
    SMP_EVENT_BOND,
    SMP_EVENT_FAILURE,
} SMPEvent;

typedef struct {
    uint8_t ltk[16];
    uint8_t rand[8];
    uint16_t ediv;
} SMPBondingData;

typedef void (*SMPCallback)(BD_ADDR addr, SMPEvent event, SMPBondingData* bondingData, void* userPointer);

void SMP_Init(void);

int SMP_StartPairing(BD_ADDR addr, SMPCallback callback, void* userPointer);

int SMP_StartEncrypt(BD_ADDR addr, SMPCallback callback, SMPBondingData* bondingData, void* userPointer);

void SMP_HandleEncryptChange(BD_ADDR addr, uint8_t status, uint8_t encr_enable);
