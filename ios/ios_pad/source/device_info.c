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

#include "device_info.h"
#include "bta/bta_api.h"
#include "controllers.h"
#include "fsa.h"
#include "main.h"
#include "stack/btm.h"
#include "uc.h"
#include "wud.h"

// We allow for more infos than WUD, but we filter against WUD when storing
#define MAX_DEVICE_INFO_COUNT 32

static BT_DevInfo* bt_devInfo = (BT_DevInfo*) 0x12157778;

static int device_info_semaphore = -1;
static DeviceInfo device_infos[MAX_DEVICE_INFO_COUNT] = { 0 };

static int devInfo_read = 0;

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    // Only support uppercase characters to prevent potential duplicates
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

uint8_t filename_to_bda(const char* filename, BD_ADDR out)
{
    if (strnlen(filename, 256) != 16) {
        return 0;
    }

    if (strncmp(filename + 12, ".bin", sizeof(".bin")) != 0) {
        return 0;
    }

    for (int i = 0; i < 6; i++) {
        int hi = hex_nibble(filename[i * 2]);
        int lo = hex_nibble(filename[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            return 0;
        }

        out[i] = (uint8_t) ((hi << 4) | lo);
    }

    return 1;
}

static void _DeviceInfo_ParseBTDB(void)
{
    // Parse entires from the official btdb.
    // This gives us info about official devices and allows migrating
    // old pairings which are not on the SD Card yet
    for (int i = 0; i < bt_devInfo->num_entries; i++) {
        BT_DevInfo_Entry* entry = &bt_devInfo->entries[i];

        DeviceInfo* info = DeviceInfo_GetOrAllocate(entry->address);
        if (!info) {
            break;
        }

        if (entry->magic == MAGIC_EMPTY) {
            // Assume empty magic pairings in the btdb are always official pairings
            info->magic = MAGIC_OFFICIAL;
        } else {
            // Old pairing, let's migrate it
            info->magic = entry->magic;
            info->vendor_id = entry->vendor_id;
            info->product_id = entry->product_id;
            info->flush = 1;

            // Take link key from WUD
            WUDDevInfo* dev = WUDiGetDevInfo(entry->address);
            if (dev) {
                memcpy(info->classic.link_key, dev->link_key, 16);
            }
        }
    }
}

static void _DeviceInfo_UpdateBgConnDev_Callback(void* p_param)
{
    DeviceInfo* info = (DeviceInfo*) p_param;

    if (!btm_ble_find_dev_in_whitelist(info->address)) {
        BTM_BleUpdateBgConnDev(1, info->address);
    }
}

static void _DeviceInfo_ReadPairing(BD_ADDR address, const char* path)
{
    int fileHandle;
    BloopairStoredDeviceHeader header;

    DEBUG_PRINT("Reading pairing for %s: %s\n", bdaddr_to_string(address), path);

    if (FSA_OpenFile(gFsaHandle, path, "r", &fileHandle) < 0) {
        return;
    }

    void* readBuffer = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, 0x100, 0x40);
    if (!readBuffer) {
        FSA_CloseFile(gFsaHandle, fileHandle);
        return;
    }

    if (FSA_ReadFile(gFsaHandle, readBuffer, 1, sizeof(header), fileHandle, 0) != sizeof(header)) {
        IOS_Free(CROSS_PROCESS_HEAP_ID, readBuffer);
        FSA_CloseFile(gFsaHandle, fileHandle);
        return;
    }
    memcpy(&header, readBuffer, sizeof(header));

    if (header.file_version != 0) {
        IOS_Free(CROSS_PROCESS_HEAP_ID, readBuffer);
        FSA_CloseFile(gFsaHandle, fileHandle);
        return;
    }

    DeviceInfo* info = DeviceInfo_GetOrAllocate(address);
    if (!info) {
        IOS_Free(CROSS_PROCESS_HEAP_ID, readBuffer);
        FSA_CloseFile(gFsaHandle, fileHandle);
        return;
    }

    info->magic = header.device_magic;
    info->vendor_id = header.vendor_id;
    info->product_id = header.product_id;
    info->flush = 0; // In case we just overwrote an already migrated pairing, we don't need to flush again

    switch (header.device_magic) {
    case MAGIC_BLOOPAIR:
    case MAGIC_SWITCH:
        // Read link key following header
        if (FSA_ReadFile(gFsaHandle, readBuffer, 1, 16, fileHandle, 0) != 16) {
            break;
        }
        memcpy(info->classic.link_key, readBuffer, 16);

        DEBUG_PRINT("Registering classic device %s\n", bdaddr_to_string(info->address));

        // Register pairing with WUD
        WUDRegisterDevice(info->address, info->classic.link_key, "Nintendo RVL-CNT-01-UC");
        break;
    case MAGIC_SWITCH2: {
        // Read LTK following header
        if (FSA_ReadFile(gFsaHandle, readBuffer, 1, 16, fileHandle, 0) != 16) {
            break;
        }

        memcpy(info->switch2.ltk, readBuffer, 16);

        DEBUG_PRINT("Registering Switch 2 device %s\n", bdaddr_to_string(info->address));

        // Register pairing with WUD
        static const uint8_t dummy_link_key[16] = { 0 };
        WUDRegisterDevice(info->address, dummy_link_key, "Nintendo RVL-CNT-01-UC");

        // Add device to auto connections whitelist
        DeviceInfo_AddToBgConn(info);
        break;
    }
    case MAGIC_BLOOPAIR_BLE: {
        BloopairStoredDeviceBLEData* ble = (BloopairStoredDeviceBLEData*) readBuffer;
        if (FSA_ReadFile(gFsaHandle, readBuffer, 1, sizeof(BloopairStoredDeviceBLEData), fileHandle, 0) !=
            sizeof(BloopairStoredDeviceBLEData)) {
            break;
        }

        info->ble.address_type = ble->address_type;
        memcpy(info->ble.ltk, ble->ltk, sizeof(ble->ltk));
        memcpy(info->ble.rand, ble->rand, sizeof(ble->rand));
        info->ble.ediv = ble->ediv;

        DEBUG_PRINT("Registering BLE device %s\n", bdaddr_to_string(info->address));

        // Register pairing with WUD
        static const uint8_t dummy_link_key[16] = { 0 };
        WUDRegisterDevice(info->address, dummy_link_key, "Nintendo RVL-CNT-01-UC");

        // Add device to auto connections whitelist
        DeviceInfo_AddToBgConn(info);
        break;
    }
    default:
        break;
    }

    IOS_Free(CROSS_PROCESS_HEAP_ID, readBuffer);
    FSA_CloseFile(gFsaHandle, fileHandle);
}

