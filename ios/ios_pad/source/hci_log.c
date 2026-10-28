/*
 *   HCI traffic log (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#include "hci_log.h"
#include "audio.h"
#include "utils.h"

// BT_HDR of the stack: data starts at 8 + offset
typedef struct PACKED {
    uint16_t event;
    uint16_t len;
    uint16_t offset;
    uint16_t layer_specific;
} BtHdr;

#define BT_EVT_HCI_CMD  0x2000
#define BT_EVT_HCI_EVT  0x1000
#define BT_EVT_HCI_ACL  0x1100
#define BT_EVT_HCI_ACL_TX 0x2100

static void (*const real_hci_deliver)(void* hdr) = (void*) 0x11f2fe90;
static uint32_t (*const real_hci_write)(uint32_t arg, void* hdr) = (void*) 0x11f3b860;

uint32_t hciTxHook(uint32_t arg, void* hdr)
{
    BtHdr* h = (BtHdr*) hdr;
    if (h && h->event == BT_EVT_HCI_CMD && h->len >= 3) {
        const uint8_t* d = (const uint8_t*) hdr + 8 + h->offset;
        audioLogHci(BLOOPAIR_AUDIO_EV_HCI_CMD, d[0] | (d[1] << 8), d + 3, d[2]);
    }

    if (h && h->event == BT_EVT_HCI_ACL_TX && h->len >= 4) {
        const uint8_t* d = (const uint8_t*) hdr + 8 + h->offset;
        audioLogHci(BLOOPAIR_AUDIO_EV_ACL_TX, (d[0] | (d[1] << 8)) & 0xFFF, d + 4, h->len - 4);
    }

    return real_hci_write(arg, hdr);
}

void hciRxHook(void* hdr)
{
    BtHdr* h = (BtHdr*) hdr;
    if (h && (h->event & 0xFF00) == BT_EVT_HCI_EVT && h->len >= 2) {
        const uint8_t* d = (const uint8_t*) hdr + 8 + h->offset;
        audioLogHci(BLOOPAIR_AUDIO_EV_HCI_EVT, d[0], d + 2, d[1]);
    }

    if (h && h->event == BT_EVT_HCI_ACL && h->len >= 4) {
        const uint8_t* d = (const uint8_t*) hdr + 8 + h->offset;
        audioLogHci(BLOOPAIR_AUDIO_EV_ACL_RX, (d[0] | (d[1] << 8)) & 0xFFF, d + 4, h->len - 4);
    }

    real_hci_deliver(hdr);
}

static uint8_t (*const real_hci_disconnect)(uint16_t handle, uint8_t reason) = (void*) DEFINE_REAL(0x11f1ef60, 0xe92d4070);

uint8_t hciDisconnectHook(uint16_t handle, uint8_t reason)
{
    audioLogDisconnectCall(reason, (uint32_t) __builtin_return_address(0));
    return real_hci_disconnect(handle, reason);
}
