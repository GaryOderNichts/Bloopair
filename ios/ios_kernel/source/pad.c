/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pad.h"
#include "imports.h"
#include "../../ios_pad/ios_pad_syms.h"

void run_ios_pad_patches(void)
{
    // map memory for our custom ios-pad code
    // custom ios-pad text
    ios_map_shared_info_t map_info;
    map_info.paddr = 0x11F86000;
    map_info.vaddr = 0x11F86000;
    map_info.size = 0x10000;         // Can map up to 0x11FC0000, 0x10000 should be enough for now
    map_info.domain = 6;            // PAD
    map_info.type = 3;              // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
    map_info.cached = 0xFFFFFFFF;
    _iosMapSharedUserExecution(&map_info);

    // custom ios-pad bss
    map_info.paddr = 0x12159000;
    map_info.vaddr = 0x12159000;
    map_info.size = 0x6000;         // Can map up to 0x12300000, 0x6000 should be enough for now
    map_info.domain = 6;            // PAD
    map_info.type = 3;              // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
    map_info.cached = 0xFFFFFFFF;
    _iosMapSharedUserExecution(&map_info);

    // IOS-PAD needs SD access for reading and storing pairings, packetlogging and romdumping
    setClientCapabilities(6, 0xb, 0xffffffffffffffffllu);

    // security callback hook
    *(volatile uint32_t *) 0x1214d3c4 = bta_sec_callback;

    // search callback hook
    *(volatile uint32_t *) 0x11f3f278 = bta_search_callback;

    // hid event hook
    *(volatile uint32_t *) 0x1214d93c = bta_hh_event;

    // hid open hook
    *(volatile uint32_t *) 0x11fc1f84 = bta_hh_open_act;

    // hid data hook
    *(volatile uint32_t *) 0x11f06af0 = ARM_BL(0x11f06af0, bta_hh_co_data);

    // hook BTA_DmSearch so we can edit it's params
    *(volatile uint32_t *) 0x11f3e998 = ARM_BL(0x11f3e998, BTA_DmSearch_hook);

    // the Wii U doesn't read the DI record by default so we don't have the vid and pid
    // so patch start sdp to read the vid and pid and store it in the custom info store
    *(volatile uint32_t *) 0x11f06e98 = ARM_BL(0x11f06e98, SDP_DiDiscover);
    *(volatile uint32_t *) 0x11f06ef8 = bta_hh_di_sdp_cback;

    // hook writeDevInfo so we can write our custom data too
    *(volatile uint32_t *) 0x11f4181c = ARM_B(0x11f4181c, writeDevInfo_hook);
    *(volatile uint32_t *) 0x11f411fc = ARM_B(0x11f411fc, purgeDevInfo_hook);

    // ppc smd messages hook
    *(volatile uint32_t *) 0x11f01a10 = ARM_B(0x11f01a10, processSmdMessages);

    // hook security procedures
    *(volatile uint32_t *) 0x11f14f00 = ARM_B(0x11f14f00, btm_sec_execute_procedure_hook);

    // hook btrm lib handling so we can have custom ipc calls
    *(volatile uint32_t *) 0x11f03428 = ARM_B(0x11f03428, _btrmCustomLibHook);

    // BLE discoverable check
    *(volatile uint32_t *) 0x11f0cee4 = ARM_BL(0x11f0cee4, btm_ble_is_discoverable_hook);

    // Fix N's HID handle mixup, will cause issues with our BLE patches otherwise
    *(volatile uint32_t *) 0x11f40814 = 0xe1a02001; // mov r2, r1

    // Hook BTA_Hh API to redirect to our custom BLE API
    *(volatile uint32_t *) 0x11f077f4 = ARM_B(0x11f077f4, BTA_HhOpen_hook);
    *(volatile uint32_t *) 0x11f07634 = ARM_B(0x11f07634, BTA_HhClose_hook);
    *(volatile uint32_t *) 0x11f07678 = ARM_B(0x11f07678, BTA_HhAddDev_hook);
    *(volatile uint32_t *) 0x11f075e8 = ARM_B(0x11f075e8, BTA_HhRemoveDev_hook);

    // Hook encryption event for BLE
    *(volatile uint32_t *) 0x11f165c0 = ARM_B(0x11f165c0, btm_sec_encrypt_change_hook);

    // Nop out BLE log spam
    *(volatile uint32_t *) 0x11f1b890 = 0xe1a00000; // mov r0, r0
    *(volatile uint32_t *) 0x11f1ba98 = 0xe1a00000; // mov r0, r0

    // Don't call btsnd_hcic_ble_read_remote_feat, to avoid getting the controller
    // stuck if a PDU is immediately sent after establishing a connection
    // The event parsing is broken anyways in the stack lol
    *(volatile uint32_t *) 0x11f0b940 = 0xe12fff1e; // bx lr

    // We need to hook disconnections to check for auto connection resume, since this isn't
    // done without SMP compiled in
    *(volatile uint32_t *) 0x11f170a8 = ARM_B(0x11f170a8, btm_sec_disconnected_hook);

    // Nop out the immediate btm_ble_resume_bg_conn upon connection established
    // It can cause some weird recursive spiral? We call it ourself once a connection is actually established
    *(volatile uint32_t *) 0x11f0b930 = 0xe1a00000; // mov r0, r0

    // Hooks for BLE device info management
    *(volatile uint32_t *) 0x11f3f290 = write_link_key_hook;
    *(volatile uint32_t *) 0x11f04d88 = ARM_B(0x11f04d88, BTA_DmAddDevice_hook);
    *(volatile uint32_t *) 0x11f42430 = ARM_B(0x11f42430, WUDiRemoveDevice_hook);

    // Fix the HCI fragmentation logic and support BLE fragmentation
    *(volatile uint32_t *) 0x11f3b860 = ARM_B(0x11f3b860, hci_h4_send_msg_hook);

// #define PACKETLOGGER
#ifdef PACKETLOGGER
    *(volatile uint32_t *) 0x11f2fe90 = ARM_B(0x11f2fe90, data_ind_hook);
    *(volatile uint32_t *) 0x11f3ba18 = ARM_B(0x11f3ba18, uusb_transfer_hook);
#endif

// #define MORE_LOGS
#ifdef MORE_LOGS
/******************************************************************************
**
** Trace Levels
**
** The following values may be used for different levels:
**      BT_TRACE_LEVEL_NONE    0        * No trace messages to be generated
**      BT_TRACE_LEVEL_ERROR   1        * Error condition trace messages
**      BT_TRACE_LEVEL_WARNING 2        * Warning condition trace messages
**      BT_TRACE_LEVEL_API     3        * API traces
**      BT_TRACE_LEVEL_EVENT   4        * Debug messages for events
**      BT_TRACE_LEVEL_DEBUG   5        * Debug messages (general)
******************************************************************************/
#define TRACE_LEVEL 5

    // bta_sys_cfg.trace_level
    *(volatile uint8_t *) 0x11fca8d4 = TRACE_LEVEL;

    // appl_trace_level
    *(volatile uint8_t *) 0x120fd085 = TRACE_LEVEL;

    // btm_cb.trace_level
    *(volatile uint8_t *) 0x12150f88 = TRACE_LEVEL;

    // hh_cb.trace_level
    *(volatile uint8_t *) 0x12151475 = TRACE_LEVEL;

    // l2cb.l2cap_trace_level
    *(volatile uint8_t *) 0x12151478 = TRACE_LEVEL;

    // sdp_cb.trace_level
    *(volatile uint8_t *) 0x12155454 = TRACE_LEVEL;
#endif
}
