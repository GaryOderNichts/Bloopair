#pragma once

#include "bt_api.h"

void pairing_export_capture_security_record(tBTM_SEC_DEV_REC* record);
int pairing_export_get(const uint8_t* address, uint8_t* hci_link_key, uint8_t* key_type);
void pairing_export_refresh_generation(void);
uint32_t pairing_export_get_generation(void);
