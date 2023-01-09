/*
 *   Copyright (C) 2021-2023 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "bloopair_ipc.h"
#include <coreinit/ios.h>

#ifdef __cplusplus
extern "C" {
#endif

IOSHandle Bloopair_Open(void);

IOSError Bloopair_Close(IOSHandle handle);

BOOL Bloopair_IsActive(IOSHandle handle);

int32_t Bloopair_GetVersion(IOSHandle handle);

// reads the bluetooth device address of the local bluetooth controller
IOSError Bloopair_ReadControllerBDA(IOSHandle handle, uint8_t* outBDA);

// manually adds a controller pairing
IOSError Bloopair_AddControllerPairing(IOSHandle handle, uint8_t* bda, uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid);

#ifdef __cplusplus
}
#endif
