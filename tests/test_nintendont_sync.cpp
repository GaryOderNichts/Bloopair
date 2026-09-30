#include <bloopair/nintendont_sync.h>

#include <cassert>
#include <cstring>

static BloopairControllerPairingData Controller(uint8_t address, uint8_t key)
{
    BloopairControllerPairingData result{};
    result.bd_address[5] = address;
    std::memset(result.hci_link_key, key, sizeof(result.hci_link_key));
    result.controller_type = NINTENDONT_SWITCH_PRO_TYPE;
    result.vendor_id = 0x057e;
    result.product_id = 0x2009;
    return result;
}

int main()
{
    NintendontSwitchPairing record{};
    auto first = Controller(1, 0x11);
    auto second = Controller(2, 0x22);
    auto powera = Controller(9, 0x99);
    powera.vendor_id = 0;
    powera.product_id = 0;
    NintendontSyncAddOrReplace(&record, &powera);
    assert(record.count == 0);
    NintendontSyncAddOrReplace(&record, &first);
    NintendontSyncAddOrReplace(&record, &second);
    assert(record.count == 2);

    auto repaired = Controller(1, 0x33);
    NintendontSyncAddOrReplace(&record, &repaired);
    assert(record.count == 2);
    assert(record.controllers[0].hci_link_key[0] == 0x33);
    assert(NintendontSyncHasAddress(&record, first.bd_address));

    powera.vendor_id = 0x20d6;
    powera.product_id = 0xa711;
    NintendontSyncAddOrReplace(&record, &powera);
    assert(record.count == 2);

    BloopairStoredSwitchProList stored{};
    stored.count = 1;
    std::memcpy(stored.controllers[0].bd_address, second.bd_address, 6);
    assert(!NintendontSyncIsStored(&stored, first.bd_address));
    assert(NintendontSyncIsStored(&stored, second.bd_address));

    for (uint8_t i = 3; i <= 6; i++) {
        auto extra = Controller(i, i);
        NintendontSyncAddOrReplace(&record, &extra);
    }
    assert(record.count == NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS);

    /* Unsupported devices before, between and after supported devices must
     * never consume one of the four export positions. */
    record = {};
    for (uint8_t i = 10; i < 16; i++) {
        auto unsupported = Controller(i, i);
        unsupported.vendor_id = 0;
        unsupported.product_id = 0;
        NintendontSyncAddOrReplace(&record, &unsupported);
    }
    for (uint8_t i = 1; i <= 4; i++) {
        auto supported = Controller(i, i);
        NintendontSyncAddOrReplace(&record, &supported);
        auto unsupported = Controller((uint8_t) (20 + i), i);
        unsupported.vendor_id = 0x20d6;
        unsupported.product_id = 0xa711;
        NintendontSyncAddOrReplace(&record, &unsupported);
    }
    assert(record.count == NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS);
    for (uint8_t i = 0; i < record.count; i++) {
        assert(record.controllers[i].vendor_id == 0x057e);
        assert(record.controllers[i].product_id == 0x2009);
    }
}
