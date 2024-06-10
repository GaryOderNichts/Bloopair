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

static BT_DevInfo* bt_devInfo = (BT_DevInfo*) 0x12157778;

static StoredInfo stored_infos[BTA_HH_MAX_KNOWN] = { 0 };

static int devInfo_read = 0;

void store_read_device_info(void)
{
    // we only need to do this once, after that bloopair keeps track of device info
    if (devInfo_read) {
        return;
    }
    devInfo_read = 1;

    for (int i = 0; i < bt_devInfo->num_entries; i++) {
        BT_DevInfo_Entry* entry = &bt_devInfo->entries[i];

        StoredInfo* info = store_get_device_info(entry->address);
        if (!info) {
            info = store_allocate_device_info(entry->address);
            if (!info) {
                break;
            }
        }

        if (entry->magic == MAGIC_EMPTY) {
            info->magic = MAGIC_UNKNOWN;
        } else {
            info->magic = entry->magic;
            info->vendor_id = entry->vendor_id;
            info->product_id = entry->product_id;
        }
    }
}


StoredInfo* store_get_device_info(uint8_t* address)
{
    for (int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (stored_infos[i].magic != MAGIC_EMPTY &&
            memcmp(stored_infos[i].address, address, BD_ADDR_LEN) == 0) {
            return &stored_infos[i];
        }
    }

    return NULL;
}

StoredInfo* store_allocate_device_info(uint8_t* address)
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

int (*const real_writeDevInfo)(void* callback) = (void*) DEFINE_REAL(0x11f41820, 0xe92d4ff0);
int writeDevInfo_hook(void* callback)
{
    DEBUG_PRINT("writeDevInfo_hook %p\n", callback);

    // populate devInfo with our custom info before writing
    for (int i = 0; i < bt_devInfo->num_entries; i++) {
        BT_DevInfo_Entry* entry = &bt_devInfo->entries[i];

        StoredInfo* info = store_get_device_info(entry->address);
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

void store_read_DI_record(uint8_t* bda, tSDP_DISCOVERY_DB* db)
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

    DEBUG_PRINT("got vid %X and pid %X\n", vendor_id, product_id);

    // store the vid and pid
    StoredInfo* info = store_get_device_info(bda);
    if (!info) {
        info = store_allocate_device_info(bda);
    }

    info->vendor_id = vendor_id;
    info->product_id = product_id;
}
