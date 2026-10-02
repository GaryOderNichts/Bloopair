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

#include <imports.h>
#include "stack/sdp.h"

/* Structures are based on reverse engineering and https://android.googlesource.com/platform/external/bluetooth/bluedroid/ */

#define BD_NAME_LEN     248
typedef uint8_t BD_NAME[BD_NAME_LEN];         /* Device name */

#define BD_ADDR_LEN     6                   /* Device address length */
typedef uint8_t BD_ADDR[BD_ADDR_LEN];         /* Device address */

#define DEV_CLASS_LEN   3
typedef uint8_t DEV_CLASS[DEV_CLASS_LEN];     /* Device class */

#define LINK_KEY_LEN    16
typedef uint8_t LINK_KEY[LINK_KEY_LEN];       /* Link Key */

typedef struct {
    uint16_t event;
    uint16_t len;
    uint16_t offset;
    uint16_t layer_specific;
} BT_HDR;
CHECK_SIZE(BT_HDR, 0x8);

/* Structure returned with remote name  request */
typedef struct
{
    uint16_t    status;
    uint16_t    length;
    BD_NAME     remote_bd_name;
} tBTM_REMOTE_DEV_NAME;

typedef struct
{
    BT_HDR   hdr;
    BD_ADDR  bd_addr;
    uint8_t  sec_mask;
    uint8_t  mode;
} tBTA_HH_API_CONN;

#define BTA_HH_SDP_CMPL_EVT  0x1707
#define BTA_HH_OPEN_CMPL_EVT 0x170b

extern uint16_t sdp_db_size;

/* BTA HID Host callback events */
#define BTA_HH_ENABLE_EVT       0       /* HH enabled */
#define BTA_HH_DISABLE_EVT      1       /* HH disabled */
#define BTA_HH_OPEN_EVT         2       /* connection opened */
#define BTA_HH_CLOSE_EVT        3       /* connection closed */
#define BTA_HH_GET_DSCP_EVT     10      /* Get report descripotor */
#define BTA_HH_ADD_DEV_EVT      11      /* Add Device callback */
#define BTA_HH_VC_UNPLUG_EVT    13      /* virtually unplugged */

/* callback event data for BTA_HH_OPEN_EVT */
typedef struct {
    BD_ADDR  bda;    /* HID device bd address    */
    uint8_t  status; /* operation status         */
    uint8_t  handle; /* device handle            */
} tBTA_HH_CONN;

/* callback event data for BTA_HH_CLOSE_EVT */
typedef struct
{
    uint8_t  status;     /* operation status         */
    uint8_t  handle;     /* device handle            */
} tBTA_HH_CBDATA;

#define BTA_HH_MAX_RPT_CHARS 10

#define BTA_HH_MAX_KNOWN 0x10

/* invalid device handle */
#define BTA_HH_INVALID_HANDLE   0xff

/* report descriptor information */
typedef struct {
    uint16_t dl_len;
    uint8_t *dsc_list;
} tBTA_HH_DEV_DESCR;

/* device control block */
typedef struct
{
    tBTA_HH_DEV_DESCR   dscp_info;      /* report descriptor */
    BD_ADDR             addr;           /* BD-Addr of the HID device */
    uint16_t            attr_mask;      /* attribute mask */
    uint16_t            w4_evt;         /* W4_handshake event name */
    uint8_t             index;          /* index number referenced to handle index */
    uint8_t             sub_class;      /* Cod sub class */
    uint8_t             unk1;
    uint8_t             sec_mask;       /* security mask */
    uint8_t             app_id;         /* application ID for this connection */
    uint8_t             hid_handle;     /* device handle */
    uint8_t             vp;             /* virtually unplug flag */
    uint8_t             in_use;         /* control block currently in use */
    uint8_t             incoming_conn;  /* is incoming connection? */
    uint8_t             unk2;
    uint8_t             mode;           /* protocol mode */
    uint8_t             state;          /* CB state */
} tBTA_HH_DEV_CB;

