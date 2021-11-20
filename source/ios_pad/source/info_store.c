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

#include "info_store.h"
#include "controllers.h"

bt_devInfo_t* bt_devInfo = (bt_devInfo_t*) 0x12157778;

StoredInfo_t stored_infos[BTA_HH_MAX_KNOWN] = { 0 };

StoredInfo_t* store_get_device_info(uint8_t* address)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (stored_infos[i].magic != MAGIC_EMPTY &&
            memcmp(stored_infos[i].address, address, BD_ADDR_LEN) == 0) {
            return &stored_infos[i];
        }
    }

    return NULL;
}

StoredInfo_t* store_allocate_device_info(uint8_t* address)
{
    // look for a free entry
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (stored_infos[i].magic == MAGIC_EMPTY) {
            stored_infos[i].magic = MAGIC_UNKNOWN;
            memcpy(stored_infos[i].address, address, BD_ADDR_LEN);
            return &stored_infos[i];
        }
    }

    return NULL;
}

void readDevInfo(void)
{
    for (int i = 0; i < bt_devInfo->num_entries; i++) {
        bt_devInfo_entry_t* entry = &bt_devInfo->entries[i];

        StoredInfo_t* info = store_get_device_info(entry->address);
        if (!info) {
            info = store_allocate_device_info(entry->address);
            if (!info) {
                break;
            }
        }

        if (entry->magic == MAGIC_EMPTY) {
            info->magic = MAGIC_UNKNOWN;
        }
        else {
            info->magic = entry->magic;
            info->vendor_id = entry->vendor_id;
            info->product_id = entry->product_id;
        }
    }
}

// executable memory which gets populated by the kernel
uint32_t __writeDevInfo_hook_buf[3] __attribute__ ((section (".fn_hook_bufs")));
int (*const real_writeDevInfo)(void* callback) = (void*) __writeDevInfo_hook_buf;
int writeDevInfo_hook(void* callback)
{
    DEBUG("writeDevInfo_hook %p\n", callback);

    // populate devInfo with our custom info before writing
    for (int i = 0; i < bt_devInfo->num_entries; i++) {
        bt_devInfo_entry_t* entry = &bt_devInfo->entries[i];

        StoredInfo_t* info = store_get_device_info(entry->address);
        if (!info) {
            // we don't have info for this entry, skip it
            continue;
        }

        entry->magic = info->magic;
        entry->vendor_id = info->vendor_id;
        entry->product_id = info->product_id;
    }

    return real_writeDevInfo(callback);
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
    StoredInfo_t* info = store_get_device_info(bda);
    if (!info) {
        info = store_allocate_device_info(bda);
    }

    info->vendor_id = vendor_id;
    info->product_id = product_id;
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

    // read dev info and add it to our stored infos
    readDevInfo();

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
