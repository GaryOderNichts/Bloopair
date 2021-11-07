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

#include "crypto.h"

extern char __ans_tbl;
extern char __sboxes;

#define ANS_TBL ((uint8_t(*)[6]) &__ans_tbl)
#define SBOXES ((uint8_t(*)[256]) &__sboxes)

static inline uint8_t ror8(uint8_t a, uint8_t b)
{
    return (a >> b) | ((a << (8 - b)) & 0xff);
}

void cryptoInit(CryptoState* state, const uint8_t* key)
{
    if (*(uint32_t*) ANS_TBL != 0xA877A6E0) {
        DEBUG("Invalid ANS_TBL!!!\n");
        return;
    }

    if (*(uint32_t*) SBOXES != 0x70510386) {
        DEBUG("Invalid SBOXES!!!\n");
        return;
    }

    // determine idx with simple brute force
    int idx;
    uint8_t t0[10];
    for (idx = 0; idx < 7; idx++) {
        const uint8_t* ans = ANS_TBL[idx];

        for(int i = 0; i < 10; i++) {
            t0[i] = SBOXES[0][key[9 - i]];
        }

        if ((key[0xf] == (uint8_t) ((ror8(ans[0] ^ t0[5], t0[2]%8) - t0[9]) ^ t0[4])) &&
            (key[0xe] == (uint8_t) ((ror8(ans[1] ^ t0[1], t0[0]%8) - t0[5]) ^ t0[7])) &&
            (key[0xd] == (uint8_t) ((ror8(ans[2] ^ t0[6], t0[8]%8) - t0[2]) ^ t0[0])) &&
            (key[0xc] == (uint8_t) ((ror8(ans[3] ^ t0[4], t0[7]%8) - t0[3]) ^ t0[2])) &&
            (key[0xb] == (uint8_t) ((ror8(ans[4] ^ t0[1], t0[6]%8) - t0[3]) ^ t0[4])) &&
            (key[0xa] == (uint8_t) ((ror8(ans[5] ^ t0[7], t0[8]%8) - t0[5]) ^ t0[9]))) {
            break;
        }
    }

    state->ft[0] = SBOXES[idx + 1][key[0xb]] ^ SBOXES[idx + 2][key[0x6]];
    state->ft[1] = SBOXES[idx + 1][key[0xd]] ^ SBOXES[idx + 2][key[0x4]];
    state->ft[2] = SBOXES[idx + 1][key[0xa]] ^ SBOXES[idx + 2][key[0x2]];
    state->ft[3] = SBOXES[idx + 1][key[0xf]] ^ SBOXES[idx + 2][key[0x7]];
    state->ft[4] = SBOXES[idx + 1][key[0xe]] ^ SBOXES[idx + 2][key[0x5]];
    state->ft[5] = SBOXES[idx + 1][key[0xc]] ^ SBOXES[idx + 2][key[0x0]];
    state->ft[6] = SBOXES[idx + 1][key[0x9]] ^ SBOXES[idx + 2][key[0x3]];
    state->ft[7] = SBOXES[idx + 1][key[0x8]] ^ SBOXES[idx + 2][key[0x1]];

    state->sb[0] = SBOXES[idx + 1][key[0xf]] ^ SBOXES[idx + 2][key[0x8]];
    state->sb[1] = SBOXES[idx + 1][key[0xa]] ^ SBOXES[idx + 2][key[0x5]];
    state->sb[2] = SBOXES[idx + 1][key[0xc]] ^ SBOXES[idx + 2][key[0x9]];
    state->sb[3] = SBOXES[idx + 1][key[0xd]] ^ SBOXES[idx + 2][key[0x0]];
    state->sb[4] = SBOXES[idx + 1][key[0xb]] ^ SBOXES[idx + 2][key[0x2]];
    state->sb[5] = SBOXES[idx + 1][key[0xe]] ^ SBOXES[idx + 2][key[0x1]];
    state->sb[6] = SBOXES[idx + 1][key[0x6]] ^ SBOXES[idx + 2][key[0x4]];
    state->sb[7] = SBOXES[idx + 1][key[0x7]] ^ SBOXES[idx + 2][key[0x3]];
}

void encrypt(const CryptoState* state, uint8_t* encrypted, const uint8_t* decrypted, uint32_t addr_offset, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        encrypted[i] = (decrypted[i] - state->ft[(i + (addr_offset & 7)) % 8]) ^ state->sb[(i + (addr_offset & 7)) % 8];
    }
}

void decrypt(const CryptoState* state, uint8_t* decrypted, const uint8_t* encrypted, uint32_t addr_offset, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        decrypted[i] = (encrypted[i] ^ state->sb[(i + (addr_offset & 7)) % 8]) + state->ft[(i + (addr_offset & 7)) % 8];
    }
}
