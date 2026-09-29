#include <array>
#include <cassert>
#include <cstring>

#include <bloopair/nintendont_pairing.h>

static void fill_entry(NintendontSwitchPairingEntry& entry, uint8_t id)
{
    entry.controller_bda[0] = id;
    entry.key_type = NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN;
    entry.controller_type = NINTENDONT_SWITCH_PRO_TYPE;
    entry.vendor_id = 0x057e;
    entry.product_id = 0x2009;
    for (size_t i = 0; i < sizeof(entry.hci_link_key); i++) {
        entry.hci_link_key[i] = static_cast<uint8_t>(id + i);
    }
}

int main()
{
    std::array<uint8_t, 16> broadcom{};
    std::array<uint8_t, 16> hci{};
    for (size_t i = 0; i < broadcom.size(); i++) {
        broadcom[i] = static_cast<uint8_t>(i);
    }
    NintendontSwitchPairingBroadcomKeyToHci(hci.data(), broadcom.data());
    for (size_t i = 0; i < hci.size(); i++) {
        assert(hci[i] == broadcom[15 - i]);
    }

    NintendontSwitchPairing pairing{};
    pairing.magic = NINTENDONT_SWITCH_PAIRING_MAGIC;
    pairing.version = NINTENDONT_SWITCH_PAIRING_VERSION;
    pairing.size = sizeof(pairing);
    pairing.console_bda[0] = 1;
    pairing.count = 2;
    fill_entry(pairing.controllers[0], 1);
    fill_entry(pairing.controllers[1], 2);
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    assert(sizeof(NintendontSwitchPairingEntry) == 30);
    assert(sizeof(NintendontSwitchPairing) == 140);
    assert(NintendontSwitchPairingIsValid(&pairing));

    pairing.controllers[1].controller_bda[0] = 1;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    assert(!NintendontSwitchPairingIsValid(&pairing));
    pairing.controllers[1].controller_bda[0] = 2;
    pairing.count = 0;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    assert(!NintendontSwitchPairingIsValid(&pairing));
    pairing.count = 2;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    pairing.controllers[0].hci_link_key[3] ^= 0x40;
    assert(!NintendontSwitchPairingIsValid(&pairing));
    return 0;
}
