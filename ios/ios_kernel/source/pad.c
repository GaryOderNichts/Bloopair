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
    map_info.size = 0x6000;         // Can map up to 0x11FC0000, 0x6000 should be enough for now
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

    // security callback hook
    *(volatile uint32_t *) 0x1214d3c4 = bta_sec_callback;

    // search callback hook
    *(volatile uint32_t *) 0x11f3f278 = bta_search_callback;

    // hid event hook
    *(volatile uint32_t *) 0x1214d93c = bta_hh_event;

    // hid open hook
    *(volatile uint32_t *) 0x11fc1f84 = bta_hh_open_act;

    // hid disable hook
    *(volatile uint32_t *) 0x11f07db0 = ARM_BL(0x11f07db0, bta_hh_api_disable);

    // hid data hook
    *(volatile uint32_t *) 0x11f06af0 = ARM_BL(0x11f06af0, bta_hh_co_data);

    // hook BTA_DmSearch so we can edit it's params
    *(volatile uint32_t *) 0x11f3e998 = ARM_BL(0x11f3e998, BTA_DmSearch_hook);

    // the Wii U doesn't read the DI record by default so we don't have the vid and pid
    // so patch start sdp to read the vid and pid and store it in the custom info store
    *(volatile uint32_t *) 0x11f06e98 = ARM_BL(0x11f06e98, SDP_DiDiscover);
    *(volatile uint32_t *) 0x11f06ef8 = bta_hh_di_sdp_callback;

    // hook writeDevInfo so we can write our custom data too
    *(volatile uint32_t *) 0x11f4181c = ARM_B(0x11f4181c, writeDevInfo_hook);

    // ppc smd messages hook
    *(volatile uint32_t *) 0x11f01a10 = ARM_B(0x11f01a10, processSmdMessages);

    // hook security procedures
    *(volatile uint32_t *) 0x11f14f00 = ARM_B(0x11f14f00, btm_sec_execute_procedure_hook);

    // hook btrm lib handling so we can have custom ipc calls
    *(volatile uint32_t *) 0x11f03428 = ARM_B(0x11f03428, _btrmCustomLibHook);

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
