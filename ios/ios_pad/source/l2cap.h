/*
 *   L2CAP interface of the Bluetooth stack in IOS-PAD (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#pragma once

#include <imports.h>
#include "bt_api.h"

// functions of the Bluetooth stack in IOS-PAD (addresses in imports.sym)
extern uint16_t L2CA_Register(uint16_t psm, const void* p_cb_info);
extern uint16_t L2CA_ConnectReq(uint16_t psm, uint8_t* p_bd_addr);
// answer to an incoming connection (found by its trace text, wraps L2CA_ErtmConnectRsp)
extern uint8_t L2CA_ConnectRsp(uint8_t* p_bd_addr, uint8_t id, uint16_t lcid, uint16_t result, uint16_t status);

#define L2CAP_CONN_NO_RESOURCES 4
extern uint8_t L2CA_ConfigReq(uint16_t cid, void* p_cfg);
extern uint8_t L2CA_ConfigRsp(uint16_t cid, void* p_cfg);
extern uint8_t L2CA_DataWrite(uint16_t cid, BT_HDR* p_data);
extern uint8_t L2CA_DisconnectReq(uint16_t cid);
extern uint8_t L2CA_DisconnectRsp(uint16_t cid);

// tL2CAP_CFG_INFO is 0x48 bytes, only the fields we need are accessed (offsets taken from the HID host)
typedef struct {
    uint8_t raw[0x48];
} __attribute__ ((aligned(4))) L2capCfg;

#define CFG_RESULT(c)           (*(volatile uint16_t*) &(c)->raw[0x00])
#define CFG_MTU_PRESENT(c)      ((c)->raw[0x02])
#define CFG_MTU(c)              (*(volatile uint16_t*) &(c)->raw[0x04])
#define CFG_FLUSH_PRESENT(c)    ((c)->raw[0x20])

// tL2CAP_APPL_INFO, same layout the HID host registers
typedef struct {
    void (*connectInd)(uint8_t* bd_addr, uint32_t lcid, uint32_t psm, uint32_t id);
    void (*connectCfm)(uint32_t lcid, uint32_t result);
    void (*connectPnd)(uint32_t lcid);
    void (*configInd)(uint32_t lcid, L2capCfg* cfg);
    void (*configCfm)(uint32_t lcid, L2capCfg* cfg);
    void (*disconnectInd)(uint32_t lcid, uint32_t ackNeeded);
    void (*disconnectCfm)(uint32_t lcid, uint32_t result);
    void (*qosViolationInd)(uint8_t* bd_addr);
    void (*dataInd)(uint32_t lcid, BT_HDR* p_buf);
    void (*congestionStatus)(uint32_t lcid, uint32_t congested);
    void (*txComplete)(uint32_t lcid, uint32_t numSdu);
} L2capApplInfo;
