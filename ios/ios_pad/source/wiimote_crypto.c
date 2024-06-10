/*
 *   Copyright (C) GaryOderNichts
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
 * 
 *   Based on:
 *   -   <https://wiibrew.org/wiki/Wiimote>
 * 
 *   -   <https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/WiimoteEmu/Encryption.cpp>
 *       SPDX-License-Identifier: GPL-2.0-or-later
 *       Copyright (C) Hector Martin "marcan" (hector@marcansoft.com)
 * 
 *   -   <https://github.com/rnconrad/WiimoteEmulator/blob/master/wm_crypto.c>
 *       Copyright (C) rnconrad
 */

#include "wiimote_crypto.h"

// These are provided by the linker
// The kernel copies them from the loaded padscore rodata to the end of our custom IOS-PAD code
extern uint8_t __ans_tbl[][6];
extern uint8_t __sboxes[][256];

// ror8 - rotate an 8-bit value right
static inline uint8_t ror8(uint8_t word, uint32_t shift)
{
    return (word >> shift) | (word << (8 - shift));
}

void wiimoteCryptoInit(CryptoState* state, const uint8_t* ext_key)
{
    // Make sure the data from padscore was copied properly
    if (*(uint32_t*) __ans_tbl != 0xA877A6E0) {
        DEBUG_PRINT("Invalid __ans_tbl!\n");
        return;
    }
    if (*(uint32_t*) __sboxes != 0x70510386) {
        DEBUG_PRINT("Invalid __sboxes!\n");
        return;
    }

    const uint8_t* rand_data = ext_key;
    const uint8_t* key_data = ext_key + 10;

    // Bruteforce which idx was used to generate the key
    // This will always be between 0-6 for padscore / official Wii software
    int idx;
    uint8_t t0[10];
    for (idx = 0; idx < 7; idx++) {
        const uint8_t* ans = __ans_tbl[idx];

        for (int i = 0; i < 10; i++) {
            t0[i] = __sboxes[0][rand_data[9 - i]];
        }

        // Check if the generated key for that idx matches the key
        if ((key_data[5] == (uint8_t) ((ror8(ans[0] ^ t0[5], t0[2] % 8) - t0[9]) ^ t0[4])) &&
            (key_data[4] == (uint8_t) ((ror8(ans[1] ^ t0[1], t0[0] % 8) - t0[5]) ^ t0[7])) &&
            (key_data[3] == (uint8_t) ((ror8(ans[2] ^ t0[6], t0[8] % 8) - t0[2]) ^ t0[0])) &&
            (key_data[2] == (uint8_t) ((ror8(ans[3] ^ t0[4], t0[7] % 8) - t0[3]) ^ t0[2])) &&
            (key_data[1] == (uint8_t) ((ror8(ans[4] ^ t0[1], t0[6] % 8) - t0[3]) ^ t0[4])) &&
            (key_data[0] == (uint8_t) ((ror8(ans[5] ^ t0[7], t0[8] % 8) - t0[5]) ^ t0[9]))) {
            break;
        }
    }

    // This should never happen
    if (idx == 7) {
        DEBUG_PRINT("ext_key did not match any idx!\n");
        return;
    }

    // Generate the tables now that the idx was determined
    state->ft[0] = __sboxes[idx + 1][key_data[1]] ^ __sboxes[idx + 2][rand_data[6]];
    state->ft[1] = __sboxes[idx + 1][key_data[3]] ^ __sboxes[idx + 2][rand_data[4]];
    state->ft[2] = __sboxes[idx + 1][key_data[0]] ^ __sboxes[idx + 2][rand_data[2]];
    state->ft[3] = __sboxes[idx + 1][key_data[5]] ^ __sboxes[idx + 2][rand_data[7]];
    state->ft[4] = __sboxes[idx + 1][key_data[4]] ^ __sboxes[idx + 2][rand_data[5]];
    state->ft[5] = __sboxes[idx + 1][key_data[2]] ^ __sboxes[idx + 2][rand_data[0]];
    state->ft[6] = __sboxes[idx + 1][rand_data[9]] ^ __sboxes[idx + 2][rand_data[3]];
    state->ft[7] = __sboxes[idx + 1][rand_data[8]] ^ __sboxes[idx + 2][rand_data[1]];

    state->sb[0] = __sboxes[idx + 1][key_data[5]] ^ __sboxes[idx + 2][rand_data[8]];
    state->sb[1] = __sboxes[idx + 1][key_data[0]] ^ __sboxes[idx + 2][rand_data[5]];
    state->sb[2] = __sboxes[idx + 1][key_data[2]] ^ __sboxes[idx + 2][rand_data[9]];
    state->sb[3] = __sboxes[idx + 1][key_data[3]] ^ __sboxes[idx + 2][rand_data[0]];
    state->sb[4] = __sboxes[idx + 1][key_data[1]] ^ __sboxes[idx + 2][rand_data[2]];
    state->sb[5] = __sboxes[idx + 1][key_data[4]] ^ __sboxes[idx + 2][rand_data[1]];
    state->sb[6] = __sboxes[idx + 1][rand_data[6]] ^ __sboxes[idx + 2][rand_data[4]];
    state->sb[7] = __sboxes[idx + 1][rand_data[7]] ^ __sboxes[idx + 2][rand_data[3]];
}

void wiimoteEncrypt(const CryptoState* state, void* out, const void* data, uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        ((uint8_t*) out)[i] = (((const uint8_t*) data)[i] - state->ft[(addr + i) % 8]) ^ state->sb[(addr + i) % 8];
    }
}

void wiimoteDecrypt(const CryptoState* state, void* out, const void* data, uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        ((uint8_t*) out)[i] = (((const uint8_t*) data)[i] ^ state->sb[(addr + i) % 8]) + state->ft[(addr + i) % 8];
    }
}
