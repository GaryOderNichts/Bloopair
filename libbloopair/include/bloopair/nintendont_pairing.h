#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define NINTENDONT_SWITCH_PAIRING_MAGIC   0x4e535042u
#define NINTENDONT_SWITCH_PAIRING_VERSION 3u
#define NINTENDONT_SWITCH_PAIRING_PATH    "wiiu/bloopair/nintendont-switch-pro.bin"
#define NINTENDONT_SWITCH_PRO_TYPE        0x24u
#define NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN 0xffu
#define NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS 4u

typedef struct __attribute__((packed)) {
    uint8_t controller_bda[6];
    uint8_t hci_link_key[16];
    uint8_t key_type;
    uint8_t controller_type;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t reserved[2];
} NintendontSwitchPairingEntry;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint8_t console_bda[6];
    uint8_t count;
    uint8_t reserved;
    NintendontSwitchPairingEntry controllers[NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS];
    uint32_t checksum;
} NintendontSwitchPairing;

static inline void NintendontSwitchPairingBroadcomKeyToHci(
    uint8_t destination[16], const uint8_t source[16])
{
    for (size_t i = 0; i < 16; i++) {
        destination[i] = source[15 - i];
    }
}

static inline uint32_t NintendontSwitchPairingChecksum(const NintendontSwitchPairing* pairing)
{
    const uint8_t* bytes = (const uint8_t*) pairing;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < offsetof(NintendontSwitchPairing, checksum); i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

static inline int NintendontSwitchPairingIsValid(const NintendontSwitchPairing* pairing)
{
    uint8_t console_or = 0;
    if (!pairing || pairing->magic != NINTENDONT_SWITCH_PAIRING_MAGIC ||
        pairing->version != NINTENDONT_SWITCH_PAIRING_VERSION ||
        pairing->size != sizeof(*pairing) || pairing->count == 0 ||
        pairing->count > NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS) {
        return 0;
    }
    for (size_t i = 0; i < sizeof(pairing->console_bda); i++) {
        console_or |= pairing->console_bda[i];
    }
    if (!console_or) {
        return 0;
    }
    for (size_t i = 0; i < pairing->count; i++) {
        const NintendontSwitchPairingEntry* entry = &pairing->controllers[i];
        uint8_t address_or = 0;
        uint8_t key_or = 0;
        if (entry->controller_type != NINTENDONT_SWITCH_PRO_TYPE ||
            entry->vendor_id != 0x057e || entry->product_id != 0x2009) {
            return 0;
        }
        for (size_t j = 0; j < sizeof(entry->controller_bda); j++) {
            address_or |= entry->controller_bda[j];
        }
        for (size_t j = 0; j < sizeof(entry->hci_link_key); j++) {
            key_or |= entry->hci_link_key[j];
        }
        if (!address_or || !key_or) {
            return 0;
        }
        for (size_t j = 0; j < i; j++) {
            if (memcmp(entry->controller_bda,
                    pairing->controllers[j].controller_bda, 6) == 0) {
                return 0;
            }
        }
    }
    return pairing->checksum == NintendontSwitchPairingChecksum(pairing);
}

typedef char NintendontSwitchPairingEntrySizeCheck[
    (sizeof(NintendontSwitchPairingEntry) == 30) ? 1 : -1];
typedef char NintendontSwitchPairingSizeCheck[
    (sizeof(NintendontSwitchPairing) == 140) ? 1 : -1];
