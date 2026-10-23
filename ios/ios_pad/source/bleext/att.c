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
#include "att.h"

static BT_HDR* _ATT_Allocate_Request(uint32_t size)
{
    BT_HDR* buf = GKI_getbuf(sizeof(BT_HDR) + size + L2CAP_MIN_OFFSET);
    if (!buf) {
        return NULL;
    }

    buf->offset = L2CAP_MIN_OFFSET;
    buf->len = size;

    return buf;
}

BT_HDR* ATT_Build_ExchangeMTU_Request(uint16_t mtu)
{
    BT_HDR* buf = _ATT_Allocate_Request(3);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_EXCHANGE_MTU_REQ;
    *p++ = mtu & 0xff;
    *p++ = (mtu >> 8) & 0xff;

    return buf;
}

BT_HDR* ATT_Build_FindInformation_Request(uint16_t startingHandle, uint16_t endingHandle)
{
    BT_HDR* buf = _ATT_Allocate_Request(5);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_FIND_INFORMATION_REQ;
    *p++ = startingHandle & 0xff;
    *p++ = (startingHandle >> 8) & 0xff;
    *p++ = endingHandle & 0xff;
    *p++ = (endingHandle >> 8) & 0xff;

    return buf;
}

BT_HDR* ATT_Build_ReadByType_Request(uint16_t startingHandle, uint16_t endingHandle, tBT_UUID uuid)
{
    BT_HDR* buf = _ATT_Allocate_Request(5 + uuid.len);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_READ_BY_TYPE_REQ;
    *p++ = startingHandle & 0xff;
    *p++ = (startingHandle >> 8) & 0xff;
    *p++ = endingHandle & 0xff;
    *p++ = (endingHandle >> 8) & 0xff;
    if (uuid.len == LEN_UUID_16) {
        *p++ = uuid.uu.uuid16 & 0xff;
        *p++ = (uuid.uu.uuid16 >> 8) & 0xff;
    } else if (uuid.len == LEN_UUID_128) {
        memcpy(p, uuid.uu.uuid128, LEN_UUID_128);
    }

    return buf;
}

BT_HDR* ATT_Build_ReadByGroupType_Request(uint16_t startingHandle, uint16_t endingHandle, tBT_UUID uuid)
{
    BT_HDR* buf = _ATT_Allocate_Request(5 + uuid.len);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_READ_BY_GROUP_TYPE_REQ;
    *p++ = startingHandle & 0xff;
    *p++ = (startingHandle >> 8) & 0xff;
    *p++ = endingHandle & 0xff;
    *p++ = (endingHandle >> 8) & 0xff;
    if (uuid.len == LEN_UUID_16) {
        *p++ = uuid.uu.uuid16 & 0xff;
        *p++ = (uuid.uu.uuid16 >> 8) & 0xff;
    } else if (uuid.len == LEN_UUID_128) {
        memcpy(p, uuid.uu.uuid128, LEN_UUID_128);
    }

    return buf;
}

BT_HDR* ATT_Build_Read_Request(uint16_t handle)
{
    BT_HDR* buf = _ATT_Allocate_Request(3);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_READ_REQ;
    *p++ = handle & 0xff;
    *p++ = (handle >> 8) & 0xff;

    return buf;
}

BT_HDR* ATT_Build_ReadBlob_Request(uint16_t handle, uint16_t offset)
{
    BT_HDR* buf = _ATT_Allocate_Request(5);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_READ_BLOB_REQ;
    *p++ = handle & 0xff;
    *p++ = (handle >> 8) & 0xff;
    *p++ = offset & 0xff;
    *p++ = (offset >> 8) & 0xff;

    return buf;
}

BT_HDR* ATT_Build_Write_Request(uint16_t handle, const void* data, uint16_t size)
{
    uint16_t payloadSize = 1 + 2 + size; // opcode + handle + data

    BT_HDR* buf = _ATT_Allocate_Request(payloadSize);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_WRITE_REQ;
    *p++ = handle & 0xff;
    *p++ = (handle >> 8) & 0xff;
    memcpy(p, data, size);

    return buf;
}

BT_HDR* ATT_Build_Write_Command(uint16_t handle, const void* data, uint16_t size)
{
    uint16_t payloadSize = 1 + 2 + size; // opcode + handle + data

    BT_HDR* buf = _ATT_Allocate_Request(payloadSize);
    if (!buf) {
        return NULL;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;

    *p++ = ATT_WRITE_CMD;
    *p++ = handle & 0xff;
    *p++ = (handle >> 8) & 0xff;
    memcpy(p, data, size);

    return buf;
}
