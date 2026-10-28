/*
 *   Bluetooth device scan (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#pragma once

#include <imports.h>
#include "bt_api.h"
#include <bta/bta_api.h>
#include <bloopair/ipc.h>

// enable / disable the scan mode, optionally clearing the result list
void scanSetMode(uint8_t enabled, uint8_t clear);

// is the scan mode currently enabled
uint8_t scanIsEnabled(void);

// record an inquiry result (device address, class, rssi and name from EIR if present)
void scanOnInquiryResult(const tBTA_DM_INQ_RES* res);

// record the name of a device which was already found
void scanOnName(const uint8_t* bd_addr, const uint8_t* name);

// returns 1 if the device is known and is a peripheral (controller),
// 0 if it is known and not a peripheral, -1 if unknown
int scanDeviceKind(const uint8_t* bd_addr);

// copy the current results
void scanGetResults(BloopairScanResults* out);

// copies the address of the best audio device (headphones first, then strongest signal) to out, returns 1 if found
int scanFindFirstAudio(uint8_t* out);