static void _DeviceInfo_ReadPairings(void)
{
    int dirHandle;
    FSADirectoryEntry entry;
    BD_ADDR addr;
    char path[512];

    if (FSA_OpenDir(gFsaHandle, BLOOPAIR_DEVICES_PATH, &dirHandle) < 0) {
        return;
    }

    while (FSA_ReadDir(gFsaHandle, dirHandle, &entry) == 0) {
        if (!filename_to_bda(entry.name, addr)) {
            DEBUG_PRINT("Bloopair: Ignoring invalid device %s\n", entry.name);
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", BLOOPAIR_DEVICES_PATH, entry.name);
        _DeviceInfo_ReadPairing(addr, path);
    }

    FSA_CloseDir(gFsaHandle, dirHandle);
}

static void _DeviceInfo_PurgePairings(uint8_t all)
{
    int dirHandle;
    FSADirectoryEntry entry;
    char path[512];
    BD_ADDR addr;

    if (FSA_OpenDir(gFsaHandle, BLOOPAIR_DEVICES_PATH, &dirHandle) < 0) {
        return;
    }

    while (FSA_ReadDir(gFsaHandle, dirHandle, &entry) == 0) {
        if (!filename_to_bda(entry.name, addr)) {
            continue;
        }

        // Check if we should only purge this device if WUD doesn't know about it
        if (!all && WUDiGetDevInfo(addr)) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", BLOOPAIR_DEVICES_PATH, entry.name);

        DEBUG_PRINT("Bloopair: Purging %s\n", path);

        FSA_Remove(gFsaHandle, path);
    }

    FSA_CloseDir(gFsaHandle, dirHandle);
}

static void _DeviceInfo_WritePairing(DeviceInfo* info)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%02X%02X%02X%02X%02X%02X.bin", BLOOPAIR_DEVICES_PATH, info->address[0],
             info->address[1], info->address[2], info->address[3], info->address[4], info->address[5]);

    DEBUG_PRINT("Bloopair: Write pairing %s\n", path);

    int fileHandle;
    if (FSA_OpenFile(gFsaHandle, path, "w", &fileHandle) < 0) {
        return;
    }

    void* writeBuffer = IOS_AllocAligned(CROSS_PROCESS_HEAP_ID, 0x100, 0x40);
    if (!writeBuffer) {
        return;
    }

    // Write header
    BloopairStoredDeviceHeader* header = (BloopairStoredDeviceHeader*) writeBuffer;
    header->file_version = 0;
    header->device_magic = info->magic;
    header->vendor_id = info->vendor_id;
    header->product_id = info->product_id;
    FSA_WriteFile(gFsaHandle, header, 1, sizeof(BloopairStoredDeviceHeader), fileHandle, 0);

    switch (info->magic) {
    case MAGIC_BLOOPAIR:
    case MAGIC_SWITCH: {
        // Take link key from WUD
        WUDDevInfo* dev = WUDiGetDevInfo(info->address);
        if (dev) {
            memcpy(writeBuffer, dev->link_key, 16);
            FSA_WriteFile(gFsaHandle, writeBuffer, 1, 16, fileHandle, 0);
        }
        break;
    }
    case MAGIC_SWITCH2: {
        memcpy(writeBuffer, info->switch2.ltk, 16);
        FSA_WriteFile(gFsaHandle, writeBuffer, 1, 16, fileHandle, 0);
        break;
    }
    case MAGIC_BLOOPAIR_BLE: {
        BloopairStoredDeviceBLEData* ble = (BloopairStoredDeviceBLEData*) writeBuffer;
        ble->address_type = info->ble.address_type;
        memcpy(ble->ltk, info->ble.ltk, sizeof(ble->ltk));
        memcpy(ble->rand, info->ble.rand, sizeof(ble->rand));
        ble->ediv = info->ble.ediv;
        FSA_WriteFile(gFsaHandle, ble, 1, sizeof(BloopairStoredDeviceBLEData), fileHandle, 0);
        break;
    }
    default:
        break;
    }

    IOS_Free(CROSS_PROCESS_HEAP_ID, writeBuffer);
    FSA_CloseFile(gFsaHandle, fileHandle);
}

