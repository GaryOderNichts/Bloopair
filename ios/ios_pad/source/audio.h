/*
 *   Bluetooth audio connection test (BTAudio)
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

// arm / disarm the test (called from the IPC thread, only touches flags)
void audioArmTest(uint8_t enabled);

// copy the current status and event log
void audioGetStatus(BloopairAudioStatus* out);

// called from the search callback (Bluetooth stack context) for every search event
void audioOnSearchEvent(uint8_t event);

// called for every security event of the Bluetooth stack (pairing etc.).
// returns 1 if the event belongs to the audio test and must not be passed on
int audioOnSecurityEvent(uint8_t event, void* p_data);

// called when the console starts a device search, services is the search parameter
void audioOnSearchStart(uint8_t services);

// returns 1 while the audio test uses the radio and no new search may be started
int audioBlockSearch(void);

// log one HCI command or event (kind BLOOPAIR_AUDIO_EV_HCI_CMD / _EVT), only while the test pairs or connects
void audioLogHci(uint8_t kind, uint16_t code, const uint8_t* params, uint32_t len);

// add an entry to the event log of the current connection (for hfp.c)
void audioLogEvent(uint16_t event, uint16_t value, uint32_t data);

// log who asks the stack to disconnect a link (address of the calling code), only while the test pairs or connects
void audioLogDisconnectCall(uint8_t reason, uint32_t caller);

// runs in the Bluetooth application task: close the connection (requested through audioArmTest(0))
void audioStopRequested(void);

// SBC frames from the PPC side, returns the number of frames taken
int audioWrite(const uint8_t* data, uint32_t length);

// connect to one audio device (request from the IPC thread, the work runs in the stack's context)
void audioRequestConnect(const BloopairAudioConnectData* request);
void audioRequestDisconnect(void);

// messages posted with a2dpPostMessage which are meant for audio.c
void audioHandleMessage(uint8_t code);