/* key board parsing control block */
typedef struct
{
    uint8_t             mod_key[4]; /* ctrl, shift(upper), Alt, GUI */
    uint8_t             num_lock;
    uint8_t             caps_lock;
    uint8_t             last_report[BTA_HH_MAX_RPT_CHARS];
} tBTA_HH_KB_CB;

/******************************************************************************
** Main Control Block
*******************************************************************************/
typedef struct
{
    tBTA_HH_KB_CB           kb_cb;                  /* key board control block,
                                                       suppose BTA will connect
                                                       to only one keyboard at
                                                        the same time */
    tBTA_HH_DEV_CB          kdev[BTA_HH_MAX_KNOWN]; /* device control block */
    tBTA_HH_DEV_CB*         p_cur;                  /* current device control
                                                       block idx, used in sdp */
    uint8_t                 cb_index[BTA_HH_MAX_KNOWN]; /* maintain a CB index
                                                           map to dev handle */
    void                    *p_cback;               /* Application callbacks */
    tSDP_DISCOVERY_DB       *p_disc_db;
    uint8_t                 trace_level;            /* tracing level */
    uint8_t                 cnt_num;                /* connected device number */
    uint8_t                 w4_disable;             /* w4 disable flag */
} tBTA_HH_CB;
CHECK_SIZE(tBTA_HH_CB, 0x230);

/* Inquiry modes */
#define BTM_GENERAL_INQUIRY         0
#define BTM_LIMITED_INQUIRY         1
#define BTM_BR_INQUIRY_MASK         0x0f
/* high byte of inquiry mode for BLE inquiry mode */
#define BTM_BLE_INQUIRY_NONE        0x00
#define BTM_BLE_GENERAL_INQUIRY     0x10
#define BTM_BLE_LIMITED_INQUIRY     0x20
#define BTM_BLE_INQUIRY_MASK        (BTM_BLE_GENERAL_INQUIRY|BTM_BLE_LIMITED_INQUIRY)

/* Inquiry Filter Condition types  */
#define BTM_CLR_INQUIRY_FILTER          0 /* Inquiry Filtering is turned off */
#define BTM_FILTER_COND_DEVICE_CLASS    1 /* Filter on device class */
#define BTM_FILTER_COND_BD_ADDR         2 /* Filter on device addr */

/* minor device class field for Peripheral Major Class */
#define BTM_COD_MINOR_JOYSTICK              0x04
#define BTM_COD_MINOR_GAMEPAD               0x08

/***************************
** major device class field
****************************/
#define BTM_COD_MAJOR_PERIPHERAL            0x05

/* the COD masks */
#define BTM_COD_FORMAT_TYPE_MASK      0x03
#define BTM_COD_MINOR_CLASS_MASK      0xFC
#define BTM_COD_MAJOR_CLASS_MASK      0x1F

/* Address types
*/
#define BLE_ADDR_PUBLIC         0x00
#define BLE_ADDR_RANDOM         0x01
#define BLE_ADDR_IS_STATIC(x)   ((x[0] & 0xC0) == 0xC0)

/* Device Types
*/
#define BT_DEVICE_TYPE_BREDR   0x01
#define BT_DEVICE_TYPE_BLE     0x02

enum
{
    BTM_BLE_CONN_NONE,
    BTM_BLE_CONN_AUTO,
    BTM_BLE_CONN_SELECTIVE
};

enum
{
    BTA_HH_RPTT_RESRV,      /* reserved         */
    BTA_HH_RPTT_INPUT,      /* input report     */
    BTA_HH_RPTT_OUTPUT,     /* output report    */
    BTA_HH_RPTT_FEATURE     /* feature report   */
};

#define HID_TRANS_SET_REPORT    (5)

