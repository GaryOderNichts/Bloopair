/******************************************************************************
 *
 *  Copyright (C) 2002-2012 Broadcom Corporation
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
#include "bt_api.h"

#define HID_HOST_MAX_DEVICES 16

typedef struct desc_info
{
    uint16_t dl_len;
    uint8_t *dsc_list;
} tHID_DEV_DSCP_INFO;
CHECK_SIZE(tHID_DEV_DSCP_INFO, 0x8);

typedef struct
{
    char svc_name[32];   /*Service Name */
    char svc_descr[32]; /*Service Description*/
    char prov_name[32]; /*Provider Name.*/
    uint16_t    rel_num;    /*Release Number */
    uint16_t    hpars_ver;  /*HID Parser Version.*/

    //wiiu-edit: snip
#if 0
    uint16_t    ssr_max_latency; /* HIDSSRHostMaxLatency value, if HID_SSR_PARAM_INVALID not used*/
    uint16_t    ssr_min_tout; /* HIDSSRHostMinTimeout value, if HID_SSR_PARAM_INVALID not used* */
#endif

    uint8_t     sub_class;    /*Device Subclass.*/
    uint8_t     ctry_code;     /*Country Code.*/
    uint16_t    sup_timeout;/* Supervisory Timeout */

    tHID_DEV_DSCP_INFO  dscp_info;   /* Descriptor list and Report list to be set in the SDP record.
                                       This parameter is used if HID_DEV_USE_GLB_SDP_REC is set to FALSE.*/
    void       *p_sdp_layer_rec;
} tHID_DEV_SDP_INFO;
CHECK_SIZE(tHID_DEV_SDP_INFO, 0x74);

typedef struct
{
    uint8_t             conn_state;
    uint8_t             conn_flags;
    uint8_t             ctrl_id;
    uint16_t            ctrl_cid;
    uint16_t            intr_cid;
    uint16_t            rem_mtu_size;
    uint16_t            disc_reason;                       /* Reason for disconnecting (for HID_HDEV_EVT_CLOSE) */
    TIMER_LIST_ENT      timer_entry;
} tHID_CONN;
CHECK_SIZE(tHID_CONN, 0x24);

typedef struct
{
    uint8_t        in_use;
    BD_ADDR        addr;  /* BD-Addr of the host device */
    uint16_t       attr_mask; /* 0x01- virtual_cable; 0x02- normally_connectable; 0x03- reconn_initiate;
    			                 0x04- sdp_disable; */
    uint8_t        state;  /* Device state if in HOST-KNOWN mode */
    uint8_t        conn_substate;
    uint8_t        conn_tries; /* Remembers to the number of connection attempts while CONNECTING */

    tHID_CONN      conn; /* L2CAP channel info */
} tHID_HOST_DEV_CTB;
CHECK_OFFSET(tHID_HOST_DEV_CTB, 0x1, addr);
CHECK_OFFSET(tHID_HOST_DEV_CTB, 0x8, attr_mask);
CHECK_OFFSET(tHID_HOST_DEV_CTB, 0xa, state);
CHECK_OFFSET(tHID_HOST_DEV_CTB, 0xb, conn_substate);
CHECK_OFFSET(tHID_HOST_DEV_CTB, 0xc, conn_tries);
CHECK_OFFSET(tHID_HOST_DEV_CTB, 0x10, conn);
CHECK_SIZE(tHID_HOST_DEV_CTB, 0x34);

typedef struct host_ctb
{
    tHID_HOST_DEV_CTB       devices[HID_HOST_MAX_DEVICES];
    void  *callback;             /* Application callbacks */
    tL2CAP_CFG_INFO         l2cap_cfg;

    uint8_t                 sdp_busy;
    void  *sdp_cback;
    void       *p_sdp_db;
    tHID_DEV_SDP_INFO       sdp_rec;
    uint8_t                 reg_flag;
    uint8_t                 trace_level;
} tHID_HOST_CTB;
CHECK_SIZE(tHID_HOST_CTB, 0x410);

extern tHID_HOST_CTB hh_cb;

uint8_t HID_HostAddDev(BD_ADDR addr, uint16_t attr_mask, uint8_t* handle);

// wiiu-edit:
// HID_HostRemoveDev, but without closing the device
// allows us to remove our BLE claimed handles
uint8_t HID_HostRemoveDevNoClose(uint8_t dev_handle);
