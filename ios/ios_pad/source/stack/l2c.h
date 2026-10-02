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

// Define values and structs based on RE and broadcom headers
#define L2CAP_NUM_FIXED_CHNLS 4
#define L2CAP_FIXED_CHNL_ARRAY_SIZE 8
#define L2CAP_NUM_CHNL_PRIORITY 3
#define MAX_L2CAP_LINKS 7
#define MAX_L2CAP_CHANNELS 14
#define MAX_L2CAP_CLIENTS 8

#define LST_CONNECTED 4

/* L2CAP Predefined CIDs  (0x0004-0x003E Reserved)
*/
#define L2CAP_ATT_CID                   4
#define L2CAP_SMP_CID                   6

/* First fixed channel supported */
#ifndef L2CAP_FIRST_FIXED_CHNL
#define L2CAP_FIRST_FIXED_CHNL              3
#endif

/* Data Packet Flags  (bits 2-15 are reserved) */
/* layer specific 14-15 bits are used for FCR SAR */
#define L2CAP_FLUSHABLE_MASK        0x0003
#define L2CAP_FLUSHABLE_CH_BASED    0x0000
#define L2CAP_FLUSHABLE_PKT         0x0001
#define L2CAP_NON_FLUSHABLE_PKT     0x0002

typedef struct
{
#define L2CAP_FCR_BASIC_MODE    0x00
#define L2CAP_FCR_ERTM_MODE     0x03
#define L2CAP_FCR_STREAM_MODE   0x04

    uint8_t  mode;

    uint8_t  tx_win_sz;
    uint8_t  max_transmit;
    uint16_t rtrans_tout;
    uint16_t mon_tout;
    uint16_t mps;
} tL2CAP_FCR_OPTS;
CHECK_SIZE(tL2CAP_FCR_OPTS, 0xa);

/* Fixed channel connected and disconnected. Parameters are
**      BD Address of remote
**      TRUE if channel is connected, FALSE if disconnected
**      Reason for connection failure
*/
typedef void (tL2CA_FIXED_CHNL_CB) (BD_ADDR, uint8_t, uint16_t);

/* Signalling data received. Parameters are
**      BD Address of remote
**      Pointer to buffer with data
*/
typedef void (tL2CA_FIXED_DATA_CB) (BD_ADDR, BT_HDR *);

typedef struct
{
    tL2CA_FIXED_CHNL_CB    *pL2CA_FixedConn_Cb;
    tL2CA_FIXED_DATA_CB    *pL2CA_FixedData_Cb;
    tL2CAP_FCR_OPTS         fixed_chnl_opts;

    uint16_t                default_idle_tout;
} tL2CAP_FIXED_CHNL_REG;
CHECK_SIZE(tL2CAP_FIXED_CHNL_REG, 0x14);

typedef struct
{
    uint8_t                 in_use;
    uint16_t                psm;
    uint16_t                real_psm;               /* This may be a dummy RCB for an o/b connection but */
    uint8_t                 _todo[46];
} tL2C_RCB;
CHECK_SIZE(tL2C_RCB, 0x34);

typedef struct t_l2c_ccb
{
    uint8_t             in_use;                 /* TRUE when in use, FALSE when not */
    int     chnl_state;             /* Channel state                    */

    struct t_l2c_ccb    *p_next_ccb;            /* Next CCB in the chain            */
    struct t_l2c_ccb    *p_prev_ccb;            /* Previous CCB in the chain        */
    struct t_l2c_linkcb *p_lcb;                 /* Link this CCB is assigned to     */

    uint16_t              local_cid;              /* Local CID                        */
    uint16_t              remote_cid;             /* Remote CID                       */

    TIMER_LIST_ENT      timer_entry;            /* CCB Timer List Entry             */

    tL2C_RCB            *p_rcb;                 /* Registration CB for this Channel */

#define IB_CFG_DONE     0x01
#define OB_CFG_DONE     0x02
#define RECONFIG_FLAG   0x04                    /* True after initial configuration */
#define CFG_DONE_MASK   (IB_CFG_DONE | OB_CFG_DONE)

    uint8_t               config_done;            /* Configuration flag word         */
    uint8_t               local_id;               /* Transaction ID for local trans  */
    uint8_t               remote_id;              /* Transaction ID for local  */

#define CCB_FLAG_NO_RETRY       0x01            /* no more retry */
#define CCB_FLAG_SENT_PENDING   0x02            /* already sent pending response */
    uint8_t               flags;

    tL2CAP_CFG_INFO     our_cfg;                /* Our saved configuration options    */
    uint16_t  peer_cfg_bits;          /* Store what peer wants to configure */
    tL2CAP_CFG_INFO     peer_cfg;               /* Peer's saved configuration options */

    BUFFER_Q            xmit_hold_q;            /* Transmit data hold queue         */

    uint8_t             cong_sent;              /* Set when congested status sent   */
    uint16_t              buff_quota;             /* Buffer quota before sending congestion   */

    uint8_t ccb_priority;          /* Channel priority                 */
    uint8_t tx_data_rate;         /* Channel Tx data rate             */
    uint8_t rx_data_rate;         /* Channel Rx data rate             */

    uint8_t             is_flushable;                   /* TRUE if channel is flushable     */

    uint16_t              fixed_chnl_idle_tout;   /* Idle timeout to use for the fixed channel       */
} tL2C_CCB;
CHECK_SIZE(tL2C_CCB, 0xe4);

