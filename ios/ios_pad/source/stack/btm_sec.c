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

#include <bt_api.h>
#include "device_info.h"
#include "btm.h"
#include "bleext/smp.h"

uint8_t (*const real_btm_sec_execute_procedure)(tBTM_SEC_DEV_REC *p_dev_rec) = (void*) DEFINE_REAL(0x11f14f04, 0xe92d41f0);

uint8_t btm_sec_execute_procedure_hook(tBTM_SEC_DEV_REC *p_dev_rec)
{
    DEBUG_PRINT("btm_sec_execute_procedure_hook\n");

    // make sure the device info has been read
    DeviceInfo_Init();

    DeviceInfo* info = DeviceInfo_Get(p_dev_rec->bd_addr);

    // if this is a paired ds3, bypass security checks
    if (info && info->vendor_id == 0x054c && info->product_id == 0x0268) {
        p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHORIZE | BTM_SEC_IN_AUTHORIZE |
                                        BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_IN_AUTHENTICATE |
                                        BTM_SEC_OUT_ENCRYPT | BTM_SEC_IN_ENCRYPT |
                                        BTM_SEC_FORCE_MASTER | BTM_SEC_ATTEMPT_MASTER |
                                        BTM_SEC_FORCE_SLAVE | BTM_SEC_ATTEMPT_SLAVE);

        DEBUG_PRINT("Security Manager: access granted\n");

        return 0;
    }

    return real_btm_sec_execute_procedure(p_dev_rec);
}

void (*const real_btm_sec_encrypt_change)(uint8_t handle, uint8_t status, uint8_t encr_enable) = (void*) DEFINE_REAL(0x11f165c4, 0xe1a00800);

void btm_sec_encrypt_change_hook(uint8_t handle, uint8_t status, uint8_t encr_enable)
{
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);

    DEBUG_PRINT("btm_sec_encrypt_change_hook 0x%x %d %d\n", handle, status, encr_enable);

    // If this is a BLE device we do our own security stuff
    if (p_dev_rec && p_dev_rec->device_type == BT_DEVICE_TYPE_BLE) {
        // TODO transaction collision?

        if ((status == 0) && encr_enable)
            p_dev_rec->sec_flags |= (BTM_SEC_FLAG_AUTHENTICATED | BTM_SEC_FLAG_ENCRYPTED);

        /* It is possible that we decrypted the link to perform role switch */
        /* mark link not to be encrypted, so that when we execute security next time it will kick in again */
        if ((status == 0) && !encr_enable)
            p_dev_rec->sec_flags &= ~BTM_SEC_FLAG_ENCRYPTED;

        // Notify SMP of encryption change
        SMP_HandleEncryptChange(p_dev_rec->bd_addr, status, encr_enable);
        return;
    }

    return real_btm_sec_encrypt_change(handle, status, encr_enable);
}

static TIMER_LIST_ENT timer;

static void resume_bg_conn_callback(TIMER_LIST_ENT* p_tle)
{
    btm_ble_resume_bg_conn(NULL, 1);
}

void (*const real_btm_sec_disconnected)(uint16_t handle, uint8_t reason) = (void*) DEFINE_REAL(0x11f170ac, 0xe1a00800);
void btm_sec_disconnected_hook(uint16_t handle, uint8_t reason)
{
    DEBUG_PRINT("btm_sec_disconnected_hook %x %x\n", handle, reason);

    real_btm_sec_disconnected(handle, reason);

    // Resume bg_conn if a device is disconnected
    // This is not compiled in due to missing SMP
    // FIXME We do this on a timer due to a race condition in the stack if multiple devices are disconnected at the same time
    //       This is not the cleanest solution, but it might work for now?
    // Edit: I'm not sure if this is actually still necessary since we nopped out the immediate restart in btm_acl_created
    //       but better safe than sorry I guess?
    timer.p_cback = (TIMER_CBACK*) resume_bg_conn_callback;
    bta_sys_start_timer(&timer, 0, 1000);
}
