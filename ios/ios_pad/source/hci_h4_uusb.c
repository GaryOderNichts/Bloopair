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
#include "stack/btu.h"

#define BTU_TASK            0
#define BTU_HCI_RCV_MBOX    0

// We need to hook this to add fragmentation support for BLE ACL data
// N didn't implement this for BLE (makes sense, they never supported it anyway)
void hci_h4_send_msg_hook(void*, BT_HDR* p_msg)
{
    uint8_t type = 0;
    uint8_t* p = ((uint8_t*)(p_msg + 1)) + p_msg->offset;
    uint16_t event = p_msg->event & BT_EVT_MASK;
    uint16_t sub_event = p_msg->event & BT_SUB_EVT_MASK;

    uint16_t acl_pkt_size = 0;
    uint16_t acl_data_size = 0;
    if (sub_event == LOCAL_BR_EDR_CONTROLLER_ID) {
        acl_data_size = btu_cb.hcit_acl_data_size;
        acl_pkt_size = btu_cb.hcit_acl_pkt_size;
    } else {
        acl_data_size = btu_cb.hcit_ble_acl_data_size;
        acl_pkt_size = btu_cb.hcit_ble_acl_pkt_size;
    }

    if (event == BT_EVT_TO_LM_HCI_ACL) {
        type = 2;

        /* Check if sending ACL data needs fragmenting */
        if (p_msg->len > acl_pkt_size) {
            uint16_t handle;
            BT_HDR* send_buf;

            /* Get the handle from the packet */
            handle = p[0] | p[1] << 8;
            // Bug: N advances past the handle, so the first fragmented message will be missing the
            //      handle. I guess the Wii U never sends messages > buffer size, so this was fine ig.
            // p += 2;

            /* Set packet boundary flags to "continuation packet" */
            handle = (handle & 0xCFFF) | 0x1000;

            send_buf = p_msg;
            while (p_msg->len > acl_pkt_size) {
                // This is how IOSU does it lol
                while (!(p_msg = GKI_getpoolbuf(3))) {
                    usleep(500);
                }

                // Copy over header
                memcpy(p_msg, send_buf, sizeof(BT_HDR));
                p_msg->len -= acl_data_size;
                p_msg->offset = 0;

                // Add handle to new packet
                uint8_t* new_p = ((uint8_t*)(p_msg + 1)) + p_msg->offset;
                *new_p++ = (handle) & 0xff;
                *new_p++ = (handle >> 8) & 0xff;

                // Add length to remainder packet
                if (p_msg->len > acl_pkt_size) {
                    *new_p++ = (acl_data_size) & 0xff;
                    *new_p++ = (acl_data_size >> 8) & 0xff;
                } else {
                    uint16_t len = p_msg->len - 4;
                    *new_p++ = (len) & 0xff;
                    *new_p++ = (len >> 8) & 0xff;
                }

                // Copy remaining data
                send_buf->len = acl_pkt_size;
                memcpy(new_p, p + acl_pkt_size, p_msg->len - 4);

                // Write truncated packet
                UUSB_Write(type, p, send_buf->len, send_buf);

                /* If we were only to send partial buffer, stop when done.    */
                /* Send the buffer back to L2CAP to send the rest of it later */
                if (p_msg->layer_specific) {
                    if (--p_msg->layer_specific == 0) {
                        p_msg->event = BT_EVT_TO_BTU_L2C_SEG_XMIT;
                        GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_msg);
                        return;
                    }
                }

                p = (uint8_t*)(p_msg + 1);
                send_buf = p_msg;
            }
        }
    } else if (event == BT_EVT_TO_LM_HCI_SCO) {
        type = 3;
    } else if (event == BT_EVT_TO_LM_HCI_CMD) {
        type = 0;
    } else {
        type = 2;
    }

    UUSB_Write(type, p, p_msg->len, p_msg);
}
