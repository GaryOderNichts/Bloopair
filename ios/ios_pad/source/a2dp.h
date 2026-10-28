/*
 *   A2DP media streaming (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint16_t cid;               // l2cap channel of the media transport
    uint16_t mtu;               // largest packet the device accepts on it
    uint16_t frameLength;       // bytes of one SBC frame
    uint16_t samplesPerFrame;   // pcm samples per channel in one frame (blocks * subbands)
    uint32_t sampleRate;        // in Hz
} A2dpParams;

#define A2DP_RING_FRAMES    64

// registers the handler which runs our code in the Bluetooth application task (call from the stack context)
void a2dpInit(void);

// start / stop sending queued frames (stack context)
// returns 0 if the stream can not run with these parameters
int a2dpStart(const A2dpParams* params);
void a2dpStop(void);

// queue whole SBC frames, returns how many were taken (IPC thread)
uint32_t a2dpWrite(const uint8_t* data, uint32_t length);

// the l2cap channel reported (no more) room for packets
void a2dpSetCongested(uint8_t congested);

// message codes for a2dpPostMessage, 1 is used internally for the timer
#define A2DP_MSG_STOP       2
#define A2DP_MSG_CONNECT    3
#define A2DP_MSG_CONNECT2   4

// ask the Bluetooth application task to run audioHandleMessage(code) (any thread)
void a2dpPostMessage(uint8_t code);
void a2dpPostStopRequest(void);

typedef struct {
    uint16_t queued;
    uint32_t sent;
    uint32_t underruns;
    uint32_t congestions;
    uint32_t congestedMs;
    uint32_t maxCongestedMs;
    uint32_t droppedFrames;
} A2dpStats;

void a2dpGetStats(A2dpStats* stats);