/***********************************************************************
** Define a queue of linked CCBs.
*/
typedef struct
{
    tL2C_CCB        *p_first_ccb;               /* The first channel in this queue */
    tL2C_CCB        *p_last_ccb;                /* The last  channel in this queue */
} tL2C_CCB_Q;
CHECK_SIZE(tL2C_CCB_Q, 0x8);

typedef struct
{
    tL2C_CCB        *p_serve_ccb;               /* current serving ccb within priority group */
    tL2C_CCB        *p_first_ccb;               /* first ccb of priority group */
    uint8_t           num_ccb;                    /* number of channels in priority group */
    uint8_t           quota;                      /* burst transmission quota */
} tL2C_RR_SERV;
CHECK_SIZE(tL2C_RR_SERV, 0xc);

/* Define a link control block. There is one link control block between
** this device and any other device (i.e. BD ADDR).
*/
typedef struct t_l2c_linkcb
{
    uint8_t             in_use;                     /* TRUE when in use, FALSE when not */
    int     link_state;

    TIMER_LIST_ENT      timer_entry;                /* Timer list entry for timeout evt */
    uint16_t              handle;                     /* The handle used with LM          */

    tL2C_CCB_Q          ccb_queue;                  /* Queue of CCBs on this LCB        */

    tL2C_CCB            *p_pending_ccb;             /* ccb of waiting channel during link disconnect */
    TIMER_LIST_ENT      info_timer_entry;           /* Timer entry for info resp timeout evt */
    BD_ADDR             remote_bd_addr;             /* The BD address of the remote     */

    uint8_t               link_role;                  /* Master or slave                  */
    uint8_t               id;
    void   *p_echo_rsp_cb;             /* Echo response callback           */
    uint16_t              idle_timeout;               /* Idle timeout                     */
    uint8_t             is_bonding;                 /* True - link active only for bonding */

    uint16_t              link_flush_tout;            /* Flush timeout used               */

    uint16_t              link_xmit_quota;            /* Num outstanding pkts allowed     */
    uint16_t              sent_not_acked;             /* Num packets sent but not acked   */

    //wiiu-edit: what?
    uint8_t _unk[6];

    uint8_t             partial_segment_being_sent; /* Set TRUE when a partial segment  */
                                                    /* is being sent.                   */
    uint8_t             w4_info_rsp;                /* TRUE when info request is active */
    uint8_t               info_rx_bits;               /* set 1 if received info type */
    uint32_t              peer_ext_fea;               /* Peer's extended features mask    */
    BUFFER_Q            link_xmit_data_q;           /* Transmit data buffer queue       */

    uint8_t               peer_chnl_mask[L2CAP_FIXED_CHNL_ARRAY_SIZE];

    BT_HDR              *p_hcit_rcv_acl;            /* Current HCIT ACL buf being rcvd  */
    uint16_t              idle_timeout_sv;            /* Save current Idle timeout        */
    uint8_t               acl_priority;               /* L2C_PRIORITY_NORMAL or L2C_PRIORITY_HIGH */
    void       *p_nocp_cb;                 /* Num Cmpl pkts callback           */

#if (L2CAP_NUM_FIXED_CHNLS > 0)
    tL2C_CCB            *p_fixed_ccbs[L2CAP_NUM_FIXED_CHNLS];
    uint16_t              disc_reason;
#endif

    uint8_t             is_ble_link;

#if 0 // wiiu-edit: snip
    uint8_t             ble_addr_type;

#define UPD_ENABLED     0  /* If peer requests update, we will change params */
#define UPD_DISABLED    1  /* application requested not to update */
#define UPD_PENDING     2  /* while updates are disabled, peer requested new parameters */
#define UPD_UPDATED     3  /* peer updated connection parameters */
    uint8_t               upd_disabled;

    uint16_t              min_interval; /* parameters as requested by peripheral */
    uint16_t              max_interval;
    uint16_t              latency;
    uint16_t              timeout;
#endif

    /* each priority group is limited burst transmission  */
    /* round robin service for the same priority channels */
    tL2C_RR_SERV          rr_serv[L2CAP_NUM_CHNL_PRIORITY];
    uint8_t               rr_pri;                             /* current serving priority group */

} tL2C_LCB;
CHECK_OFFSET(tL2C_LCB, 0x5c, sent_not_acked);
CHECK_OFFSET(tL2C_LCB, 0x64, partial_segment_being_sent);
CHECK_OFFSET(tL2C_LCB, 0x6c, link_xmit_data_q);
CHECK_OFFSET(tL2C_LCB, 0x9e, is_ble_link);
CHECK_SIZE(tL2C_LCB, 0xc8);

