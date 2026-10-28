/*
 *   HCI traffic log (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#pragma once

#include <stdint.h>

// replaces the call of the stack's HCI send function (commands go through here), logs and forwards
uint32_t hciTxHook(uint32_t arg, void* hdr);

// replaces the call in the USB interrupt-read callback which hands a received HCI event to the stack
void hciRxHook(void* hdr);

// replaces the start of the stack's btsnd_hcic_disconnect, logs who calls it and runs the original
uint8_t hciDisconnectHook(uint16_t handle, uint8_t reason);
