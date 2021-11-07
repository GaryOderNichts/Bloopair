/*
 *   Copyright (C) 2021 GaryOderNichts
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

#include <coreinit/ios.h>

IOSHandle openBtrm();

void closeBtrm(IOSHandle handle);

// returns true if bloopair is running
bool isBloopairRunning(IOSHandle handle);

// reads the bluetooth device address of the local bluetooth controller
IOSError readControllerBDAddr(IOSHandle handle, uint8_t* outBDA);

// manually adds a controller pairing
IOSError addControllerPairing(IOSHandle handle, uint8_t* bda, uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid);
