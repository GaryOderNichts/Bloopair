#pragma once

#include <stddef.h>
#include <stdint.h>

#define NINTENDONT_SWITCH_PAIRING_MAGIC   0x4e535042u
#define NINTENDONT_SWITCH_PAIRING_VERSION 2u
#define NINTENDONT_SWITCH_PAIRING_PATH    "wiiu/bloopair/nintendont-switch-pro.bin"
#define NINTENDONT_SWITCH_PRO_TYPE        0x24u
#define NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN 0xffu

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint8_t controller_bda[6];
    uint8_t console_bda[6];
    /* Exact byte order used by the HCI Link Key Request Reply payload. */
    uint8_t hci_link_key[16];
    uint8_t key_type;
    uint8_t controller_type;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t reserved[2];
    uint32_t checksum;
} NintendontSwitchPairing;

/*
 * Bluedroid/Broadcom stores a BR/EDR link key in the opposite order from the
 * HCI command payload. ARRAY16_TO_STREAM performs this same conversion when
 * Bloopair answers a Link Key Request itself.
 */
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
    uint8_t key_or = 0;
    uint8_t controller_or = 0;
    uint8_t console_or = 0;
    if (!pairing || pairing->magic != NINTENDONT_SWITCH_PAIRING_MAGIC ||
        pairing->version != NINTENDONT_SWITCH_PAIRING_VERSION ||
        pairing->size != sizeof(*pairing) ||
        pairing->controller_type != NINTENDONT_SWITCH_PRO_TYPE ||
        pairing->vendor_id != 0x057e ||
        pairing->product_id != 0x2009) {
        return 0;
    }
    for (size_t i = 0; i < sizeof(pairing->controller_bda); i++) {
        controller_or |= pairing->controller_bda[i];
        console_or |= pairing->console_bda[i];
    }
    for (size_t i = 0; i < sizeof(pairing->hci_link_key); i++) {
        key_or |= pairing->hci_link_key[i];
    }
    return controller_or != 0 && console_or != 0 && key_or != 0 &&
        pairing->checksum == NintendontSwitchPairingChecksum(pairing);
}

typedef char NintendontSwitchPairingSizeCheck[(sizeof(NintendontSwitchPairing) == 48) ? 1 : -1];
