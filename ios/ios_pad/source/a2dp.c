/*
 *   A2DP media streaming (BTAudio)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   The PPC side delivers ready SBC frames. A timer thread wakes up every 10 ms and asks the
 *   Bluetooth application task (via a BTA message, so everything that touches the stack runs in
 *   the stack's own context) to send as many frames as the elapsed time allows.
 */

#include <imports.h>
#include <string.h>
#include "a2dp.h"
#include "audio.h"

#define BT_HDR_SIZE         8
#define TX_OFFSET           32
#define RTP_HEADER_SIZE     12
#define MAX_FRAME_LENGTH    200
#define TICK_US             10000
#define TICK_MESSAGE        0xA2D0
#define THREAD_STACK_SIZE   0x800
#define PRIME_FRAMES        8
#define MAX_BACKLOG_FRAMES  24      // more waiting than this only adds delay (about 65 ms at 48 kHz)

// BTA message codes (the high byte of the event is the id of our subsystem)
#define MSG_TICK            1

extern uint8_t L2CA_DataWrite(uint16_t cid, BT_HDR* p_data);

// bta_sys_cb.reg: one pointer per subsystem id, bta_sys_event calls (*reg[id])(msg) and frees msg if it returns non-zero
#define BTA_SYS_REG_TABLE   ((volatile uint32_t*) 0x1214d948)
#define BTA_SYS_MAX_ID      0x2b

static uint8_t frames[A2DP_RING_FRAMES][MAX_FRAME_LENGTH];
static volatile uint32_t ringHead;      // frames written so far
static volatile uint32_t ringTail;      // frames sent so far

static A2dpParams params;
static volatile uint8_t active;
static volatile uint8_t congested;
static volatile uint8_t tickPending;
static volatile uint32_t ticksOwed;
static uint8_t primed;
static uint32_t credit;
static uint16_t rtpSequence;
static uint32_t rtpTimestamp;
static uint32_t sentFrames;
static uint32_t underruns;
static uint32_t congestions;
static volatile uint32_t congestedTicks;
static volatile uint32_t currentCongestedTicks;
static volatile uint32_t maxCongestedTicks;
static uint32_t droppedFrames;

static uint8_t btaId;
static uint32_t* btaReg;
static int queueId;
static int timerId;
static int threadStarted;
static uint32_t queueStorage;
static void* threadStack;

// head and tail count up forever (A2DP_RING_FRAMES is a power of two), the slot is the counter modulo the size
static uint32_t ringCount(void)
{
    return ringHead - ringTail;
}

void a2dpPostMessage(uint8_t code)
{
    if (!btaId) {
        return;
    }

    BT_HDR* msg = (BT_HDR*) GKI_getbuf(sizeof(BT_HDR));
    if (!msg) {
        tickPending = 0;
        return;
    }

    msg->event = (btaId << 8) | code;
    msg->len = 0;
    msg->offset = 0;
    msg->layer_specific = 0;
    bta_sys_sendmsg(msg);
}

static void sendPacket(uint32_t count)
{
    uint32_t total = RTP_HEADER_SIZE + 1 + count * params.frameLength;
    BT_HDR* p_buf = (BT_HDR*) GKI_getbuf(BT_HDR_SIZE + TX_OFFSET + total);
    if (!p_buf) {
        return;
    }

    uint8_t* d = ((uint8_t*) p_buf) + BT_HDR_SIZE + TX_OFFSET;
    p_buf->event = 0;
    p_buf->layer_specific = 0;
    p_buf->offset = TX_OFFSET;
    p_buf->len = total;

    // RTP header: version 2, dynamic payload type 96
    d[0] = 0x80;
    d[1] = 0x60;
    d[2] = rtpSequence >> 8;
    d[3] = rtpSequence & 0xFF;
    d[4] = rtpTimestamp >> 24;
    d[5] = rtpTimestamp >> 16;
    d[6] = rtpTimestamp >> 8;
    d[7] = rtpTimestamp;
    d[8] = 'W';
    d[9] = 'U';
    d[10] = 'I';
    d[11] = 'U';
    // SBC payload header: not fragmented, number of frames
    d[12] = count & 0x0F;

    uint8_t* out = d + RTP_HEADER_SIZE + 1;
    for (uint32_t i = 0; i < count; i++) {
        memcpy(out, frames[(ringTail + i) % A2DP_RING_FRAMES], params.frameLength);
        out += params.frameLength;
    }
    ringTail += count;

    rtpSequence++;
    rtpTimestamp += count * params.samplesPerFrame;
    sentFrames += count;

    L2CA_DataWrite(params.cid, p_buf);
}

