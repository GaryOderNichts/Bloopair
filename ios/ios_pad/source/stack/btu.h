/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#pragma once

#include "imports.h"
#include "bt_api.h"
#include "utils.h"

/* callbacks
*/
typedef void (*tBTU_TIMER_CALLBACK)(TIMER_LIST_ENT *p_tle);
typedef void (*tBTU_EVENT_CALLBACK)(BT_HDR *p_hdr);

/* structure to hold registered timers */
typedef struct
{
    TIMER_LIST_ENT          *p_tle;      /* timer entry */
    tBTU_TIMER_CALLBACK     timer_cb;    /* callback triggered when timer expires */
} tBTU_TIMER_REG;

/* structure to hold registered event callbacks */
typedef struct
{
    uint16_t                  event_range;  /* start of event range */
    tBTU_EVENT_CALLBACK     event_cb;     /* callback triggered when event is in range */
} tBTU_EVENT_REG;

/* AMP HCI control block */
typedef struct
{
    BUFFER_Q         cmd_xmit_q;
    BUFFER_Q         cmd_cmpl_q;
    uint16_t           cmd_window;
    TIMER_LIST_ENT   cmd_cmpl_timer;        /* Command complete timer */
} tHCI_CMD_CB;
CHECK_SIZE(tHCI_CMD_CB, 0x34);

/* Define structure holding BTU variables
*/
typedef struct
{
    tBTU_TIMER_REG   timer_reg[2];
    tBTU_EVENT_REG   event_reg[6];

    TIMER_LIST_Q  quick_timer_queue;        /* Timer queue for transport level (100/10 msec)*/
    TIMER_LIST_Q  timer_queue;              /* Timer queue for normal BTU task (1 second)   */

    TIMER_LIST_ENT   cmd_cmpl_timer;        /* Command complete timer */

    uint16_t    hcit_acl_data_size;           /* Max ACL data size across HCI transport    */
    uint16_t    hcit_acl_pkt_size;            /* Max ACL packet size across HCI transport  */
                                            /* (this is data size plus 4 bytes overhead) */

    uint16_t    hcit_ble_acl_data_size;           /* Max BLE ACL data size across HCI transport    */
    uint16_t    hcit_ble_acl_pkt_size;            /* Max BLE ACL packet size across HCI transport  */
                                            /* (this is data size plus 4 bytes overhead) */

    uint8_t     reset_complete;             /* TRUE after first ack from device received */
    uint8_t       trace_level;                /* Trace level for HCI layer */

    // wiiu-edit
    uint8_t unknown[10];

    tHCI_CMD_CB hci_cmd_cb[1]; /* including BR/EDR */
} tBTU_CB;
CHECK_OFFSET(tBTU_CB, 0x70, hcit_acl_data_size);
CHECK_OFFSET(tBTU_CB, 0x72, hcit_acl_pkt_size);
CHECK_OFFSET(tBTU_CB, 0x74, hcit_ble_acl_data_size);
CHECK_OFFSET(tBTU_CB, 0x76, hcit_ble_acl_pkt_size);
CHECK_SIZE(tBTU_CB, 0xb8);

extern tBTU_CB btu_cb;
