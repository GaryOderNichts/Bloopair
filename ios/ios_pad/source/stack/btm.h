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

/* scanning enable status */
#define BTM_BLE_SCAN_ENABLE      0x01
#define BTM_BLE_SCAN_DISABLE     0x00

/* advertising enable status */
#define BTM_BLE_ADV_ENABLE     0x01
#define BTM_BLE_ADV_DISABLE    0x00

#define BTM_BLE_DUPLICATE_ENABLE        1
#define BTM_BLE_DUPLICATE_DISABLE       0

typedef uint8_t (tBTM_BLE_SEL_CBACK)(BD_ADDR random_bda, uint8_t* p_remote_name);

typedef struct
{
    uint8_t             ble_addr_type;  /* LE device type: public or random address */
    BD_ADDR             reconn_addr;    /* reconnect address */
    BD_ADDR             cur_rand_addr;  /* current random address */
    BD_ADDR             static_addr;    /* static address */
} tBTM_SEC_BLE;
CHECK_SIZE(tBTM_SEC_BLE, 0x13);

typedef struct
{
    uint16_t              min_conn_int;
    uint16_t              max_conn_int;
    uint16_t              slave_latency;
    uint16_t              supervision_tout;

}tBTM_LE_CONN_PRAMS;
CHECK_SIZE(tBTM_LE_CONN_PRAMS, 0x8);

/*
** Define structure for Security Device Record.
** A record exists for each device authenticated with this device
*/
#define BTM_SEC_SERVICE_ARRAY_SIZE 3
typedef struct
{
    void                *p_cur_service;
    void                *p_callback;
    void                *p_ref_data;
    uint32_t             timestamp;         /* Timestamp of the last connection   */
    uint32_t             trusted_mask[BTM_SEC_SERVICE_ARRAY_SIZE];  /* Bitwise OR of trusted services     */
    uint16_t             hci_handle;        /* Handle to connection when exists   */
    uint16_t             clock_offset;      /* Latest known clock offset          */
    BD_ADDR              bd_addr;           /* BD_ADDR of the device              */
    DEV_CLASS            dev_class;         /* DEV_CLASS of the device            */
    LINK_KEY             link_key;          /* Device link key                    */

    uint8_t         sec_bd_name[65];    /* User friendly name of the device. (may be truncated to save space in dev_rec table) */
    uint8_t         sec_flags;          /* Current device security state      */
    uint8_t         features[8];        /* Features suported by the device    */

    uint8_t     sec_state;              /* Operating state                    */
    uint8_t     is_originator;          /* TRUE if device is originating connection */
    uint8_t     role_master;            /* TRUE if current mode is master     */
    uint16_t    security_required;      /* Security required for connection   */
    uint8_t     link_key_not_sent;      /* link key notification has not been sent waiting for name */
    uint8_t     link_key_type;          /* Type of key used in pairing   */
    uint8_t     link_key_changed;       /* Changed link key during current connection */

    uint8_t     sm4;                    /* BTM_SM4_TRUE, if the peer supports SM4 */
    uint8_t     rmt_io_caps;            /* IO capability of the peer device */
    uint8_t     rmt_auth_req;           /* the auth_req flag as in the IO caps rsp evt */

    uint8_t               enc_key_size;           /* current link encryption key size */
    tBTM_SEC_BLE        ble;
    uint8_t     device_type;
    tBTM_LE_CONN_PRAMS  conn_params;
} tBTM_SEC_DEV_REC;
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0x1c, hci_handle);
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0x7a, sec_flags);
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0x86, security_required);
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0x8b, sm4);
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0x8f, ble);
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0xa2, device_type);
CHECK_OFFSET(tBTM_SEC_DEV_REC, 0xa4, conn_params);
CHECK_SIZE(tBTM_SEC_DEV_REC, 0xac);

/* random address management control block */
typedef struct
{
    BD_ADDR			            private_addr;
    BD_ADDR                     random_bda;
    uint8_t                     busy;
    uint16_t                    index;
    void      *p_resolve_cback;
    void                        *p;
    TIMER_LIST_ENT              raddr_timer_ent;
} tBTM_LE_RANDOM_CB;
CHECK_SIZE(tBTM_LE_RANDOM_CB, 0x30);

#define BTM_BLE_MAX_BG_CONN_DEV_NUM    10

