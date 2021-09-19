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

#include "sdp.h"

uint16_t SDP_DiDiscover(uint8_t* remote_device, tSDP_DISCOVERY_DB *p_db, uint32_t len, void *p_cb)
{
    uint16_t  result   = 8;
    uint16_t  num_uuids = 1;
    uint16_t  di_uuid   = UUID_SERVCLASS_PNP_INFORMATION;

    /* build uuid for db init */
    tBT_UUID init_uuid;
    init_uuid.len = 2;
    init_uuid.uu.uuid16 = di_uuid;

    if ( SDP_InitDiscoveryDb(p_db, len, num_uuids, &init_uuid, 0, NULL) )
        if ( SDP_ServiceSearchRequest(remote_device, p_db, p_cb) )
            result = 0;

    return result;
}

uint8_t (*const SDP_InitDiscoveryDb)(tSDP_DISCOVERY_DB *p_db, uint32_t len, uint16_t num_uuid, tBT_UUID *p_uuid_list, uint16_t num_attr, uint16_t *p_attr_list) = (void*) 0x11f2aefc;
uint8_t (*const SDP_ServiceSearchRequest)(uint8_t *p_bd_addr, tSDP_DISCOVERY_DB *p_db, void* p_cb) = (void*) 0x11f2aec8;
uint8_t (*const HID_HostGetSDPRecord)(uint8_t *addr, tSDP_DISCOVERY_DB *p_db, uint32_t db_len, void* p_cb) = (void*) 0x11f1f840;
tSDP_DISC_ATTR* (*const SDP_FindAttributeInRec)(tSDP_DISC_REC *p_rec, uint16_t attr_id) = (void*) 0x11f2a6dc;
tSDP_DISC_REC* (*const SDP_FindServiceUUIDInDb)(tSDP_DISCOVERY_DB *p_db, tBT_UUID *p_uuid, tSDP_DISC_REC *p_start_rec) = (void*) 0x11f2ada0;
