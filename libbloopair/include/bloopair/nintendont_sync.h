#pragma once

#include <bloopair/ipc.h>
#include <bloopair/nintendont_pairing.h>

static inline int NintendontSyncSameAddress(const uint8_t a[6], const uint8_t b[6])
{
    return memcmp(a, b, 6) == 0;
}

static inline int NintendontSyncIsStored(const BloopairStoredSwitchProList* stored,
                                         const uint8_t address[6])
{
    for (uint8_t i = 0; i < stored->count && i < BLOOPAIR_MAX_STORED_SWITCH_PROS; i++) {
        if (NintendontSyncSameAddress(stored->controllers[i].bd_address, address)) return 1;
    }
    return 0;
}

static inline int NintendontSyncHasAddress(const NintendontSwitchPairing* record,
                                            const uint8_t address[6])
{
    for (uint8_t i = 0; i < record->count; i++) {
        if (NintendontSyncSameAddress(record->controllers[i].controller_bda, address)) return 1;
    }
    return 0;
}

static inline void NintendontSyncAddOrReplace(NintendontSwitchPairing* record,
                                               const BloopairControllerPairingData* source)
{
    uint8_t index = record->count;
    for (uint8_t i = 0; i < record->count; i++) {
        if (NintendontSyncSameAddress(record->controllers[i].controller_bda,
                                     source->bd_address)) {
            index = i;
            break;
        }
    }
    if (index == record->count) {
        if (record->count >= NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS) return;
        record->count++;
    }
    NintendontSwitchPairingEntry* entry = &record->controllers[index];
    memcpy(entry->controller_bda, source->bd_address, sizeof(entry->controller_bda));
    memcpy(entry->hci_link_key, source->hci_link_key, sizeof(entry->hci_link_key));
    entry->key_type = source->key_type;
    entry->controller_type = source->controller_type;
    entry->vendor_id = source->vendor_id;
    entry->product_id = source->product_id;
    memset(entry->reserved, 0, sizeof(entry->reserved));
}