typedef struct
{
    // force 32-bit alignment
    uint32_t force_alignment;
    uint8_t _todo[0xb0];
} tBTM_BLE_INQ_CB;
CHECK_SIZE(tBTM_BLE_INQ_CB, 0xb4);

typedef struct
{
    /*****************************************************
    **      BLE Inquiry
    *****************************************************/
    tBTM_BLE_INQ_CB     inq_var;

    /* background connection procedure cb value */
    uint8_t  bg_conn_type;
    uint16_t              scan_int;
    uint16_t              scan_win;
    tBTM_BLE_SEL_CBACK  *p_select_cback;
    TIMER_LIST_ENT      scan_param_idle_timer;

    uint8_t             bg_conn_dev_num;
    BD_ADDR             bg_conn_dev_list[BTM_BLE_MAX_BG_CONN_DEV_NUM];

#define BLE_BG_CONN_IDLE    0
#define BLE_BG_CONN_ACTIVE  1
#define BLE_BG_CONN_SUSPEND 2

    uint8_t               bg_conn_state;

    /* random address management control block */
    tBTM_LE_RANDOM_CB   addr_mgnt_cb;

    /* white list information */
    uint8_t            num_empty_filter;      /* Number of entries in white list */
    uint8_t            max_filter_entries;    /* Maximum number of entries that can be stored */
    uint8_t          enabled;
    uint8_t          privacy;               /* privacy enabled or disabled */
} tBTM_BLE_CB;
CHECK_OFFSET(tBTM_BLE_CB, 0xb4, bg_conn_type);
CHECK_OFFSET(tBTM_BLE_CB, 0xd8, bg_conn_dev_num);
CHECK_OFFSET(tBTM_BLE_CB, 0xd9, bg_conn_dev_list);
CHECK_OFFSET(tBTM_BLE_CB, 0x115, bg_conn_state);
CHECK_SIZE(tBTM_BLE_CB, 0x14c);

typedef struct
{
    void *p_remname_cmpl_cb;
    uint8_t _todo[0x5ed-4];
    uint8_t    state;
    uint8_t            inq_active;        /* Bit Mask indicating type of inquiry is active */
    uint8_t no_inc_ssp;
} tBTM_INQUIRY_VAR_ST;
CHECK_OFFSET(tBTM_INQUIRY_VAR_ST, 0x5ee, inq_active);
CHECK_SIZE(tBTM_INQUIRY_VAR_ST, 0x5f0);

typedef struct {
    uint8_t _todo0[0xb3c];
    tBTM_BLE_CB ble_ctr_cb;
    uint8_t _todo1[0x1cd0 - 0xc88];
    tBTM_INQUIRY_VAR_ST btm_inq_vars;
    uint8_t _todo2[0x3518 - 0x22c0];
} tBTM_CB;
CHECK_OFFSET(tBTM_CB, 0xb3c, ble_ctr_cb);
CHECK_OFFSET(tBTM_CB, 0x1cd0, btm_inq_vars);
CHECK_OFFSET(tBTM_CB, 0x22be, btm_inq_vars.inq_active);
CHECK_SIZE(tBTM_CB, 0x3518);

extern tBTM_CB btm_cb;

// extern
uint8_t BTM_IsAclConnectionUp(BD_ADDR remote_bda);

uint8_t BTM_BleUpdateBgConnDev(uint8_t add_remove, BD_ADDR remote_bda);

uint8_t btm_update_bg_conn_list(uint8_t to_add, BD_ADDR bd_addr);

uint8_t btm_ble_find_dev_in_whitelist(BD_ADDR bd_addr);

uint8_t btm_update_dev_to_white_list(uint8_t to_add, BD_ADDR bd_addr, uint8_t addr_type);

// extern 
uint8_t btm_ble_start_auto_conn(uint8_t start);

// extern
uint8_t btm_ble_start_select_conn(uint8_t start, tBTM_BLE_SEL_CBACK* p_select_cback);

// extern
uint8_t btm_ble_resume_bg_conn(tBTM_BLE_SEL_CBACK* p_sele_callback, uint8_t def_param);

// extern
uint8_t btm_remove_acl(BD_ADDR bd_addr);

// extern
tBTM_SEC_DEV_REC* btm_find_dev(BD_ADDR bd_addr);

// extern
tBTM_SEC_DEV_REC* btm_find_dev_by_handle(uint16_t handle);
