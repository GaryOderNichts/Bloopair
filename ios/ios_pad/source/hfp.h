/*
 *   Battery level of the headphones over the hands-free profile (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#pragma once

#include <imports.h>
#include <bloopair/ipc.h>

// start reading the battery of the device the audio stream runs to (stack context, after the stream started)
void hfpStart(const uint8_t* bd_addr);

// close everything again (stack context), safe to call when nothing runs
void hfpStop(void);

// state for the status of the PPC side
void hfpGetStatus(BloopairAudioStatus* out);
