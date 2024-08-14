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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define PACKED __attribute__ ((__packed__))

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, #type " must be " #size " bytes")

// This macro is somewhat hacky and relies on the fact that rodata is mapped as executable
#define DEFINE_REAL(addr, instr) \
    ((const uint32_t[]) { \
    instr, \
    0xe51ff004, /* ldr pc, [pc, #-4] */ \
    addr, \
    });

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define bswap16 __builtin_bswap16
#define bswap32 __builtin_bswap32

uint32_t crc32(uint32_t seed, const void* data, size_t len);

void dumpHex(const void *data, size_t size);

#ifndef NDEBUG
#define DEBUG_PRINT(x, ...) printf(x, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x, ...)
#endif
