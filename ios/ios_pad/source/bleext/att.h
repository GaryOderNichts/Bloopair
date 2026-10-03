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

// Table 3.37: Attribute Protocol Summary
enum {
    ATT_ERROR_RSP                  = 0x01,
    ATT_EXCHANGE_MTU_REQ           = 0x02,
    ATT_EXCHANGE_MTU_RSP           = 0x03,
    ATT_FIND_INFORMATION_REQ       = 0x04,
    ATT_FIND_INFORMATION_RSP       = 0x05,
    ATT_FIND_BY_TYPE_VALUE_REQ     = 0x06,
    ATT_FIND_BY_TYPE_VALUE_RSP     = 0x07,
    ATT_READ_BY_TYPE_REQ           = 0x08,
    ATT_READ_BY_TYPE_RSP           = 0x09,
    ATT_READ_REQ                   = 0x0A,
    ATT_READ_RSP                   = 0x0B,
    ATT_READ_BLOB_REQ              = 0x0C,
    ATT_READ_BLOB_RSP              = 0x0D,
    ATT_READ_MULTIPLE_REQ          = 0x0E,
    ATT_READ_MULTIPLE_RSP          = 0x0F,
    ATT_READ_BY_GROUP_TYPE_REQ     = 0x10,
    ATT_READ_BY_GROUP_TYPE_RSP     = 0x11,
    ATT_WRITE_REQ                  = 0x12,
    ATT_WRITE_RSP                  = 0x13,
    ATT_WRITE_CMD                  = 0x52,
    ATT_PREPARE_WRITE_REQ          = 0x16,
    ATT_PREPARE_WRITE_RSP          = 0x17,
    ATT_EXECUTE_WRITE_REQ          = 0x18,
    ATT_EXECUTE_WRITE_RSP          = 0x19,
    ATT_READ_MULTIPLE_VARIABLE_REQ = 0x20,
    ATT_READ_MULTIPLE_VARIABLE_RSP = 0x21,
    ATT_MULTIPLE_HANDLE_VALUE_NTF  = 0x23,
    ATT_HANDLE_VALUE_NTF           = 0x1B,
    ATT_HANDLE_VALUE_IND           = 0x1D,
    ATT_HANDLE_VALUE_CFM           = 0x1E,
    ATT_SIGNED_WRITE_CMD           = 0xD2,
};

// Table 3.3: Error Codes
enum {
    ATT_ERROR_INVALID_HANDLE                 = 0x01,
    ATT_ERROR_READ_NOT_PERMITTED             = 0x02,
    ATT_ERROR_WRITE_NOT_PERMITTED            = 0x03,
    ATT_ERROR_INVALID_PDU                    = 0x04,
    ATT_ERROR_INSUFFICIENT_AUTHENTICATION    = 0x05,
    ATT_ERROR_REQUEST_NOT_SUPPORTED          = 0x06,
    ATT_ERROR_INVALID_OFFSET                 = 0x07,
    ATT_ERROR_INSUFFICIENT_AUTHORIZATION     = 0x08,
    ATT_ERROR_PREPARE_QUEUE_FULL             = 0x09,
    ATT_ERROR_ATTRIBUTE_NOT_FOUND            = 0x0A,
    ATT_ERROR_ATTRIBUTE_NOT_LONG             = 0x0B,
    ATT_ERROR_ENCRYPTION_KEY_SIZE_TOO_SHORT  = 0x0C,
    ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH = 0x0D,
    ATT_ERROR_UNLIKELY_ERROR                 = 0x0E,
    ATT_ERROR_INSUFFICIENT_ENCRYPTION        = 0x0F,
    ATT_ERROR_UNSUPPORTED_GROUP_TYPE         = 0x10,
    ATT_ERROR_INSUFFICIENT_RESOURCES         = 0x11,
};

BT_HDR* ATT_Build_ExchangeMTU_Request(uint16_t mtu);

BT_HDR* ATT_Build_FindInformation_Request(uint16_t startingHandle, uint16_t endingHandle);

BT_HDR* ATT_Build_ReadByType_Request(uint16_t startingHandle, uint16_t endingHandle, tBT_UUID uuid);

BT_HDR* ATT_Build_ReadByGroupType_Request(uint16_t startingHandle, uint16_t endingHandle, tBT_UUID uuid);

BT_HDR* ATT_Build_Read_Request(uint16_t handle);

BT_HDR* ATT_Build_ReadBlob_Request(uint16_t handle, uint16_t offset);

BT_HDR* ATT_Build_Write_Request(uint16_t handle, const void* data, uint16_t size);

BT_HDR* ATT_Build_Write_Command(uint16_t handle, const void* data, uint16_t size);
