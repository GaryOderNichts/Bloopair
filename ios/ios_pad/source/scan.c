/*
 *   Bluetooth device scan (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#include "scan.h"

#define EIR_TYPE_SHORT_NAME     0x08
#define EIR_TYPE_COMPLETE_NAME  0x09
#define EIR_MAX_SIZE            240

static BloopairScanResults scanResults;

static BloopairScanEntry* findEntry(const uint8_t* bd_addr)
{
    for (uint32_t i = 0; i < scanResults.count; i++) {
        if (memcmp(scanResults.entries[i].bd_address, bd_addr, 6) == 0) {
            return &scanResults.entries[i];
        }
    }

    return NULL;
}

static BloopairScanEntry* allocEntry(const uint8_t* bd_addr)
{
    if (scanResults.count >= BLOOPAIR_SCAN_MAX_RESULTS) {
        return NULL;
    }

    BloopairScanEntry* entry = &scanResults.entries[scanResults.count++];
    memset(entry, 0, sizeof(*entry));
    memcpy(entry->bd_address, bd_addr, 6);
    return entry;
}

static void setName(BloopairScanEntry* entry, const uint8_t* name, uint32_t maxLen)
{
    uint32_t len = 0;
    while (len < maxLen && len < (BLOOPAIR_SCAN_NAME_SIZE - 1) && name[len] != 0) {
        len++;
    }

    if (len == 0) {
        return;
    }

    memset(entry->name, 0, sizeof(entry->name));
    memcpy(entry->name, name, len);
    entry->flags |= BLOOPAIR_SCAN_FLAG_NAME_KNOWN;
}

// find the device name inside an extended inquiry response
static void parseEirName(BloopairScanEntry* entry, const uint8_t* eir)
{
    uint32_t pos = 0;
    while (pos + 1 < EIR_MAX_SIZE) {
        uint8_t len = eir[pos];
        if (len == 0) {
            break;
        }

        if (pos + len >= EIR_MAX_SIZE) {
            break;
        }

        uint8_t type = eir[pos + 1];
        if (type == EIR_TYPE_COMPLETE_NAME || type == EIR_TYPE_SHORT_NAME) {
            setName(entry, &eir[pos + 2], len - 1);
            // prefer the complete name, but a short name is better than nothing
            if (type == EIR_TYPE_COMPLETE_NAME) {
                return;
            }
        }

        pos += len + 1;
    }
}

void scanSetMode(uint8_t enabled, uint8_t clear)
{
    if (clear) {
        memset(&scanResults, 0, sizeof(scanResults));
    }

    scanResults.enabled = enabled ? 1 : 0;
}

uint8_t scanIsEnabled(void)
{
    return scanResults.enabled;
}

void scanOnInquiryResult(const tBTA_DM_INQ_RES* res)
{
    if (!scanResults.enabled) {
        return;
    }

    BloopairScanEntry* entry = findEntry(res->bd_addr);
    if (!entry) {
        entry = allocEntry(res->bd_addr);
        if (!entry) {
            return;
        }
    }

    memcpy(entry->dev_class, res->dev_class, 3);
    entry->rssi = res->rssi;

    if (res->p_eir) {
        parseEirName(entry, res->p_eir);
    }
}

void scanOnName(const uint8_t* bd_addr, const uint8_t* name)
{
    if (!scanResults.enabled) {
        return;
    }

    BloopairScanEntry* entry = findEntry(bd_addr);
    if (!entry) {
        return;
    }

    setName(entry, name, BLOOPAIR_SCAN_NAME_SIZE - 1);
}

int scanDeviceKind(const uint8_t* bd_addr)
{
    BloopairScanEntry* entry = findEntry(bd_addr);
    if (!entry) {
        return -1;
    }

    return ((entry->dev_class[1] & BTM_COD_MAJOR_CLASS_MASK) == BTM_COD_MAJOR_PERIPHERAL) ? 1 : 0;
}

void scanGetResults(BloopairScanResults* out)
{
    memcpy(out, &scanResults, sizeof(scanResults));
}

#define MAJOR_CLASS_AUDIO_VIDEO 4

// headphones are what we want: wearable headset (1) and headphones (6) first, then any other
// audio/video device; among equals the strongest signal wins (the one next to the console)
static int audioScore(const BloopairScanEntry* entry)
{
    if ((entry->dev_class[1] & BTM_COD_MAJOR_CLASS_MASK) != MAJOR_CLASS_AUDIO_VIDEO) {
        return -1;
    }

    uint8_t minor = entry->dev_class[0] >> 2;
    return (minor == 1 || minor == 6) ? 2 : 1;
}

int scanFindFirstAudio(uint8_t* out)
{
    int best = -1;
    int bestScore = -1;
    for (uint32_t i = 0; i < scanResults.count; i++) {
        BloopairScanEntry* entry = &scanResults.entries[i];
        int score = audioScore(entry);
        if (score < 0) {
            continue;
        }

        if (best < 0 || score > bestScore ||
            (score == bestScore && entry->rssi > scanResults.entries[best].rssi)) {
            best = i;
            bestScore = score;
        }
    }

    if (best < 0) {
        return 0;
    }

    memcpy(out, scanResults.entries[best].bd_address, 6);
    return 1;
}
