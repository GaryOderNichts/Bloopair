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

#include "info_store.h"
#include "controllers.h"

StoredInfo_t stored_infos[BTA_HH_MAX_KNOWN] = { 0 };

int store_update_device_info(int write)
{
    UCSysConfig_t configs[2];
    _memset(configs, 0, sizeof(configs));

    _strncpy(configs[0].name, "slc:btStd", 64);
    configs[0].access = 0x777;
    configs[0].data_type = UC_DATA_TYPE_COMPLEX;

    _strncpy(configs[1].name, "slc:btStd.bloopairInfo", 64);
    configs[1].data_type = UC_DATA_TYPE_BINARY;
    configs[1].data_size = sizeof(stored_infos);
    configs[1].data = &stored_infos;

    int handle = UCOpen();
    if (handle < 0) {
        return handle;
    }

    int res;
    if (write) {
        res = UCWriteSysConfig(handle, 2, configs);
    }
    else {
        res = UCReadSysConfig(handle, 2, configs);
    }
    
    UCClose(handle);

    return res;
}

void store_clear_removed_devices(void)
{
    // read our stored devices
    store_update_device_info(0);

    // create a temporary store
    StoredInfo_t tmp_stored_infos[BTA_HH_MAX_KNOWN];
    _memset(tmp_stored_infos, 0, sizeof(tmp_stored_infos));

    if (bt_db->num_entries > BTA_HH_MAX_KNOWN) {
        return;
    }

    // only add paired devices back to the list
    StoredInfo_t* tmpInfo = tmp_stored_infos;
    for (int i = 0; i < bt_db->num_entries; i++) {
        bt_db_entry_t* entry = &bt_db->entries[i];

        StoredInfo_t* info = store_get_device_info(entry->address);
        if (info) {
            _memcpy(tmpInfo, info, sizeof(StoredInfo_t));
            tmpInfo++;
        }
    }

    // copy and write back infos
    _memcpy(stored_infos, tmp_stored_infos, sizeof(stored_infos));
    store_update_device_info(1);
}

int store_add_name(uint8_t* address, uint8_t* name)
{
    StoredInfo_t* info = store_get_device_info(address);

    if (!info) {
        // look for a free entry
        for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
            if (!stored_infos[i].is_used) {
                info = &stored_infos[i];

                // mark as used any set address
                info->is_used = 1;
                _memcpy(info->address, address, sizeof(BD_ADDR));
                break;
            }
        }
    }

    if (!info) {
        // can't add
        return -1;
    }

    _strncpy((char*) info->name, (char*) name, 64);

    return 0;
}

int store_add_vid_pid(uint8_t* address, uint16_t vendor_id, uint16_t product_id)
{
    StoredInfo_t* info = store_get_device_info(address);

    if (!info) {
        // look for a free entry
        for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
            if (!stored_infos[i].is_used) {
                info = &stored_infos[i];

                // mark as used any set address
                info->is_used = 1;
                _memcpy(info->address, address, sizeof(BD_ADDR));
                break;
            }
        }
    }

    if (!info) {
        // can't add
        return -1;
    }

    info->vendor_id = vendor_id;
    info->product_id = product_id;
    info->is_used = 1;

    return 0;
}

StoredInfo_t* store_get_device_info(uint8_t* address)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (stored_infos[i].is_used &&
            _memcmp(stored_infos[i].address, address, sizeof(BD_ADDR)) == 0) {
            return &stored_infos[i];
        }
    }

    return NULL;
}

