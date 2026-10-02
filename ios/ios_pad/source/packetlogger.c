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
#include <bt_api.h>
#include <imports.h>

#include "fsa.h"
#include "main.h"

// HCI PacketLogger according to
// - https://github.com/wireshark/wireshark/blob/dd0bed462bd2a80244dda18e318765eab3af6150/wiretap/packetlogger.c

#define PKLG_BUF_SIZE 1024 // should be more than enough

#define PKT_HCI_COMMAND    0x00
#define PKT_HCI_EVENT      0x01
#define PKT_SENT_ACL_DATA  0x02
#define PKT_RECV_ACL_DATA  0x03
#define PKT_SENT_SCO_DATA  0x08
#define PKT_RECV_SCO_DATA  0x09
#define PKT_LMP_SEND       0x0A
#define PKT_LMP_RECV       0x0B
#define PKT_SYSLOG         0xF7
#define PKT_KERNEL         0xF8
#define PKT_KERNEL_DEBUG   0xF9
#define PKT_ERROR          0xFA
#define PKT_POWER          0xFB
#define PKT_NOTE           0xFC
#define PKT_CONFIG         0xFD
#define PKT_NEW_CONTROLLER 0xFE

typedef struct PACKED {
    uint32_t len;
    uint32_t ts_secs;
    uint32_t ts_usecs;
    uint8_t type;
} packetlogger_header;
CHECK_SIZE(packetlogger_header, 0xD);

static int semaphore = -1;
static void* writeBuffer = NULL;
static int fileHandle = -1;

static uint8_t _pklg_event_to_type(uint16_t event)
{
    switch (event & BT_EVT_MASK) {
    case BT_EVT_TO_LM_HCI_CMD:
        return PKT_HCI_COMMAND;
    case BT_EVT_TO_BTU_HCI_EVT:
        return PKT_HCI_EVENT;
    case BT_EVT_TO_LM_HCI_ACL:
        return PKT_SENT_ACL_DATA;
    case BT_EVT_TO_BTU_HCI_ACL:
        return PKT_RECV_ACL_DATA;
    case BT_EVT_TO_LM_HCI_SCO:
        return PKT_SENT_SCO_DATA;
    case BT_EVT_TO_BTU_HCI_SCO:
        return PKT_RECV_SCO_DATA;
    default:
        return PKT_ERROR;
    }
}

static void _pklg_write_data(void* data, uint32_t size, uint16_t event)
{
    if (semaphore < 0) {
        semaphore = IOS_CreateSemaphore(1, 1);
        if (semaphore < 0) {
            DEBUG_PRINT("packetlogger: Failed to create semaphore\n");
        }
    }

    IOS_WaitSemaphore(semaphore, 0);

    // Make sure global FSA has already been initialized
    if (gFsaHandle < 0) {
        IOS_SignalSemaphore(semaphore);
        return;
    }

    if (!writeBuffer) {
        writeBuffer = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, PKLG_BUF_SIZE + sizeof(packetlogger_header), 0x40);
        if (!writeBuffer) {
            printf("packetlogger: Failed to allocate write buffer\n");
            IOS_SignalSemaphore(semaphore);
            return;
        }
    }

    if (fileHandle < 0) {
        IOSCalendarTime_t ctime;
        IOS_GetAbsTimeCalendar(&ctime);

        char name[512];
        snprintf(name, sizeof(name), BLOOPAIR_MOUNT_PATH "/wiiu/bloopair/logs/%04ld-%02ld-%02ld_%02ld-%02ld-%02ld.pklg",
                 ctime.year, ctime.month, ctime.day, ctime.hour, ctime.minute, ctime.second);

        int res = FSA_OpenFile(gFsaHandle, name, "w", &fileHandle);
        if (res < 0) {
            printf("packetlogger: Failed to open file\n");
            fileHandle = -1;
            IOS_SignalSemaphore(semaphore);
            return;
        }
    }

    size = MIN(size, PKLG_BUF_SIZE);

    uint64_t time;
    IOS_GetAbsTime64(&time);

    packetlogger_header* hdr = (packetlogger_header*) writeBuffer;
    hdr->len = size + 9;
    hdr->ts_secs = time / 1000000;
    hdr->ts_usecs = time - (hdr->ts_secs * 1000000);
    hdr->type = _pklg_event_to_type(event);
    memcpy((uint8_t*) writeBuffer + sizeof(packetlogger_header), data, size);

    if (FSA_WriteFile(gFsaHandle, writeBuffer, 1, sizeof(packetlogger_header) + size, fileHandle, 0) < 0) {
        FSA_CloseFile(gFsaHandle, fileHandle);
        fileHandle = -1;
    } else {
        FSA_FlushFile(gFsaHandle, fileHandle);
    }

    IOS_SignalSemaphore(semaphore);
}

void (*real_data_ind)(BT_HDR* p_buf) = (void*) DEFINE_REAL(0x11f2fe94, 0xe1a02000);
void data_ind_hook(BT_HDR* p_buf)
{
    _pklg_write_data((uint8_t*) (p_buf + 1) + p_buf->offset, p_buf->len, p_buf->event);

    real_data_ind(p_buf);
}

int (*real_uusb_transfer)(BT_HDR* p_buf) = (void*) DEFINE_REAL(0x11f3ba1c, 0xe1a01000);
int uusb_transfer_hook(BT_HDR* p_buf)
{
    _pklg_write_data((uint8_t*) (p_buf + 1) + p_buf->offset, p_buf->len, p_buf->event);

    return real_uusb_transfer(p_buf);
}