/* Security Service Levels [bit mask] (BTM_SetSecurityLevel)
** Encryption should not be used without authentication
*/
#define BTM_SEC_NONE               0x0000 /* Nothing required */
#define BTM_SEC_IN_AUTHORIZE       0x0001 /* Inbound call requires authorization */
#define BTM_SEC_IN_AUTHENTICATE    0x0002 /* Inbound call requires authentication */
#define BTM_SEC_IN_ENCRYPT         0x0004 /* Inbound call requires encryption */
#define BTM_SEC_OUT_AUTHORIZE      0x0008 /* Outbound call requires authorization */
#define BTM_SEC_OUT_AUTHENTICATE   0x0010 /* Outbound call requires authentication */
#define BTM_SEC_OUT_ENCRYPT        0x0020 /* Outbound call requires encryption */
#define BTM_SEC_BOND               0x0040 /* Bonding */
#define BTM_SEC_BOND_CONN          0x0080 /* bond_created_connection */
#define BTM_SEC_FORCE_MASTER       0x0100 /* Need to switch connection to be master */
#define BTM_SEC_ATTEMPT_MASTER     0x0200 /* Try to switch connection to be master */
#define BTM_SEC_FORCE_SLAVE        0x0400 /* Need to switch connection to be master */
#define BTM_SEC_ATTEMPT_SLAVE      0x0800 /* Try to switch connection to be slave */
#define BTM_SEC_IN_MITM            0x1000 /* inbound Do man in the middle protection */
#define BTM_SEC_OUT_MITM           0x2000 /* outbound Do man in the middle protection */

/* Security Flags [bit mask] (BTM_GetSecurityFlags)
*/
#define BTM_SEC_FLAG_AUTHORIZED     0x01
#define BTM_SEC_FLAG_AUTHENTICATED  0x02
#define BTM_SEC_FLAG_ENCRYPTED      0x04
#define BTM_SEC_FLAG_LKEY_KNOWN     0x10
#define BTM_SEC_FLAG_LKEY_AUTHED    0x20

typedef struct
{
    void    *p_first;
    void    *p_last;
    uint16_t count;
} BUFFER_Q;

#define BTM_BLE_CACHE_ADV_DATA_MAX      62
#define BTM_BLE_ADV_DATA_LEN_MAX        31

#define BTM_BLE_AD_TYPE_FLAG            0x01
#define BTM_BLE_AD_TYPE_SRV_PART        0x02
#define BTM_BLE_AD_TYPE_SRV_CMPL        0x03
#define BTM_BLE_AD_TYPE_NAME_SHORT      0x08
#define BTM_BLE_AD_TYPE_NAME_CMPL       0x09
#define BTM_BLE_AD_TYPE_TX_PWR          0x0A
#define BTM_BLE_AD_TYPE_DEV_CLASS       0x0D
#define BTM_BLE_AD_TYPE_ATTR            0x10
#define BTM_BLE_AD_TYPE_MANU            0xff
#define BTM_BLE_AD_TYPE_INT_RANGE       0x12
#define BTM_BLE_AD_TYPE_SOL_SRV_UUID    0x14
// wiiu-edit: these were added by myself
#define BTM_BLE_AD_TYPE_APPEARANCE      0x19

#define L2CAP_MIN_OFFSET    13     /* plus control(2), SDU length(2) */

/* local Bluetooth controller id for AMP HCI */
#define LOCAL_BR_EDR_CONTROLLER_ID      0

#define BT_EVT_MASK                 0xFF00
#define BT_SUB_EVT_MASK             0x00FF

#define BT_EVT_TO_BTU_HCI_EVT       0x1000      /* HCI Event                        */
#define BT_EVT_TO_BTU_HCI_ACL       0x1100      /* ACL Data from HCI                */
#define BT_EVT_TO_BTU_HCI_SCO       0x1200      /* SCO Data from HCI                */
#define BT_EVT_TO_LM_HCI_CMD        0x2000      /* HCI Command                      */
#define BT_EVT_TO_LM_HCI_ACL        0x2100      /* HCI ACL Data                     */
#define BT_EVT_TO_LM_HCI_SCO        0x2200      /* HCI SCO Data                     */

#define BT_EVT_TO_BTU_L2C_SEG_XMIT  0x1900      /* L2CAP segment(s) transmitted     */

