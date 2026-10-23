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
