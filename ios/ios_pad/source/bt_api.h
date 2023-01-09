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

/* Inquiry modes */
#define BTM_GENERAL_INQUIRY         0
#define BTM_LIMITED_INQUIRY         1

/* Inquiry Filter Condition types  */
#define BTM_CLR_INQUIRY_FILTER          0 /* Inquiry Filtering is turned off */
#define BTM_FILTER_COND_DEVICE_CLASS    1 /* Filter on device class */
#define BTM_FILTER_COND_BD_ADDR         2 /* Filter on device addr */

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

/* minor device class field for Peripheral Major Class */
#define BTM_COD_MINOR_GAMEPAD               0x08

/***************************
** major device class field
****************************/
#define BTM_COD_MAJOR_PERIPHERAL            0x05

/* the COD masks */
#define BTM_COD_FORMAT_TYPE_MASK      0x03
#define BTM_COD_MINOR_CLASS_MASK      0xFC
#define BTM_COD_MAJOR_CLASS_MASK      0x1F

/* Search callback events */
#define BTA_DM_INQ_RES_EVT              0       /* Inquiry result for a peer device. */
#define BTA_DM_INQ_CMPL_EVT             1       /* Inquiry complete. */
#define BTA_DM_DISC_RES_EVT             2       /* Discovery result for a peer device. */
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
#define BTA_DM_SP_CFM_REQ_EVT 10 /* Simple Pairing User Confirmation request. */

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

enum
{
    BTA_HH_RPTT_RESRV,      /* reserved         */
    BTA_HH_RPTT_INPUT,      /* input report     */
    BTA_HH_RPTT_OUTPUT,     /* output report    */
    BTA_HH_RPTT_FEATURE     /* feature report   */
};

#define HID_TRANS_SET_REPORT    (5)

/*
** Define structure for Security Device Record.
** A record exists for each device authenticated with this device
*/
#define BTM_SEC_SERVICE_ARRAY_SIZE 3
typedef struct PACKED
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

    uint8_t         sec_bd_name[68];    /* User friendly name of the device. (may be truncated to save space in dev_rec table) */
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

    uint8_t     ble[30];
} tBTM_SEC_DEV_REC;

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
