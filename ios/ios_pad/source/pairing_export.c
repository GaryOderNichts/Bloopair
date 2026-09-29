#include "pairing_export.h"

#include <bloopair/nintendont_pairing.h>
#include <string.h>

typedef struct {
    uint8_t address[BD_ADDR_LEN];
    tBTM_SEC_DEV_REC* record;
} PairingExportEntry;

static PairingExportEntry entries[BTA_HH_MAX_KNOWN];

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
        if (entries[i].record && memcmp(entries[i].address, record->bd_addr, BD_ADDR_LEN) == 0) {
            entries[i].record = record;
            return;
        }
        if (!entries[i].record && !free_entry) {
            free_entry = &entries[i];
        }
    }

    if (free_entry) {
        memcpy(free_entry->address, record->bd_addr, BD_ADDR_LEN);
        free_entry->record = record;
    }
}

int pairing_export_get(const uint8_t* address, uint8_t* hci_link_key, uint8_t* key_type)
{
    if (!address || !hci_link_key || !key_type) {
        return -4;
    }

    for (unsigned int i = 0; i < BTA_HH_MAX_KNOWN; i++) {
        tBTM_SEC_DEV_REC* record = entries[i].record;
        if (!record || memcmp(entries[i].address, address, BD_ADDR_LEN) != 0 ||
            memcmp(record->bd_addr, address, BD_ADDR_LEN) != 0) {
            continue;
        }
        if (!key_is_nonzero(record->link_key)) {
            return -6;
        }

        NintendontSwitchPairingBroadcomKeyToHci(hci_link_key, record->link_key);
        *key_type = NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN;
        return 0;
    }

    return -6;
}
