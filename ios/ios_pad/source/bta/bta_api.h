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
#include <bt_api.h>

typedef uint8_t (tBTA_DM_BLE_SEL_CBACK)(BD_ADDR random_bda, uint8_t* p_remote_name);


/* Inquiry filter device class condition */
typedef struct
{
    DEV_CLASS       dev_class;        /* device class of interest */
    DEV_CLASS       dev_class_mask;   /* mask to determine the bits of device class of interest */
} tBTA_DM_COD_COND;

/* Inquiry Filter Condition */
typedef union
{
    BD_ADDR              bd_addr;            /* BD address of  device to filter. */
    tBTA_DM_COD_COND     dev_class_cond;     /* Device class filter condition */
} tBTA_DM_INQ_COND;

/* Inquiry Parameters */
typedef struct
{
    uint8_t             mode;           /* Inquiry mode, limited or general. */
    uint8_t             duration;       /* Inquiry duration in 1.28 sec units. */
    uint8_t             max_resps;      /* Maximum inquiry responses.  Set to zero for unlimited responses. */
    uint8_t             report_dup;     /* report duplicated inquiry response with higher RSSI value */
    uint8_t             filter_type;    /* Filter condition type. */
    tBTA_DM_INQ_COND    filter_cond;    /* Filter condition data. */
} tBTA_DM_INQ;


/* Search callback events */
#define BTA_DM_INQ_RES_EVT              0       /* Inquiry result for a peer device. */
#define BTA_DM_INQ_CMPL_EVT             1       /* Inquiry complete. */
#define BTA_DM_DISC_RES_EVT             2       /* Discovery result for a peer device. */
// wiiu-edit: no gatt
// #define BTA_DM_DISC_BLE_RES_EVT         3       /* Discovery result for BLE GATT based servoce on a peer device. */
#define BTA_DM_DISC_CMPL_EVT            4       /* Discovery complete. */
#define BTA_DM_DI_DISC_CMPL_EVT         5       /* Discovery complete. */
#define BTA_DM_SEARCH_CANCEL_CMPL_EVT   6       /* Search cancelled */

/* Structure associated with BTA_DM_DISC_RES_EVT */
typedef struct {
    BD_ADDR  bd_addr;        /* BD address peer device. */
    BD_NAME  bd_name;        /* Name of peer device. */
    uint32_t services;       /* Services found on peer device. */
    uint8_t result;
} tBTA_DM_DISC_RES;

/* Structure associated with BTA_DM_INQ_RES_EVT */
typedef struct
{
    BD_ADDR       bd_addr;               /* BD address peer device. */
    DEV_CLASS     dev_class;             /* Device class of peer device. */
    uint8_t       remt_name_not_required;   /* Application sets this flag if it already knows the name of the device */
                                            /* If the device name is known to application BTA skips the remote name request */
    uint8_t       is_limited;               /* TRUE, if the limited inquiry bit is set in the CoD */
    int8_t        rssi;                     /* The rssi value */
    uint8_t       *p_eir;                   /* received EIR */
    uint8_t       inq_result_type;
    uint8_t       ble_addr_type;
    uint8_t       ble_evt_type;
    uint8_t       device_type;
} tBTA_DM_INQ_RES;

/* Security Callback Events */
#define BTA_DM_LINK_UP_EVT              5       /* Connection UP event */
#define BTA_DM_SP_CFM_REQ_EVT           10 /* Simple Pairing User Confirmation request. */

typedef struct {
    BT_HDR  hdr;
    BD_ADDR bd_addr;
    uint8_t accept;
} tBTA_DM_API_CONFIRM;

/* Structure associated with BTA_DM_SP_CFM_REQ_EVT */
typedef struct {
    BD_ADDR   bd_addr;        /* peer address */
    DEV_CLASS dev_class;      /* peer CoD */
    BD_NAME   bd_name;        /* peer device name */
    uint32_t  num_val;        /* the numeric value for comparison. If just_works, do not show this number to UI */
    uint8_t   just_works;     /* TRUE, if "Just Works" association model */
    uint8_t   loc_auth_req;   /* Authentication required for local device */
    uint8_t   rmt_auth_req;   /* Authentication required for peer device */
    uint8_t   loc_io_caps;    /* IO Capabilities of local device */
    uint8_t   rmt_io_caps;    /* IO Capabilities of remote device */
} tBTA_DM_SP_CFM_REQ;

/* Execute call back */
typedef void (tBTA_DM_EXEC_CBACK) (void * p_param);

typedef struct
{
    BT_HDR                  hdr;
    uint8_t                 bg_conn_type;
    tBTA_DM_BLE_SEL_CBACK   *p_select_cback;
}tBTA_DM_API_BLE_SET_BG_CONN_TYPE;

typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    uint8_t             dev_type;
    uint8_t             addr_type;
}tBTA_DM_API_ADD_BLE_DEVICE;

void bta_dmexecutecallback(tBTA_DM_EXEC_CBACK* p_callback, void* p_param);
void BTA_DmSetAfhChannels(uint8_t first, uint8_t last);
void BTA_DmAddDevice(uint8_t* bd_addr, uint8_t* dev_class, uint8_t* link_key, uint32_t trusted_mask, uint8_t is_trusted, uint8_t key_type, uint8_t io_cap);
void BTA_DmConfirm(uint8_t* bd_addr, uint8_t accept);
void BTA_DmAddBleDevice(BD_ADDR bd_addr, uint8_t addr_type, uint8_t dev_type);
void BTA_DmBleSetBgConnType(uint8_t bg_conn_type, tBTA_DM_BLE_SEL_CBACK *p_select_cback);