int (*const real_wpad_start_clear_device)(void) = (void*) 0x11f40e34;
int wpad_start_clear_device_hook(void)
{
    DEBUG("wpad_start_clear_device\n");

    UCSysConfig_t configs[2];
    _memset(configs, 0, sizeof(configs));

    _strncpy(configs[0].name, "slc:btStd", 64);
    configs[0].access = 0x777;
    configs[0].data_type = UC_DATA_TYPE_COMPLEX;

    _strncpy(configs[1].name, "slc:btStd.bloopairInfo", 64);
    configs[1].data_type = UC_DATA_TYPE_BINARY;

    int handle = UCOpen();
    if (handle >= 0) {
        UCDeleteSysConfig(handle, 2, configs);
        UCClose(handle);
    }

    // clear stored infos in memory as well
    _memset(stored_infos, 0, sizeof(stored_infos));

    return real_wpad_start_clear_device();
}

/*
We need to handle a few things in a seperate thread or else we're blocking other threads,
which will result in crashes. 
This thread will read those requests from a message queue.
This is mostly for parsing big di records which take too long
*/

#define THREAD_STACK_SIZE 1024
#define MESSAGE_BUF_SIZE 8

static uint8_t thread_running = 0;
static void* thread_stack_base = NULL;
static int thread_id = -1;
int info_message_queue = -1;

static void read_DI_record(uint8_t* bda, tSDP_DISCOVERY_DB* db)
{
    uint16_t vendor_id = 0xffff;
    uint16_t product_id = 0xffff;

    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_SERVCLASS_PNP_INFORMATION;
    tSDP_DISC_REC* rec = SDP_FindServiceUUIDInDb(db, &uuid, NULL);
    if (rec) {
        tSDP_DISC_ATTR* attr = SDP_FindAttributeInRec(rec, ATTR_ID_VENDOR_ID);
        if (attr) {
            vendor_id = attr->attr_value.v.u16;
        }

        attr = SDP_FindAttributeInRec(rec, ATTR_ID_PRODUCT_ID);
        if (attr) {
            product_id = attr->attr_value.v.u16;
        }
    }

    DEBUG("got vid %X and pid %X\n", vendor_id, product_id);

    // store the vid and pid
    store_add_vid_pid(bda, vendor_id, product_id);
}

static int info_thread_function(void* arg)
{
    uint32_t message_buf[MESSAGE_BUF_SIZE];
    info_message_queue = IOS_CreateMessageQueue(message_buf, MESSAGE_BUF_SIZE);

    ReportMessage_t* message = NULL;

    while (thread_running) {
        if (IOS_ReceiveMessage(info_message_queue, (uint32_t*) &message, 0) < 0) {
            DEBUG("IOS_ReceiveMessage returned %d\n");
            continue;
        }

        DEBUG("report thread: received message %d\n", message->type);

        switch (message->type) {
        case MESSAGE_TYPE_DI_RECORD: {
            read_DI_record(message->addr, (tSDP_DISCOVERY_DB*) message->data);

            // write info
            store_update_device_info(1);
            break;
        }
        case MESSAGE_TYPE_SAVE_STORE: {
            // write info
            store_update_device_info(1);
            break;
        }
        default:
            break;
        }

        IOS_Free(0xcaff, message);
    }

    return 0;
}

void start_info_thread(void)
{
    if (thread_running) {
        return;
    }

    // clear any devices that are no longer paired from our store and read our store
    store_clear_removed_devices();

    thread_running = 1;

    // allocate a stack
    thread_stack_base = IOS_AllocAligned(0xcaff, THREAD_STACK_SIZE, 0x20);

    // create the thread (priority needs to be lower than or equal the current thread)
    thread_id = IOS_CreateThread(info_thread_function, NULL, (uint32_t*) ((uint8_t*) thread_stack_base + THREAD_STACK_SIZE),
        THREAD_STACK_SIZE, IOS_GetThreadPriority(0), 1);
    
    // start the thread
    IOS_StartThread(thread_id);

    // make sure the thread is in a "waiting for message" state
    usleep(2500);
}

void stop_info_thread(void)
{
    // tell the thread to stop
    thread_running = 0;

    // wait until it finished
    IOS_JoinThread(thread_id, NULL);

    // free stack
    IOS_Free(0xcaff, thread_stack_base);
}