static void _DeviceInfo_WritePairings(void)
{
    // Purge pairings WUD no longer knows about
    _DeviceInfo_PurgePairings(0);

    // Only write pairings WUD knows about, since we let WUD do housekeeping
    WUDDevice* dev = gWBC.firstDevice;
    while (dev) {
        DeviceInfo* info = DeviceInfo_Get(dev->info->address);

        // Write our custom pairings to SD
        if (info && info->flush &&
            (info->magic == MAGIC_BLOOPAIR || info->magic == MAGIC_SWITCH ||
             info->magic == MAGIC_SWITCH2 || info->magic == MAGIC_BLOOPAIR_BLE)) {
            _DeviceInfo_WritePairing(info);
            info->flush = 0;
        }

        dev = dev->next;
    }
}

void DeviceInfo_Init(void)
{
    // we only need to do this once, after that bloopair keeps track of device info
    if (devInfo_read) {
        return;
    }
    devInfo_read = 1;

    device_info_semaphore = IOS_CreateSemaphore(1, 1);

    // Parse official BTDB
    _DeviceInfo_ParseBTDB();

    // Read pairings from SD
    _DeviceInfo_ReadPairings();

    // Enable auto connections for BLE
    BTA_DmBleSetBgConnType(BTM_BLE_CONN_AUTO, NULL);
}

DeviceInfo* DeviceInfo_Get(const uint8_t* address)
{
    DeviceInfo* info = NULL;

    IOS_WaitSemaphore(device_info_semaphore, 0);

    for (int i = 0; i < MAX_DEVICE_INFO_COUNT; i++) {
        if (device_infos[i].magic != MAGIC_EMPTY && memcmp(device_infos[i].address, address, BD_ADDR_LEN) == 0) {
            info = &device_infos[i];
            break;
        }
    }

    IOS_SignalSemaphore(device_info_semaphore);
    return info;
}

DeviceInfo* DeviceInfo_Allocate(const uint8_t* address)
{
    DeviceInfo* info = NULL;

    IOS_WaitSemaphore(device_info_semaphore, 0);

    // look for a free entry
    for (int i = 0; i < MAX_DEVICE_INFO_COUNT; i++) {
        if (device_infos[i].magic == MAGIC_EMPTY) {
            memset(&device_infos[i], 0, sizeof(DeviceInfo));
            device_infos[i].magic = MAGIC_UNKNOWN;
            memcpy(device_infos[i].address, address, BD_ADDR_LEN);
            info = &device_infos[i];
            break;
        }
    }

    IOS_SignalSemaphore(device_info_semaphore);
    return info;
}

DeviceInfo* DeviceInfo_GetOrAllocate(const uint8_t* address)
{
    DeviceInfo* info = DeviceInfo_Get(address);
    if (info) {
        return info;
    }

    return DeviceInfo_Allocate(address);
}

