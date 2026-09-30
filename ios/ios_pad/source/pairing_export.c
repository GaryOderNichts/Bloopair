#include "pairing_export.h"

#include <bloopair/nintendont_pairing.h>
#include <string.h>
#include "info_store.h"

typedef struct {
    uint8_t address[BD_ADDR_LEN];
    uint8_t link_key[LINK_KEY_LEN];
    uint8_t valid;
} PairingExportEntry;

static PairingExportEntry entries[BTA_HH_MAX_KNOWN];
static uint32_t change_generation = 1;
static uint32_t snapshot_fingerprint;
static uint8_t snapshot_initialized;

static int key_is_nonzero(const uint8_t* key)
{
    uint8_t value = 0;
    for (unsigned int i = 0; i < LINK_KEY_LEN; i++) {
        value |= key[i];
    }
    return value != 0;
}

void pairing_export_capture_security_record(tBTM_SEC_DEV_REC* record)
{
    PairingExportEntry* free_entry = NULL;

    if (!record) {
        return;
    }

    for (unsigned int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        if (entries[i].valid && memcmp(entries[i].address, record->bd_addr, BD_ADDR_LEN) == 0) {
            if (key_is_nonzero(record->link_key)) {
                memcpy(entries[i].link_key, record->link_key, LINK_KEY_LEN);
            }
            pairing_export_refresh_generation();
            return;
        }
        if (!entries[i].valid && !free_entry) {
            free_entry = &entries[i];
        }
    }

    if (free_entry && key_is_nonzero(record->link_key)) {
        memcpy(free_entry->address, record->bd_addr, BD_ADDR_LEN);
        memcpy(free_entry->link_key, record->link_key, LINK_KEY_LEN);
        free_entry->valid = 1;
    }
    pairing_export_refresh_generation();
}

static uint32_t fingerprint_byte(uint32_t value, uint8_t byte)
{
    return (value ^ byte) * 16777619u;
}

void pairing_export_refresh_generation(void)
{
    BloopairStoredSwitchProData stored[BLOOPAIR_MAX_STORED_SWITCH_PROS];
    size_t count = store_get_switch_pro_controllers(stored, BLOOPAIR_MAX_STORED_SWITCH_PROS);
    uint32_t fingerprint = 2166136261u;
    fingerprint = fingerprint_byte(fingerprint, (uint8_t) count);
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < sizeof(stored[i]); j++) {
            fingerprint = fingerprint_byte(fingerprint, ((const uint8_t*) &stored[i])[j]);
        }
        for (size_t j = 0; j < BTA_HH_MAX_KNOWN; j++) {
            PairingExportEntry* entry = &entries[j];
            if (!entry->valid || memcmp(entry->address, stored[i].bd_address, BD_ADDR_LEN) != 0)
                continue;
            for (size_t k = 0; k < LINK_KEY_LEN; k++) {
                fingerprint = fingerprint_byte(fingerprint, entry->link_key[k]);
            }
            break;
        }
    }
    if (!snapshot_initialized) {
        snapshot_fingerprint = fingerprint;
        snapshot_initialized = 1;
    } else if (snapshot_fingerprint != fingerprint) {
        snapshot_fingerprint = fingerprint;
        change_generation++;
        if (change_generation == 0) change_generation = 1;
    }
}

uint32_t pairing_export_get_generation(void)
{
    pairing_export_refresh_generation();
    return change_generation;
}

int pairing_export_get(const uint8_t* address, uint8_t* hci_link_key, uint8_t* key_type)
{
    if (!address || !hci_link_key || !key_type) {
        return -4;
    }

    for (unsigned int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        PairingExportEntry* entry = &entries[i];
        if (!entry->valid || memcmp(entry->address, address, BD_ADDR_LEN) != 0) {
            continue;
        }
        NintendontSwitchPairingBroadcomKeyToHci(hci_link_key, entry->link_key);
        *key_type = NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN;
        return 0;
    }

    return -6;
}