static void sendDue(void)
{
    uint32_t owed = ticksOwed;
    ticksOwed = 0;
    tickPending = 0;

    if (!active) {
        return;
    }

    credit += owed * (params.sampleRate / (1000000 / TICK_US));
    if (credit > 12 * params.samplesPerFrame) {
        credit = 12 * params.samplesPerFrame;
    }

    uint32_t maxPerPacket = (params.mtu - RTP_HEADER_SIZE - 1) / params.frameLength;
    if (maxPerPacket > 15) {
        maxPerPacket = 15;
    }

    // after the link was stuck the queue is full of old sound: skip it, or the delay stays high for good
    if (!congested && ringCount() > MAX_BACKLOG_FRAMES) {
        uint32_t drop = ringCount() - PRIME_FRAMES;
        ringTail += drop;
        droppedFrames += drop;
        credit = 0; // what is left is sent in real time, not in one burst that empties the queue again
    }

    while (credit >= params.samplesPerFrame && !congested) {
        uint32_t avail = ringCount();
        if (!primed) {
            if (avail < PRIME_FRAMES) {
                credit = 0;
                return;
            }
            primed = 1;
        }

        if (avail == 0) {
            underruns++;
            primed = 0;
            credit = 0;
            return;
        }

        uint32_t n = credit / params.samplesPerFrame;
        if (n > avail) {
            n = avail;
        }
        if (n > maxPerPacket) {
            n = maxPerPacket;
        }

        sendPacket(n);
        credit -= n * params.samplesPerFrame;
    }
}

static uint8_t btaHandler(BT_HDR* msg)
{
    switch (msg->event & 0xFF) {
    case MSG_TICK:
        sendDue();
        break;
    default:
        audioHandleMessage(msg->event & 0xFF);
        break;
    }

    return 1;
}

static int timerThread(void* arg)
{
    for (;;) {
        uint32_t message;
        IOS_ReceiveMessage(queueId, &message, 0);

        if (!active) {
            continue;
        }

        if (congested) {
            congestedTicks++;
            currentCongestedTicks++;
            if (currentCongestedTicks > maxCongestedTicks) {
                maxCongestedTicks = currentCongestedTicks;
            }
        }

        ticksOwed++;
        if (!tickPending) {
            tickPending = 1;
            a2dpPostMessage(MSG_TICK);
        }
    }

    return 0;
}

void a2dpInit(void)
{
    if (btaId) {
        return;
    }

    // find a subsystem id nobody uses and register our handler there
    for (int id = BTA_SYS_MAX_ID; id > 0; id--) {
        if (BTA_SYS_REG_TABLE[id] == 0) {
            btaReg = (uint32_t*) GKI_getbuf(8);
            if (!btaReg) {
                return;
            }

            btaReg[0] = (uint32_t) btaHandler;
            btaReg[1] = 0;
            BTA_SYS_REG_TABLE[id] = (uint32_t) btaReg;
            btaId = id;
            break;
        }
    }
}

int a2dpStart(const A2dpParams* p)
{
    if (!btaId || p->frameLength == 0 || p->frameLength > MAX_FRAME_LENGTH || p->samplesPerFrame == 0) {
        return 0;
    }

    // one packet must hold at least one frame, otherwise nothing could ever be sent
    if (p->mtu < RTP_HEADER_SIZE + 1 + p->frameLength) {
        return 0;
    }

    a2dpStop();

    params = *p;
    ringHead = 0;
    ringTail = 0;
    primed = 0;
    credit = 0;
    ticksOwed = 0;
    tickPending = 0;
    congested = 0;
    rtpSequence = 0;
    rtpTimestamp = 0;
    sentFrames = 0;
    underruns = 0;
    congestions = 0;
    congestedTicks = 0;
    currentCongestedTicks = 0;
    maxCongestedTicks = 0;
    droppedFrames = 0;

    if (!threadStarted) {
        queueId = IOS_CreateMessageQueue(&queueStorage, 1);
        threadStack = IOS_AllocAligned(LOCAL_PROCESS_HEAP_ID, THREAD_STACK_SIZE, 0x20);
        if (queueId < 0 || !threadStack) {
            return 0;
        }

        int thread = IOS_CreateThread(timerThread, NULL, (uint8_t*) threadStack + THREAD_STACK_SIZE,
            THREAD_STACK_SIZE, IOS_GetThreadPriority(0), 0);
        if (thread < 0) {
            return 0;
        }

        IOS_StartThread(thread);
        threadStarted = 1;
    }

    active = 1;
    timerId = IOS_CreateTimer(TICK_US, TICK_US, queueId, TICK_MESSAGE);
    return 1;
}

void a2dpStop(void)
{
    if (!active) {
        return;
    }

    active = 0;
    IOS_DestroyTimer(timerId);
}

uint32_t a2dpWrite(const uint8_t* data, uint32_t length)
{
    if (!active || params.frameLength == 0) {
        return 0;
    }

    uint32_t count = length / params.frameLength;
    uint32_t taken = 0;
    while (taken < count && ringCount() < A2DP_RING_FRAMES) {
        memcpy(frames[ringHead % A2DP_RING_FRAMES], data + taken * params.frameLength, params.frameLength);
        ringHead++;
        taken++;
    }

    return taken;
}

void a2dpSetCongested(uint8_t value)
{
    if (value && !congested) {
        congestions++;
        currentCongestedTicks = 0;
    }
    congested = value;
}

void a2dpPostStopRequest(void)
{
    a2dpPostMessage(A2DP_MSG_STOP);
}

void a2dpGetStats(A2dpStats* stats)
{
    stats->queued = ringCount();
    stats->sent = sentFrames;
    stats->underruns = underruns;
    stats->congestions = congestions;
    stats->congestedMs = congestedTicks * (TICK_US / 1000);
    stats->maxCongestedMs = maxCongestedTicks * (TICK_US / 1000);
    stats->droppedFrames = droppedFrames;
}
