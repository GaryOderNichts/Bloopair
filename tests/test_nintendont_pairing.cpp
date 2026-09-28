#include <array>
#include <cassert>
#include <cstring>

#include <bloopair/nintendont_pairing.h>

int main()
{
    NintendontSwitchPairing pairing{};
    pairing.magic = NINTENDONT_SWITCH_PAIRING_MAGIC;
    pairing.version = NINTENDONT_SWITCH_PAIRING_VERSION;
    pairing.size = sizeof(pairing);
    pairing.controller_type = NINTENDONT_SWITCH_PRO_TYPE;
    pairing.vendor_id = 0x057e;
    pairing.product_id = 0x2009;
    pairing.controller_bda[0] = 1;
    pairing.console_bda[0] = 1;
    for (size_t i = 0; i < sizeof(pairing.link_key); i++) {
        pairing.link_key[i] = static_cast<uint8_t>(i + 1);
    }
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    assert(NintendontSwitchPairingIsValid(&pairing));

    pairing.controller_bda[2] ^= 0x40;
    assert(!NintendontSwitchPairingIsValid(&pairing));
    pairing.controller_bda[2] ^= 0x40;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);
    pairing.vendor_id = 0;
    assert(!NintendontSwitchPairingIsValid(&pairing));
    return 0;
}
