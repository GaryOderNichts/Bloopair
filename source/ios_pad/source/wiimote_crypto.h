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

#include <imports.h>

typedef struct {
    uint8_t ft[8];
    uint8_t sb[8];
} CryptoState;

void wiimoteCryptoInit(CryptoState* state, const uint8_t* ext_key);

void wiimoteEncrypt(const CryptoState* state, void* encrypted, const void* decrypted, uint32_t addr, uint32_t size);

void wiimoteDecrypt(const CryptoState* state, void* decrypted, const void* encrypted, uint32_t addr, uint32_t size);