void DeviceInfo_AddToBgConn(DeviceInfo* info)
{
    // TODO we currently never remove devices from auto connections
    //      these have only 10 slots and might fill up if someone ends up
    //      adding and removing a bunch of pairings
    //      Edit: We now remove them in WUDiRemoveDevice, is that enough?
    bta_dmexecutecallback(_DeviceInfo_UpdateBgConnDev_Callback, info);
}

int (*const real_purgeDevInfo)(void) = (void*) DEFINE_REAL(0x11f41200, 0xe92d40f0);
int purgeDevInfo_hook(void)
{
    _DeviceInfo_PurgePairings(1);

    return real_purgeDevInfo();
}

int writeDevInfo_hook(void (*callback)(int p1))
{
    DEBUG_PRINT("writeDevInfo_hook %p\n", callback);

    // Write our to-flush pairings to the SD first
    _DeviceInfo_WritePairings();

    // Make these static to prevent a potential stack overflow with our paths, should be fine?
    // This function only runs on the WUD thread anyways
    static BT_DevInfo devInfo, newDevInfo;
    static BD_ADDR devInfoEx[3], newDevInfoEx[3];
    UCSysConfig config[3] = {
        { "slc:btStd",            0x777, UC_DATATYPE_COMPLEX,   0, 0,                 NULL       },
        { "slc:btStd.devInfo",    0,     UC_DATATYPE_HEXBINARY, 0, sizeof(devInfo),   &devInfo   },
        { "slc:btStd.devInfoExt", 0,     UC_DATATYPE_HEXBINARY, 0, sizeof(devInfoEx), &devInfoEx },
    };

    memset(&devInfo, 0, sizeof(devInfo));
    memset(&devInfoEx, 0, sizeof(devInfoEx));
    memcpy(&newDevInfo, bt_devInfo, sizeof(BT_DevInfo));

    // TODO usually something weird with devInfoExt and controller_order happens here, but I have no idea what for
    // for (int i = 0; i < 3; i++) {
    //     memset(newDevInfo.controller_order[i].name, 0, 0x40);
    //     // memcpy(newDevInfoEx[i], )
    // }
    memset(&newDevInfoEx, 0, sizeof(newDevInfoEx));

    // We only write pairings that would've been in the official btdb
    // That way we also avoid running out of WUD slots, and we inherit
    // the WUD housekeeping
    int out = 0;
    for (int i = 0; i < newDevInfo.num_entries; i++) {
        BT_DevInfo_Entry* entry = &newDevInfo.entries[i];

        DeviceInfo* info = DeviceInfo_Get(entry->address);
        if (!info) {
            // we don't have info for this entry, skip it
            continue;
        }

        // Filter out any bloopair entries out of the btdb
        if (info->magic == MAGIC_EMPTY || info->magic == MAGIC_OFFICIAL) {
            if (i != out) {
                memcpy(&newDevInfo.entries[out], entry, sizeof(BT_DevInfo_Entry));
            }

            out++;
        }
    }

    // Update dev info count and clear tail
    newDevInfo.num_entries = out;
    for (uint8_t i = out; i < 10; i++) {
        memset(&newDevInfo.entries[i], 0, sizeof(newDevInfo.entries[i]));
    }

    int handle = UCOpen();
    if (handle < 0) {
        return 1;
    }

    // Check if config differs
    int ret = UCReadSysConfig(handle, 3, config);
    if (ret == 0 && memcmp(&devInfo, &newDevInfo, sizeof(devInfo)) == 0 &&
        memcmp(&devInfoEx, &newDevInfoEx, sizeof(devInfoEx)) == 0) {
        UCClose(handle);
    } else {
        memcpy(&devInfo, &newDevInfo, sizeof(newDevInfo));
        memcpy(&devInfoEx, &newDevInfoEx, sizeof(newDevInfoEx));
        ret = UCWriteSysConfig(handle, 3, config);
        UCClose(handle);
        if (ret != 0) {
            DEBUG_PRINT("UCWriteSysConfig failed %d\n", ret);
            return 1;
        }
    }

    if (callback) {
        callback(0);
    }

    return 0;
}

void DeviceInfo_ParseDIRecord(const uint8_t* bda, tSDP_DISCOVERY_DB* db)
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
    DeviceInfo* info = DeviceInfo_GetOrAllocate(bda);
    if (info) {
        info->vendor_id = vendor_id;
        info->product_id = product_id;
        info->flush = 1;
    }
}
