#include <assert.h>
#include <string.h>

#include <bloopair/nintendont_pairing.h>
#include <bloopair/ipc.h>
#include <bloopair/controllers/common.h>

#include "pairing_export.h"

static BloopairStoredSwitchProData stored[BLOOPAIR_MAX_STORED_SWITCH_PROS];
static size_t stored_count;

size_t store_get_switch_pro_controllers(BloopairStoredSwitchProData* output, size_t capacity)
{
    size_t count = stored_count < capacity ? stored_count : capacity;
    memcpy(output, stored, count * sizeof(*output));
    return count;
}

int main(void)
{
    tBTM_SEC_DEV_REC record;
    uint8_t hci_link_key[LINK_KEY_LEN];
    uint8_t key_type = 0;
    unsigned int i;

    memset(&record, 0, sizeof(record));
    for (i = 0; i < BD_ADDR_LEN; i++) {
        record.bd_addr[i] = (uint8_t) (i + 1);
    }
    for (i = 0; i < LINK_KEY_LEN; i++) {
        record.link_key[i] = (uint8_t) (0x10 + i);
    }

    memcpy(stored[0].bd_address, record.bd_addr, BD_ADDR_LEN);
    stored[0].controller_type = BLOOPAIR_CONTROLLER_SWITCH_PRO;
    stored[0].vendor_id = 0x057e;
    stored[0].product_id = 0x2009;
    stored_count = 1;

    pairing_export_capture_security_record(&record);
    uint32_t paired_generation = pairing_export_get_generation();
    assert(pairing_export_get(record.bd_addr, hci_link_key, &key_type) == 0);
    for (i = 0; i < LINK_KEY_LEN; i++) {
        assert(hci_link_key[i] == record.link_key[LINK_KEY_LEN - 1 - i]);
    }
    assert(key_type == NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN);

    record.link_key[0] ^= 0x55;
    pairing_export_capture_security_record(&record);
    assert(pairing_export_get_generation() != paired_generation);

    uint32_t repaired_generation = pairing_export_get_generation();
    stored_count = 0;
    pairing_export_refresh_generation();
    assert(pairing_export_get_generation() != repaired_generation);
    return 0;
}
