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
#include "bt_api.h"

/* HCI role defenitions */
#define HCI_ROLE_MASTER                 0x00
#define HCI_ROLE_SLAVE                  0x01
#define HCI_ROLE_UNKNOWN                0xff

// extern
uint8_t btsnd_hcic_ble_upd_ll_conn_params (uint16_t handle,
                                           uint16_t conn_int_min, uint16_t conn_int_max,
                                           uint16_t conn_latency, uint16_t conn_timeout,
                                           uint16_t min_ce_len, uint16_t max_ce_len);

// extern
uint8_t btsnd_hcic_ble_set_scan_enable(uint8_t scan_enable, uint8_t duplicate);

// extern
uint8_t btsnd_hcic_ble_rand(void (*p_cmd_cplt_cback)(tBTM_RAND_ENC* p1));

uint8_t btsnd_hcic_ble_add_white_list (uint8_t addr_type, BD_ADDR bda);

uint8_t btsnd_hcic_ble_remove_from_white_list (uint8_t addr_type, BD_ADDR bda);

uint8_t btsnd_hcic_ble_start_enc (uint16_t handle, const uint8_t rand[8], uint16_t ediv, const uint8_t ltk[16]);
