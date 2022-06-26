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

#include <stdlib.h>
#include <stdint.h>

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define DEFINE_REAL(addr, instr) \
    ((const uint32_t[]) { \
    instr, \
    0xe51ff004, /* ldr pc, [pc, #-4] */ \
    addr, \
    });

uint32_t crc32(uint32_t seed, const void* data, size_t len);

void dumpHex(const void *data, size_t size);

static inline uint16_t bswap16(uint16_t val)
{
    return (val >> 8) | (val << 8);
}

static inline uint32_t bswap32(uint32_t val)
{
    return  ((val << 24) & 0xff000000) |
            ((val <<  8) & 0x00ff0000) |
            ((val >>  8) & 0x0000ff00) |
            ((val >> 24) & 0x000000ff);
}

#ifdef LOGGING
#include "log/logger.h"
#define DEBUG(x, ...) log_printf(x, ##__VA_ARGS__)
#else
#define DEBUG(x, ...)
#endif