/* Structure returned with Vendor Specific Command complete callback */
typedef struct
{
    uint16_t  opcode;
    uint16_t  param_len;
    uint8_t*  p_param_buf;
} tBTM_VSC_CMPL;
CHECK_SIZE(tBTM_VSC_CMPL, 8);

/* VSC callback function for notifying an application that a synchronous
** BTM function is complete. The pointer contains the address of any returned data.
*/
typedef void (tBTM_VSC_CMPL_CB) (tBTM_VSC_CMPL* p1);

/* General callback function for notifying an application that a synchronous
** BTM function is complete. The pointer contains the address of any returned data.
*/
typedef void (tBTM_CMPL_CB) (void *p1);

/* Timer list entry callback type
*/
typedef void (TIMER_CBACK)(void *p_tle);

/* Define a timer list entry
*/
typedef struct _tle
{
    struct _tle  *p_next;
    struct _tle  *p_prev;
    TIMER_CBACK  *p_cback;
    int32_t       ticks;
    uint32_t      param;
    uint16_t      event;
    uint8_t       in_use;
} TIMER_LIST_ENT;
CHECK_SIZE(TIMER_LIST_ENT, 0x18);

/* Define a timer list queue
*/
typedef struct
{
    TIMER_LIST_ENT   *p_first;
    TIMER_LIST_ENT   *p_last;
    int32_t          last_ticks;
} TIMER_LIST_Q;

typedef struct
{
    // force 32-bit alignment
    uint32_t force_alignment;
    uint8_t _todo[0x44];
} tL2CAP_CFG_INFO;
CHECK_SIZE(tL2CAP_CFG_INFO, 0x48);

/* Structure returned with Rand/Encrypt complete callback */
typedef struct
{
    uint8_t   status;
    uint8_t   param_len;
    uint16_t  opcode;
    uint8_t   param_buf[16];
} tBTM_RAND_ENC;
CHECK_SIZE(tBTM_RAND_ENC, 0x14);

void bdcpy(uint8_t* a, const uint8_t* b);
void* GKI_getbuf(uint32_t size);
void GKI_freebuf(void* p_buf);
void* GKI_getpoolbuf(uint8_t pool_id);
uint8_t GKI_get_taskid(void);
void GKI_send_msg(uint8_t task_id, uint8_t mbox, void *msg);
void utl_freebuf(void **p);
void bta_sys_sendmsg(void* msg);
void bta_sys_stop_timer(TIMER_LIST_ENT* p_tle);
void bta_sys_start_timer(TIMER_LIST_ENT* p_tle, uint16_t type, int32_t timeout);
void bta_hh_snd_write_dev(uint8_t dev_handle, uint8_t t_type, uint8_t param, uint16_t data, uint8_t rpt_id, BT_HDR *p_data);
void BTA_HhSendData(uint8_t dev_handle, uint8_t* dev_bda, BT_HDR *p_buf);
void BTA_HhClose(uint8_t dev_handle);
void BTA_HhAddDev(uint8_t* bda, uint16_t attr_mask, uint8_t sub_class, uint8_t app_id, uint32_t dl_len, uint8_t* dsc_list);
void BTA_HhRemoveDev(uint8_t dev_handle);
uint8_t BTM_ReadRemoteDeviceName(uint8_t* remote_bda, void *p_cb);
uint8_t BTM_WriteStoredLinkKey(uint8_t num_keys, uint8_t *bd_addr, uint8_t *link_key, void *p_cb);
uint8_t* BTM_CheckAdvData(uint8_t* p_adv, uint8_t type, uint8_t* p_length);
void BTM_ReadDevInfo(BD_ADDR remote_bda, uint8_t* p_dev_type, uint8_t* p_addr_type);
uint8_t BTM_VendorSpecificCommand(uint16_t opcode, uint8_t param_len, uint8_t* p_param_buf, tBTM_VSC_CMPL_CB* p_cb);
void BTM_DeviceReset(tBTM_CMPL_CB *p_cb);
void btu_hcif_send_cmd(uint8_t controller_id, BT_HDR* p_buf);
int UUSB_Write(uint8_t type, void* buf, uint16_t len, BT_HDR* p_msg);
