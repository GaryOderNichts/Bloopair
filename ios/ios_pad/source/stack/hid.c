/******************************************************************************
 *
 *  Copyright (C) 2002-2012 Broadcom Corporation
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
#include "hid.h"

uint8_t HID_HostRemoveDevNoClose(uint8_t dev_handle)
{
    // if( !hh_cb.reg_flag )
    //     return (HID_ERR_NOT_REGISTERED);

    if( (dev_handle >= HID_HOST_MAX_DEVICES) || (!hh_cb.devices[dev_handle].in_use) )
        return 5;

    // HID_HostCloseDev( dev_handle ) ;
    hh_cb.devices[dev_handle].in_use = 0;
    hh_cb.devices[dev_handle].conn.conn_state = 0;
    hh_cb.devices[dev_handle].conn.ctrl_cid = hh_cb.devices[dev_handle].conn.intr_cid = 0;

    return 0;
}
