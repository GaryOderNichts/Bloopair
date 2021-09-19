/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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
    map_info.size = 0x6000;
    map_info.domain = 6;            // PAD
    map_info.type = 3;              // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
    map_info.cached = 0xFFFFFFFF;
    _iosMapSharedUserExecution(&map_info);

    // custom ios-pad bss
    map_info.paddr = 0x12159000;
    map_info.vaddr = 0x12159000;
    map_info.size = 0x6000;
    map_info.domain = 6;            // PAD
    map_info.type = 3;              // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read write
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

    // hook HID_HostGetSDPRecord to get device name first 
    *(volatile uint32_t *) 0x11f06e98 = ARM_BL(0x11f06e98, HID_HostGetSDPRecord_hook);

    // ppc smd messages hook
    *(volatile uint32_t *) 0x11f02350 = ARM_BL(0x11f02350, processSmdMessages);
    *(volatile uint32_t *) 0x11f02868 = ARM_BL(0x11f02868, processSmdMessages);
}
