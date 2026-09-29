#include <assert.h>
#include <string.h>

#include <bloopair/nintendont_pairing.h>

#include "pairing_export.h"

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

    pairing_export_capture_security_record(&record);
    assert(pairing_export_get(record.bd_addr, hci_link_key, &key_type) == 0);
    for (i = 0; i < LINK_KEY_LEN; i++) {
        assert(hci_link_key[i] == record.link_key[LINK_KEY_LEN - 1 - i]);
    }
    assert(key_type == NINTENDONT_SWITCH_PAIRING_KEY_TYPE_UNKNOWN);
    return 0;
}
