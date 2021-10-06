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

#include <bt_api.h>
#include "info_store.h"

// executable memory which gets populated by the kernel
uint32_t __btm_sec_execute_procedure_hook_buf[3] __attribute__ ((section (".fn_hook_bufs")));
uint8_t (*const real_btm_sec_execute_procedure)(tBTM_SEC_DEV_REC *p_dev_rec) = (void*) __btm_sec_execute_procedure_hook_buf;

uint8_t btm_sec_execute_procedure_hook(tBTM_SEC_DEV_REC *p_dev_rec)
{
    DEBUG("btm_sec_execute_procedure_hook\n");

    // make sure the info thread is running so persisted device info has been read
    start_info_thread();

    StoredInfo_t* info = store_get_device_info(p_dev_rec->bd_addr);

    // if this is a paired ds3, bypass security checks
    if (info && info->vendor_id == 0x054c && info->product_id == 0x0268) {
        p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHORIZE | BTM_SEC_IN_AUTHORIZE |
                                        BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_IN_AUTHENTICATE |
                                        BTM_SEC_OUT_ENCRYPT | BTM_SEC_IN_ENCRYPT |
                                        BTM_SEC_FORCE_MASTER | BTM_SEC_ATTEMPT_MASTER |
                                        BTM_SEC_FORCE_SLAVE | BTM_SEC_ATTEMPT_SLAVE);

        DEBUG("Security Manager: access granted\n");

        return 0;
    }

    return real_btm_sec_execute_procedure(p_dev_rec);
}