/* Define the L2CAP control structure
*/
typedef struct
{
    uint8_t           l2cap_trace_level;
    uint16_t          controller_xmit_window;         /* Total ACL window for all links   */

    uint16_t          round_robin_quota;              /* Round-robin link quota           */
    uint16_t          round_robin_unacked;            /* Round-robin unacked              */
    uint8_t         check_round_robin;              /* Do a round robin check           */

    uint8_t         is_cong_cback_context;

    tL2C_LCB        lcb_pool[MAX_L2CAP_LINKS];      /* Link Control Block pool          */
    tL2C_CCB        ccb_pool[MAX_L2CAP_CHANNELS];   /* Channel Control Block pool       */
    tL2C_RCB        rcb_pool[MAX_L2CAP_CLIENTS];    /* Registration info pool           */

    tL2C_CCB        *p_free_ccb_first;              /* Pointer to first free CCB        */
    tL2C_CCB        *p_free_ccb_last;               /* Pointer to last  free CCB        */

    uint8_t           desire_role;                    /* desire to be master/slave when accepting a connection */
    uint8_t         disallow_switch;                /* FALSE, to allow switch at create conn */
    uint16_t          num_lm_acl_bufs;                /* # of ACL buffers on controller   */
    uint16_t          idle_timeout;                   /* Idle timeout                     */

    BUFFER_Q        rcv_hold_q;                     /* Recv pending queue               */
    TIMER_LIST_ENT  rcv_hold_tle;                   /* Timer list entry for rcv hold    */

    tL2C_LCB        *p_cur_hcit_lcb;                /* Current HCI Transport buffer     */
    uint16_t          num_links_active;               /* Number of links active           */

    uint16_t          non_flushable_pbf;              /* L2CAP_PKT_START_NON_FLUSHABLE if controller supports */
                                                    /* Otherwise, L2CAP_PKT_START */
    uint8_t         is_flush_active;                /* TRUE if an HCI_Enhanced_Flush has been sent */


#if (L2CAP_NUM_FIXED_CHNLS > 0)
    tL2CAP_FIXED_CHNL_REG   fixed_reg[L2CAP_NUM_FIXED_CHNLS];   /* Reg info for fixed channels */
#endif

    uint8_t                  is_ble_connecting;
    BD_ADDR                  ble_connecting_bda;
    uint16_t                   controller_le_xmit_window;         /* Total ACL window for all links   */
    uint16_t                   num_lm_ble_bufs;                   /* # of ACL buffers on controller   */

#if 0 // wiiu-edit: snip
    void      *p_echo_data_cb;                /* Echo data callback */
#endif

    uint16_t          dyn_psm;
} tL2C_CB;
CHECK_OFFSET(tL2C_CB, 0xc, lcb_pool);
CHECK_OFFSET(tL2C_CB, 0x584, ccb_pool);
CHECK_OFFSET(tL2C_CB, 0x11fc, rcb_pool);
CHECK_OFFSET(tL2C_CB, 0x139c, p_free_ccb_first);
CHECK_OFFSET(tL2C_CB, 0x13dc, fixed_reg);
CHECK_SIZE(tL2C_CB, 0x143c);

extern tL2C_CB l2cb;

void l2cble_notify_le_connection (BD_ADDR bda);

uint8_t L2CA_RegisterFixedChannel(uint16_t fixed_cid, tL2CAP_FIXED_CHNL_REG *p_freg);

// extern
uint8_t L2CA_ConnectFixedChnl(uint16_t fixed_cid, BD_ADDR rem_bda);

uint16_t L2CA_SendFixedChnlData(uint16_t fixed_cid, BD_ADDR rem_bda, BT_HDR *p_buf);

uint8_t L2CA_RemoveFixedChnl(uint16_t fixed_cid, BD_ADDR rem_bda);

// extern
uint8_t L2CA_CancelBleConnectReq(BD_ADDR rem_bda);

uint8_t L2CA_UpdateBleConnParams (BD_ADDR rem_bda, uint16_t min_int, uint16_t max_int, uint16_t latency, uint16_t timeout);
