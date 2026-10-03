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
#include "l2c.h"
#include "hci.h"

tL2C_LCB  *l2cu_find_lcb_by_bd_addr(BD_ADDR p_bd_addr);
void l2cu_process_fixed_chnl_resp (tL2C_LCB *p_lcb);
// Huh Wii U doesn't copy p_fcr?
uint8_t l2cu_initialize_fixed_ccb(tL2C_LCB *p_lcb, uint16_t fixed_cid, tL2CAP_FCR_OPTS *p_fcr);
void l2c_enqueue_peer_data(tL2C_CCB *p_ccb, BT_HDR *p_buf);
void l2c_link_check_send_pkts(tL2C_LCB *p_lcb, tL2C_CCB *p_ccb, BT_HDR *p_buf);
void l2cu_no_dynamic_ccbs(tL2C_LCB *p_lcb);
void l2cu_release_ccb(tL2C_CCB *p_ccb);

void l2cble_notify_le_connection (BD_ADDR bda)
{
    tL2C_LCB *p_lcb = l2cu_find_lcb_by_bd_addr (bda);

    if (p_lcb != NULL)
        l2cu_process_fixed_chnl_resp (p_lcb);
}

uint8_t L2CA_RegisterFixedChannel(uint16_t fixed_cid, tL2CAP_FIXED_CHNL_REG *p_freg)
{
    l2cb.fixed_reg[fixed_cid - L2CAP_FIRST_FIXED_CHNL] = *p_freg;
    return (1);
}

uint16_t L2CA_SendFixedChnlData(uint16_t fixed_cid, BD_ADDR rem_bda, BT_HDR *p_buf)
{
    tL2C_LCB        *p_lcb;

    /* We need to have a link up */
    if ((p_lcb = l2cu_find_lcb_by_bd_addr (rem_bda)) == NULL)
    {
        // L2CAP_TRACE_WARNING1 ("L2CA_SendFixedChnlData(0x%04x) - no LCB", fixed_cid);
        return (0);
    }

    if ((p_lcb->peer_chnl_mask[0] & (1 << fixed_cid)) == 0)
    {
        // L2CAP_TRACE_WARNING1 ("L2CA_SendFixedChnlData() - peer does not support fixed chnl: 0x%04x", fixed_cid);
        return (0);
    }

    p_buf->event = 0;
    p_buf->layer_specific = L2CAP_FLUSHABLE_CH_BASED;

    if (!p_lcb->p_fixed_ccbs[fixed_cid - L2CAP_FIRST_FIXED_CHNL])
    {
        if (!l2cu_initialize_fixed_ccb (p_lcb, fixed_cid, &l2cb.fixed_reg[fixed_cid - L2CAP_FIRST_FIXED_CHNL].fixed_chnl_opts))
        {
            // L2CAP_TRACE_WARNING1 ("L2CA_SendFixedChnlData() - no CCB for chnl: 0x%4x", fixed_cid);
            return (0);
        }
    }

    l2c_enqueue_peer_data (p_lcb->p_fixed_ccbs[fixed_cid - L2CAP_FIRST_FIXED_CHNL], p_buf);

    l2c_link_check_send_pkts (p_lcb, NULL, NULL);

    /* If there is no dynamic CCB on the link, restart the idle timer each time something is sent */
    if (p_lcb->in_use && p_lcb->link_state == LST_CONNECTED && !p_lcb->ccb_queue.p_first_ccb)
    {
        l2cu_no_dynamic_ccbs (p_lcb);
    }

    return (!0);
}

uint8_t L2CA_RemoveFixedChnl(uint16_t fixed_cid, BD_ADDR rem_bda)
{
    tL2C_LCB    *p_lcb;
    tL2C_CCB    *p_ccb;

    /* Is a fixed channel connected to the remote BDA ?*/
    p_lcb = l2cu_find_lcb_by_bd_addr (rem_bda);
    if ( ((p_lcb) == NULL) || (!p_lcb->p_fixed_ccbs[fixed_cid - L2CAP_FIRST_FIXED_CHNL]) )
    {
        // L2CAP_TRACE_WARNING3 ("L2CA_RemoveFixedChnl()  CID: 0x%04x  BDA: %08x%04x not connected", fixed_cid,
        //                      (rem_bda[0]<<24)+(rem_bda[1]<<16)+(rem_bda[2]<<8)+rem_bda[3], (rem_bda[4]<<8)+rem_bda[5]);
        return (0);
    }

    // L2CAP_TRACE_API3 ("L2CA_RemoveFixedChnl()  CID: 0x%04x  BDA: %08x%04x", fixed_cid,
    //                   (rem_bda[0]<<24)+(rem_bda[1]<<16)+(rem_bda[2]<<8)+rem_bda[3], (rem_bda[4]<<8)+rem_bda[5]);

    /* Release the CCB, starting an inactivity timeout on the LCB if no other CCBs exist */
    p_ccb = p_lcb->p_fixed_ccbs[fixed_cid - L2CAP_FIRST_FIXED_CHNL];

    p_lcb->p_fixed_ccbs[fixed_cid - L2CAP_FIRST_FIXED_CHNL] = NULL;
    p_lcb->disc_reason = 0x16; // HCI_ERR_CONN_CAUSE_LOCAL_HOST
    l2cu_release_ccb (p_ccb);

    return (1);
}

uint8_t L2CA_UpdateBleConnParams (BD_ADDR rem_bda, uint16_t min_int, uint16_t max_int, uint16_t latency, uint16_t timeout)
{
    tL2C_LCB            *p_lcb;

    /* See if we have a link control block for the remote device */
    p_lcb = l2cu_find_lcb_by_bd_addr (rem_bda);

    /* If we don't have one, create one and accept the connection. */
    if (!p_lcb)
    {
        // L2CAP_TRACE_WARNING2 ("L2CA_UpdateBleConnParams - unknown BD_ADDR %08x%04x",
        //                       (rem_bda[0]<<24)+(rem_bda[1]<<16)+(rem_bda[2]<<8)+rem_bda[3], (rem_bda[4]<<8)+rem_bda[5]);
        return(0);
    }

    if (!p_lcb->is_ble_link)
    {
        // L2CAP_TRACE_WARNING2 ("L2CA_UpdateBleConnParams - BD_ADDR %08x%04x not LE",
        //                       (rem_bda[0]<<24)+(rem_bda[1]<<16)+(rem_bda[2]<<8)+rem_bda[3], (rem_bda[4]<<8)+rem_bda[5]);
        return(0);
    }

    if (p_lcb->link_role == HCI_ROLE_MASTER)
        btsnd_hcic_ble_upd_ll_conn_params (p_lcb->handle, min_int, max_int, latency, timeout, 0, 0);
    else
        // l2cu_send_peer_ble_par_req (p_lcb, min_int, max_int, latency, timeout);
        return (0);

    return(1);
}
