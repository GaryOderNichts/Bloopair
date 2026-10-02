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
#include "hci.h"
#include "utils.h"

/*---------*/
/* hcidefs */
/*---------*/

#define HCIC_PREAMBLE_SIZE      3

#define HCIC_PARAM_SIZE_ADD_WHITE_LIST          7
#define HCIC_PARAM_SIZE_REMOVE_WHITE_LIST       7
#define HCIC_BLE_RAND_DI_SIZE                   8
#define HCIC_BLE_ENCRYT_KEY_SIZE                16
#define HCIC_PARAM_SIZE_BLE_START_ENC           (4 + HCIC_BLE_RAND_DI_SIZE + HCIC_BLE_ENCRYT_KEY_SIZE)

#define HCI_GRP_BLE_CMDS                (0x08 << 10)
#define HCI_BLE_ADD_WHITE_LIST          (0x0011 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_REMOVE_WHITE_LIST       (0x0012 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_START_ENC               (0x0019 | HCI_GRP_BLE_CMDS)

/*---------*/
/* HCI API */
/*-------- */

uint8_t btsnd_hcic_ble_add_white_list (uint8_t addr_type, BD_ADDR bda)
{
    BT_HDR *p;
    uint8_t *pp;

    if ((p = (BT_HDR *)GKI_getpoolbuf(2)) == NULL)
        return (0);

    pp = (uint8_t *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_ADD_WHITE_LIST;
    p->offset = 0;

    *pp++ = HCI_BLE_ADD_WHITE_LIST & 0xff;
    *pp++ = (HCI_BLE_ADD_WHITE_LIST >> 8) & 0xff;
    *pp++ = HCIC_PARAM_SIZE_ADD_WHITE_LIST;

    *pp++ = addr_type;
    reverseBDA(pp, bda);
    pp += 6;

    btu_hcif_send_cmd (0, p);
    return (1);
}

uint8_t btsnd_hcic_ble_remove_from_white_list (uint8_t addr_type, BD_ADDR bda)
{
    BT_HDR *p;
    uint8_t *pp;

    if ((p = (BT_HDR *)GKI_getpoolbuf(2)) == NULL)
        return (0);

    pp = (uint8_t *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REMOVE_WHITE_LIST;
    p->offset = 0;

    *pp++ = HCI_BLE_REMOVE_WHITE_LIST & 0xff;
    *pp++ = (HCI_BLE_REMOVE_WHITE_LIST >> 8) & 0xff;
    *pp++ = HCIC_PARAM_SIZE_REMOVE_WHITE_LIST;

    *pp++ = addr_type;
    reverseBDA(pp, bda);
    pp += 6;

    btu_hcif_send_cmd (0, p);
    return (1);
}

uint8_t btsnd_hcic_ble_start_enc (uint16_t handle, const uint8_t rand[8], uint16_t ediv, const uint8_t ltk[16])
{
    BT_HDR *p;
    uint8_t *pp;

    if ((p = (BT_HDR *)GKI_getpoolbuf(2)) == NULL)
        return (0);

    pp = (uint8_t *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_BLE_START_ENC;
    p->offset = 0;

    *pp++ = HCI_BLE_START_ENC & 0xff;
    *pp++ = (HCI_BLE_START_ENC >> 8) & 0xff;
    *pp++ = HCIC_PARAM_SIZE_BLE_START_ENC;

    *pp++ = handle & 0xff;
    *pp++ = (handle >> 8) & 0xff;

    for (int i = 0; i < HCIC_BLE_RAND_DI_SIZE; i++) {
        *pp++ = rand[i];
    }

    *pp++ = ediv & 0xff;
    *pp++ = (ediv >> 8) & 0xff;

    for (int i = 0; i < HCIC_BLE_ENCRYT_KEY_SIZE; i++) {
        *pp++ = ltk[i];
    }

    btu_hcif_send_cmd (0, p);
    return (1);
}
