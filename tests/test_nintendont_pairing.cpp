#include <array>
#include <cassert>
#include <cstring>

#include <bloopair/nintendont_pairing.h>

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
    pairing.controller_type = NINTENDONT_SWITCH_PRO_TYPE;
    pairing.vendor_id = 0x057e;
    pairing.product_id = 0x2009;
    pairing.controller_bda[0] = 1;
    pairing.console_bda[0] = 1;
    pairing.key_type = NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN;
    for (size_t i = 0; i < sizeof(pairing.hci_link_key); i++) {
        pairing.hci_link_key[i] = static_cast<uint8_t>(i + 1);
    }
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    assert(NintendontSwitchPairingIsValid(&pairing));

    pairing.version = 1;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    assert(!NintendontSwitchPairingIsValid(&pairing));
    pairing.version = NINTENDONT_SWITCH_PAIRING_VERSION;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);

    pairing.controller_bda[2] ^= 0x40;
    assert(!NintendontSwitchPairingIsValid(&pairing));
    pairing.controller_bda[2] ^= 0x40;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    pairing.vendor_id = 0;
    assert(!NintendontSwitchPairingIsValid(&pairing));
    return 0;
}
