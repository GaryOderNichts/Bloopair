/*
 *   Bluetooth audio connection test (BTAudio)
 *
 *   Once armed, the next finished device search (sync button) connects to the best audio
 *   device found by the scan: pairing, L2CAP channel on the AVDTP PSM (0x19), AVDTP discover /
 *   get capabilities / set configuration / open, a second channel for the media and start.
 *   The PPC side then writes SBC frames, a2dp.c sends them.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#include "audio.h"
#include "scan.h"
#include "a2dp.h"
#include "l2cap.h"
#include "hfp.h"
#include "stack/btm.h"

#define AVDTP_PSM               0x0019
#define AVDTP_SEC_SERVICE_ID    35
#define AVDTP_LOCAL_MTU         672
#define TX_OFFSET               32      // generous space in front of the payload for headers
#define BT_HDR_SIZE             8

// functions of the Bluetooth stack in IOS-PAD (addresses in imports.sym)
extern void BTM_SetPairableMode(uint8_t allow_pairing, uint8_t connect_only_paired);
extern uint8_t BTM_SecBond(uint8_t* p_bd_addr, uint8_t pin_len, uint8_t* p_pin, uint32_t* trusted_mask);
extern uint8_t BTM_SetSecurityLevel(uint8_t is_originator, const char* p_name, uint8_t service_id,
                                    uint16_t sec_level, uint16_t psm, uint32_t mx_proto_id, uint32_t mx_chan_id);

// security events of the Bluetooth application layer (BTA_DM_SP_CFM_REQ_EVT is in bt_api.h)
#define BTA_DM_AUTH_CMPL_EVT    3
#define BTA_DM_LINK_DOWN_EVT    6

#define BTA_DM_LINK_UP_EVT      5

// state of the security manager (addresses derived from the IOS-PAD binary)
#define BTM_PAIRING_DISABLED    (*(volatile uint8_t*) 0x1214fe79)
#define BTM_PAIRING_STATE       (*(volatile uint8_t*) 0x1214fe8f)

#define BTM_LOCAL_IO_CAPS       (*(volatile uint8_t*) 0x1214e5c9)

// pointer to the simple pairing callback of the security manager (btm_cb.api.p_sp_callback)
#define BTM_SP_CALLBACK         (*(volatile uint32_t*) 0x1214fe3c)

// start of tBTA_DM_AUTH_CMPL (layout confirmed in the IOS-PAD binary)
typedef struct {
    BD_ADDR bd_addr;
    BD_NAME bd_name;
    uint8_t key_present;
    LINK_KEY key;
    uint8_t key_type;
    uint8_t success;
    uint8_t fail_reason;
} AuthCmpl;


static BloopairAudioStatus status;
static uint8_t l2capRegistered;
static uint8_t cfgLocalDone;
static uint8_t cfgRemoteDone;
static uint8_t searchServices;
static uint8_t bondAttempts;
static uint8_t retryPending;     // waiting for the failed link to go down before pairing again

static void logEvent(uint16_t event, uint16_t value, uint32_t data)
{
    if (status.logCount >= BLOOPAIR_AUDIO_LOG_SIZE) {
        return;
    }

    BloopairAudioLogEntry* entry = &status.log[status.logCount];
    entry->event = event;
    entry->value = value;
    entry->data = data;
    status.logCount++;
}

void audioLogEvent(uint16_t event, uint16_t value, uint32_t data)
{
    logEvent(event, value, data);
}

void audioLogDisconnectCall(uint8_t reason, uint32_t caller)
{
    if (!status.enabled || status.state == BLOOPAIR_AUDIO_STATE_IDLE || status.state == BLOOPAIR_AUDIO_STATE_ARMED) {
        return;
    }

    logEvent(BLOOPAIR_AUDIO_EV_HCI_DISC_CALL, reason, caller);
}

void audioLogHci(uint8_t kind, uint16_t code, const uint8_t* params, uint32_t len)
{
    if (!status.enabled || status.state == BLOOPAIR_AUDIO_STATE_IDLE || status.state == BLOOPAIR_AUDIO_STATE_ARMED) {
        return;
    }

    if (status.state == BLOOPAIR_AUDIO_STATE_STREAMING && kind != BLOOPAIR_AUDIO_EV_HCI_EVT && kind != BLOOPAIR_AUDIO_EV_HCI_CMD) {
        return;     // the media packets would fill the log
    }

    if (kind == BLOOPAIR_AUDIO_EV_HCI_EVT) {
        // inquiry results, rssi results, number of completed packets: too many and not interesting
        if (code == 0x02 || code == 0x13 || code == 0x22 || code == 0x2F) {
            return;
        }
    }

    // link keys are secrets which end up in the log file: only keep the address
    uint32_t shown = len;
    if ((kind == BLOOPAIR_AUDIO_EV_HCI_EVT && code == 0x18) || (kind == BLOOPAIR_AUDIO_EV_HCI_CMD && code == 0x040B)) {
        shown = 6;
    }
    if (shown > 12) {
        shown = 12;
    }
    if (len > 255) {
        len = 255;
    }

    for (uint32_t i = 0; i < 3 && i * 4 < shown; i++) {
        uint32_t word = 0;
        for (uint32_t j = 0; j < 4; j++) {
            uint32_t idx = i * 4 + j;
            word = (word << 8) | (idx < shown ? params[idx] : 0);
        }

        if (i == 0) {
            logEvent(kind | (len << 8), code, word);
        } else {
            logEvent(BLOOPAIR_AUDIO_EV_HCI_MORE, i, word);
        }
    }

    if (shown == 0) {
        logEvent(kind | (len << 8), code, 0);
    }
}

// 1: pair as part of the AVDTP connection (general bonding), 0: pair first with BTM_SecBond (dedicated bonding)
#define AUDIO_GENERAL_BONDING   1

// io capabilities and MITM bit used for each pairing attempt (dedicated bonding only).
// Tests so far: {1,1} (stack default) got an answer from the device but pairing failed afterwards,
// {3,0} was not answered by the device at all.
#define IO_PROFILE_COUNT    3
static const uint8_t ioProfiles[IO_PROFILE_COUNT][2] = {
    { 1, 0 },   // display yes/no, no MITM
    { 0, 0 },   // display only, no MITM
    { 2, 0 },   // keyboard only, no MITM
};

typedef uint8_t (*SpCallback)(uint32_t event, void* p_data);
static SpCallback realSpCallback;

static uint8_t spCallbackHook(uint32_t event, void* p_data)
{
    uint8_t ev = event & 0xFF;
    uint8_t* p = (uint8_t*) p_data;

    // let the original code do its work first, the answer to the controller is made from these values
    uint8_t res = realSpCallback ? realSpCallback(event, p_data) : 0;

    // the stack asks for MITM protection by default when it starts the pairing, which a device
    // without display and keyboard (earbuds) can not provide. Try gentler settings, one per attempt.
    if (ev == 0 && status.state == BLOOPAIR_AUDIO_STATE_PAIRING &&
        bondAttempts >= 1 && bondAttempts <= IO_PROFILE_COUNT &&
        memcmp(p, status.bd_address, 6) == 0) {
        p[6] = ioProfiles[bondAttempts - 1][0];     // io capabilities
        p[7] = 0;                                   // no out of band data
        p[8] = ioProfiles[bondAttempts - 1][1];     // MITM bit, the stack adds "dedicated bonding"
    }

    if (status.state == BLOOPAIR_AUDIO_STATE_PAIRING || status.state == BLOOPAIR_AUDIO_STATE_CONNECTING) {
        uint32_t info = 0;
        switch (ev) {
        case 0: // IO capabilities request: what we answer (io caps, oob, auth requirements, originator)
            info = (p[6] << 24) | (p[7] << 16) | (p[8] << 8) | p[9];
            break;
        case 1: // IO capabilities response: what the device told us
            info = (p[6] << 24) | (p[7] << 16) | (p[8] << 8);
            break;
        case 2: // user confirmation request
            info = (p[0x50] << 24) | (p[0x51] << 16) | (p[0x52] << 8) | p[0x53];
            break;
        case 8: // simple pairing complete
            info = p[0x4a];
            break;
        }

        logEvent(BLOOPAIR_AUDIO_EV_SP_EVENT, (BTM_PAIRING_STATE << 8) | ev, info);
    }

    return res;
}

static void installSpHook(void)
{
    uint32_t current = BTM_SP_CALLBACK;
    if (current != 0 && current != (uint32_t) spCallbackHook) {
        realSpCallback = (SpCallback) current;
        BTM_SP_CALLBACK = (uint32_t) spCallbackHook;
    }
}

static void removeSpHook(void)
{
    if (BTM_SP_CALLBACK == (uint32_t) spCallbackHook && realSpCallback) {
        BTM_SP_CALLBACK = (uint32_t) realSpCallback;
    }
}

// ---------------------------------------------------------------------------------------------
// AVDTP signalling: discover, get capabilities, set configuration, open, (media channel), start
// ---------------------------------------------------------------------------------------------
#define AVDTP_SIG_DISCOVER      0x01
#define AVDTP_SIG_GET_CAPS      0x02
#define AVDTP_SIG_SET_CONFIG    0x03
#define AVDTP_SIG_OPEN          0x06
#define AVDTP_SIG_START         0x07
#define AVDTP_SIG_CLOSE         0x08
#define AVDTP_SIG_ABORT         0x0A
#define AVDTP_SIG_DELAY_REPORT  0x0D

#define AVDTP_MSG_COMMAND       0
#define AVDTP_MSG_GENERAL_REJECT 1
#define AVDTP_MSG_ACCEPT        2
#define AVDTP_MSG_REJECT        3

#define LOCAL_SEID              1
#define SBC_FRAME_MAX           200     // size of a slot in the ring of a2dp.c
#define SBC_TARGET_BITPOOL      53      // "high quality" at 44.1 kHz

static uint8_t sigConnected;        // the signalling channel (status.cid) is up
static uint8_t avdtpWait;           // signal id of the command we wait for an answer to
static uint8_t avdtpLabel;
static uint8_t seidList[8];
static uint8_t seidCount;
static uint8_t seidIndex;
static uint8_t remoteSeid;
static uint8_t codecConfig[4];      // the four SBC bytes of the set configuration command
static uint32_t sampleRate;
static uint16_t mediaCid;
static uint16_t mediaMtu;
static uint8_t mediaCfgLocal;
static uint8_t mediaCfgRemote;
static uint8_t batteryWanted;       // also read the battery over the hands-free profile

static void closeChannels(void)
{
    a2dpStop();
    hfpStop();

    if (mediaCid) {
        uint16_t cid = mediaCid;
        mediaCid = 0;
        L2CA_DisconnectReq(cid);
        logEvent(BLOOPAIR_AUDIO_EV_DISCONNECT_REQ, cid, 0);
    }

    if (sigConnected) {
        sigConnected = 0;
        L2CA_DisconnectReq(status.cid);
        logEvent(BLOOPAIR_AUDIO_EV_DISCONNECT_REQ, status.cid, 0);
    }
}

static void fail(void)
{
    status.state = BLOOPAIR_AUDIO_STATE_FAILED;
    status.enabled = 0;
    removeSpHook();
    closeChannels();
}

static uint32_t firstBytes(const uint8_t* d, uint16_t len)
{
    uint32_t v = 0;
    for (uint16_t i = 0; i < 4; i++) {
        v = (v << 8) | (i < len ? d[i] : 0);
    }
    return v;
}

static BT_HDR* allocSignal(uint16_t cid, uint16_t len, uint8_t** outData)
{
    BT_HDR* p_buf = (BT_HDR*) GKI_getbuf(BT_HDR_SIZE + TX_OFFSET + len);
    if (!p_buf) {
        logEvent(BLOOPAIR_AUDIO_EV_NO_BUFFER, cid, 0);
        return NULL;
    }

    p_buf->event = 0;
    p_buf->layer_specific = 0;
    p_buf->offset = TX_OFFSET;
    p_buf->len = len;
    *outData = ((uint8_t*) p_buf) + BT_HDR_SIZE + TX_OFFSET;
    return p_buf;
}

// send an AVDTP command on the signalling channel and remember which answer we wait for
static void sendCommand(uint8_t sig, const uint8_t* payload, uint16_t len)
{
    uint8_t* d;
    BT_HDR* p_buf = allocSignal(status.cid, 2 + len, &d);
    if (!p_buf) {
        fail();
        return;
    }

    d[0] = (avdtpLabel & 0x0F) << 4;    // transaction label, single packet, command
    d[1] = sig;
    avdtpLabel++;
    memcpy(d + 2, payload, len);

    avdtpWait = sig;
    logEvent(BLOOPAIR_AUDIO_EV_AVDTP_CMD, sig, firstBytes(payload, len));
    uint8_t res = L2CA_DataWrite(status.cid, p_buf);
    if (sig == AVDTP_SIG_DISCOVER) {
        logEvent(BLOOPAIR_AUDIO_EV_DISCOVER_SENT, status.cid, res);
    }
}

// answer a command of the device (header only)
static void sendReply(uint8_t label, uint8_t msgType, uint8_t sig)
{
    uint8_t* d;
    BT_HDR* p_buf = allocSignal(status.cid, 2, &d);
    if (!p_buf) {
        return;
    }

    d[0] = (label << 4) | msgType;
    d[1] = sig;
    L2CA_DataWrite(status.cid, p_buf);
}

static void sendGetCaps(void)
{
    remoteSeid = seidList[seidIndex];
    uint8_t payload = remoteSeid << 2;
    sendCommand(AVDTP_SIG_GET_CAPS, &payload, 1);
}

static uint16_t sbcFrameLength(uint8_t mode, uint8_t blocks, uint8_t subbands, uint8_t bitpool)
{
    uint32_t channels = (mode == 0) ? 1 : 2;
    uint32_t base = 4 + (4 * subbands * channels) / 8;
    switch (mode) {
    case 0:
    case 1:
        return base + (blocks * channels * bitpool + 7) / 8;
    case 2:
        return base + (blocks * bitpool + 7) / 8;
    default:
        return base + (subbands + blocks * bitpool + 7) / 8;
    }
}

// pick settings from what the device supports (caps = the four SBC capability bytes)
// lower bitpool chosen by the user (quality setting), 0 = no limit
static uint8_t bitpoolLimit;

static int chooseConfig(const uint8_t* caps)
{
    uint8_t freqBit, modeBit, blockBit, subBit, allocBit;
    uint8_t freq, mode, blocks, subbands, alloc;

    // 48 kHz is what the console's own audio runs at, no conversion needed
    if (caps[0] & 0x10)      { freqBit = 0x10; freq = 3; sampleRate = 48000; }
    else if (caps[0] & 0x20) { freqBit = 0x20; freq = 2; sampleRate = 44100; }
    else if (caps[0] & 0x40) { freqBit = 0x40; freq = 1; sampleRate = 32000; }
    else return 0;

    if (caps[0] & 0x01)      { modeBit = 0x01; mode = 3; }
    else if (caps[0] & 0x02) { modeBit = 0x02; mode = 2; }
    else return 0;

    if (caps[1] & 0x10)      { blockBit = 0x10; blocks = 16; }
    else if (caps[1] & 0x20) { blockBit = 0x20; blocks = 12; }
    else if (caps[1] & 0x40) { blockBit = 0x40; blocks = 8; }
    else if (caps[1] & 0x80) { blockBit = 0x80; blocks = 4; }
    else return 0;

    if (caps[1] & 0x04)      { subBit = 0x04; subbands = 8; }
    else if (caps[1] & 0x08) { subBit = 0x08; subbands = 4; }
    else return 0;

    if (caps[1] & 0x01)      { allocBit = 0x01; alloc = 0; }
    else if (caps[1] & 0x02) { allocBit = 0x02; alloc = 1; }
    else return 0;

    uint8_t minPool = caps[2];
    uint8_t maxPool = caps[3];
    if (minPool > maxPool || maxPool < 2) {
        return 0;
    }

    uint8_t bitpool = SBC_TARGET_BITPOOL;
    if (bitpoolLimit && bitpool > bitpoolLimit) bitpool = bitpoolLimit;
    if (bitpool > maxPool) bitpool = maxPool;
    if (bitpool < minPool) bitpool = minPool;
    while (bitpool > minPool && sbcFrameLength(mode, blocks, subbands, bitpool) > SBC_FRAME_MAX) {
        bitpool--;
    }

    uint16_t frameLength = sbcFrameLength(mode, blocks, subbands, bitpool);
    if (frameLength > SBC_FRAME_MAX) {
        return 0;
    }

    codecConfig[0] = freqBit | modeBit;
    codecConfig[1] = blockBit | subBit | allocBit;
    codecConfig[2] = minPool;
    codecConfig[3] = bitpool;

    status.sbcFrequency = freq;
    status.sbcMode = mode;
    status.sbcBlocks = blocks;
    status.sbcSubbands = subbands;
    status.sbcAllocation = alloc;
    status.sbcBitpool = bitpool;
    status.sbcFrameLength = frameLength;

    logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 2, freq | (mode << 8) | (bitpool << 16) | (blocks << 24));
    logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 4, frameLength);
    return 1;
}

static void sendSetConfig(void)
{
    uint8_t payload[14];
    payload[0] = remoteSeid << 2;
    payload[1] = LOCAL_SEID << 2;
    payload[2] = 0x01;      // media transport, no parameters
    payload[3] = 0x00;
    payload[4] = 0x07;      // media codec, 6 bytes
    payload[5] = 0x06;
    payload[6] = 0x00;      // audio
    payload[7] = 0x00;      // SBC
    memcpy(payload + 8, codecConfig, 4);
    sendCommand(AVDTP_SIG_SET_CONFIG, payload, 12);
}

static void startConfig(uint16_t cid)
{
    L2capCfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    CFG_MTU_PRESENT(&cfg) = 1;
    CFG_MTU(&cfg) = AVDTP_LOCAL_MTU;

    uint8_t cfgRes = L2CA_ConfigReq(cid, &cfg);
    logEvent(BLOOPAIR_AUDIO_EV_CONFIG_REQ, cid, cfgRes);
}

// the signalling channel is open: find the device's endpoints
static void checkChannelOpen(uint16_t cid)
{
    if (!cfgLocalDone || !cfgRemoteDone || status.state != BLOOPAIR_AUDIO_STATE_CONFIGURING) {
        return;
    }

    status.state = BLOOPAIR_AUDIO_STATE_OPEN;
    logEvent(BLOOPAIR_AUDIO_EV_CHANNEL_OPEN, cid, 0);
    sendCommand(AVDTP_SIG_DISCOVER, NULL, 0);
}

// the media channel is open: start the stream
static void checkMediaOpen(void)
{
    if (!mediaCfgLocal || !mediaCfgRemote) {
        return;
    }

    logEvent(BLOOPAIR_AUDIO_EV_CHANNEL_OPEN, mediaCid, 1);
    uint8_t payload = remoteSeid << 2;
    sendCommand(AVDTP_SIG_START, &payload, 1);
}

static void openMediaChannel(void)
{
    mediaCfgLocal = 0;
    mediaCfgRemote = 0;
    mediaCid = L2CA_ConnectReq(AVDTP_PSM, status.bd_address) & 0xFFFF;
    logEvent(BLOOPAIR_AUDIO_EV_CONNECT_REQ, mediaCid, 1);
    if (mediaCid == 0) {
        fail();
    }
}

static void handleSignal(const uint8_t* d, uint16_t len)
{
    if (len < 2) {
        return;
    }

    uint8_t label = d[0] >> 4;
    uint8_t packetType = (d[0] >> 2) & 3;
    uint8_t msgType = d[0] & 3;
    uint8_t sig = d[1] & 0x3F;
    logEvent(BLOOPAIR_AUDIO_EV_AVDTP_RSP, sig | (msgType << 8), firstBytes(d + 2, len - 2));

    if (packetType != 0) {
        return;     // fragmented messages are not expected
    }

    if (msgType == AVDTP_MSG_COMMAND) {
        // the device asks something: delay reports are harmless, close / abort mean the end
        if (sig == AVDTP_SIG_DELAY_REPORT) {
            sendReply(label, AVDTP_MSG_ACCEPT, sig);
        } else if (sig == AVDTP_SIG_CLOSE || sig == AVDTP_SIG_ABORT) {
            sendReply(label, AVDTP_MSG_ACCEPT, sig);
            logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 6, 1);
            a2dpStop();
        } else {
            sendReply(label, AVDTP_MSG_GENERAL_REJECT, sig);
        }
        return;
    }

    if (sig != avdtpWait) {
        return;
    }
    avdtpWait = 0;

    if (msgType != AVDTP_MSG_ACCEPT) {
        // rejected: the error code follows, keep going with the next endpoint if we were still looking
        if (sig == AVDTP_SIG_GET_CAPS && ++seidIndex < seidCount) {
            sendGetCaps();
        } else if (sig == AVDTP_SIG_CLOSE) {
            closeChannels();
            status.state = BLOOPAIR_AUDIO_STATE_CLOSED;
        } else {
            fail();
        }
        return;
    }

    switch (sig) {
    case AVDTP_SIG_DISCOVER:
        // keep the raw answer for the log
        status.responseLength = len;
        memcpy(status.response, d, len > BLOOPAIR_AUDIO_RESP_SIZE ? BLOOPAIR_AUDIO_RESP_SIZE : len);

        seidCount = 0;
        for (uint16_t i = 2; i + 1 < len && seidCount < sizeof(seidList); i += 2) {
            uint8_t seid = d[i] >> 2;
            uint8_t inUse = (d[i] >> 1) & 1;
            uint8_t media = d[i + 1] >> 4;
            uint8_t sink = (d[i + 1] >> 3) & 1;
            logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 1, seid | (inUse << 8) | (sink << 16) | (media << 24));
            if (!inUse && media == 0 && sink) {
                seidList[seidCount++] = seid;
            }
        }

        if (seidCount == 0) {
            fail();
        } else {
            seidIndex = 0;
            sendGetCaps();
        }
        break;

    case AVDTP_SIG_GET_CAPS: {
        // look for the media codec capability of type SBC
        const uint8_t* caps = NULL;
        for (uint16_t i = 2; i + 1 < len;) {
            uint8_t category = d[i];
            uint8_t catLen = d[i + 1];
            if (i + 2 + catLen > len) {
                break;
            }
            if (category == 0x07 && catLen >= 6 && (d[i + 2] >> 4) == 0 && d[i + 3] == 0x00) {
                caps = d + i + 4;
                break;
            }
            i += 2 + catLen;
        }

        if (caps) {
            logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 3, firstBytes(caps, 4));
        }

        if (caps && chooseConfig(caps)) {
            sendSetConfig();
        } else if (++seidIndex < seidCount) {
            sendGetCaps();
        } else {
            fail();     // no endpoint with usable SBC
        }
        break;
    }

    case AVDTP_SIG_SET_CONFIG: {
        uint8_t payload = remoteSeid << 2;
        sendCommand(AVDTP_SIG_OPEN, &payload, 1);
        break;
    }

    case AVDTP_SIG_OPEN:
        openMediaChannel();
        break;

    case AVDTP_SIG_START: {
        A2dpParams params;
        params.cid = mediaCid;
        params.mtu = mediaMtu;
        params.frameLength = status.sbcFrameLength;
        params.samplesPerFrame = status.sbcBlocks * status.sbcSubbands;
        params.sampleRate = sampleRate;

        status.mediaMtu = mediaMtu;
        logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 5, mediaMtu);
        if (!a2dpStart(&params)) {
            fail();     // for example a media channel too small for one SBC frame
            break;
        }
        status.state = BLOOPAIR_AUDIO_STATE_STREAMING;

        // the sound runs: now the battery level, if the user wants it (a failure there changes nothing here).
        // The connection used most of the event log, the PPC side has read it by now: start a fresh one.
        if (batteryWanted) {
            status.logCount = 0;
            status.logGeneration++;
            hfpStart(status.bd_address);
        }
        break;
    }

    case AVDTP_SIG_CLOSE:
        closeChannels();
        status.state = BLOOPAIR_AUDIO_STATE_CLOSED;
        status.enabled = 0;
        removeSpHook();
        break;
    }
}

// ---------------------------------------------------------------------------------------------
// L2CAP callbacks
// ---------------------------------------------------------------------------------------------
static void cbConnectInd(uint8_t* bd_addr, uint32_t lcid, uint32_t psm, uint32_t id)
{
    // the console always starts the connection itself: log and refuse what a device starts on its own
    logEvent(BLOOPAIR_AUDIO_EV_CONNECT_IND, psm & 0xFFFF, lcid & 0xFFFF);
    L2CA_ConnectRsp(bd_addr, id & 0xFF, lcid & 0xFFFF, L2CAP_CONN_NO_RESOURCES, 0);
}

static void cbConnectCfm(uint32_t lcid, uint32_t result)
{
    uint16_t cid = lcid & 0xFFFF;
    uint16_t res = result & 0xFFFF;
    logEvent(BLOOPAIR_AUDIO_EV_CONNECT_CFM, cid, res);

    if (res != 0) {
        fail();
        return;
    }

    if (mediaCid && cid == mediaCid) {
        startConfig(cid);
        return;
    }

    status.cid = cid;
    sigConnected = 1;
    status.state = BLOOPAIR_AUDIO_STATE_CONFIGURING;
    cfgLocalDone = 0;
    cfgRemoteDone = 0;
    startConfig(cid);
}

static void cbConfigInd(uint32_t lcid, L2capCfg* cfg)
{
    uint16_t cid = lcid & 0xFFFF;
    uint16_t mtu = CFG_MTU_PRESENT(cfg) ? CFG_MTU(cfg) : 672;
    logEvent(BLOOPAIR_AUDIO_EV_CONFIG_IND, cid, CFG_MTU_PRESENT(cfg) ? CFG_MTU(cfg) : 0);

    // accept the remote configuration as it is
    CFG_RESULT(cfg) = 0;
    CFG_MTU_PRESENT(cfg) = 0;
    CFG_FLUSH_PRESENT(cfg) = 0;
    L2CA_ConfigRsp(cid, cfg);

    if (mediaCid && cid == mediaCid) {
        mediaMtu = mtu;
        mediaCfgRemote = 1;
        checkMediaOpen();
        return;
    }

    cfgRemoteDone = 1;
    checkChannelOpen(cid);
}

static void cbConfigCfm(uint32_t lcid, L2capCfg* cfg)
{
    uint16_t cid = lcid & 0xFFFF;
    uint16_t res = CFG_RESULT(cfg);
    logEvent(BLOOPAIR_AUDIO_EV_CONFIG_CFM, cid, res);

    if (res != 0) {
        fail();
        return;
    }

    if (mediaCid && cid == mediaCid) {
        mediaCfgLocal = 1;
        checkMediaOpen();
        return;
    }

    cfgLocalDone = 1;
    checkChannelOpen(cid);
}

static void cbDisconnectInd(uint32_t lcid, uint32_t ackNeeded)
{
    uint16_t cid = lcid & 0xFFFF;
    logEvent(BLOOPAIR_AUDIO_EV_DISCONNECT_IND, cid, ackNeeded & 0xFF);

    if (ackNeeded & 0xFF) {
        L2CA_DisconnectRsp(cid);
    }

    // one channel gone ends the whole connection
    if (mediaCid && cid == mediaCid) {
        mediaCid = 0;
    } else if (cid == status.cid) {
        sigConnected = 0;
    }
    closeChannels();

    if (status.state != BLOOPAIR_AUDIO_STATE_FAILED && status.state != BLOOPAIR_AUDIO_STATE_CLOSED) {
        status.state = BLOOPAIR_AUDIO_STATE_CLOSED;
        status.enabled = 0;
        removeSpHook();
    }
}

static void cbDisconnectCfm(uint32_t lcid, uint32_t result)
{
    logEvent(BLOOPAIR_AUDIO_EV_DISCONNECT_CFM, lcid & 0xFFFF, result & 0xFFFF);
}

static void cbDataInd(uint32_t lcid, BT_HDR* p_buf)
{
    uint16_t cid = lcid & 0xFFFF;
    if (cid == status.cid) {
        handleSignal(((uint8_t*) p_buf) + BT_HDR_SIZE + p_buf->offset, p_buf->len);
    }

    GKI_freebuf(p_buf);
}

static void cbCongestion(uint32_t lcid, uint32_t congested)
{
    if (mediaCid && (lcid & 0xFFFF) == mediaCid) {
        a2dpSetCongested(congested & 0xFF);
    }
}

static const L2capApplInfo applInfo = {
    cbConnectInd,
    cbConnectCfm,
    NULL,
    cbConfigInd,
    cbConfigCfm,
    cbDisconnectInd,
    cbDisconnectCfm,
    NULL,
    cbDataInd,
    cbCongestion,
    NULL,
};

// open the L2CAP channel on the AVDTP psm (the device has to be paired already)
static void connectL2cap(void)
{
    // tell the security manager how to treat connections to the AVDTP psm
    uint8_t secRes = BTM_SetSecurityLevel(1, "AVDTP", AVDTP_SEC_SERVICE_ID,
        BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_OUT_ENCRYPT, AVDTP_PSM, 0, 0);
    logEvent(BLOOPAIR_AUDIO_EV_SECURITY, 0, secRes);

    if (!l2capRegistered) {
        uint16_t psm = L2CA_Register(AVDTP_PSM, &applInfo) & 0xFFFF;
        logEvent(BLOOPAIR_AUDIO_EV_L2CAP_REGISTER, psm, 0);
        if (psm == 0) {
            fail();
            return;
        }

        l2capRegistered = 1;
    }

    status.state = BLOOPAIR_AUDIO_STATE_CONNECTING;
    uint16_t cid = L2CA_ConnectReq(AVDTP_PSM, status.bd_address) & 0xFFFF;
    status.cid = cid;
    logEvent(BLOOPAIR_AUDIO_EV_CONNECT_REQ, cid, 0);
    if (cid == 0) {
        fail();
    }
}

// start pairing with the target device
static void startBond(void)
{
    retryPending = 0;
    bondAttempts++;
    uint8_t bondRes = BTM_SecBond(status.bd_address, 0, NULL, NULL);
    logEvent(BLOOPAIR_AUDIO_EV_BOND_REQ, 0, bondRes);

    if (bondRes == 0) {
        // already paired
        connectL2cap();
    } else if (bondRes != 1) {
        fail();
    }

    // otherwise wait for the pairing result, see audioOnSecurityEvent
}

// sec_flags bits of a device record of the security manager
#define BTM_SEC_LINK_KEY_KNOWN  0x10
#define BTM_SEC_LINK_KEY_AUTHED 0x20

// Pairing a device again: the security manager may still know the link key of an earlier connection
// in this session. It would answer the key request with that key, no new key comes out of the pairing
// and the plugin has nothing to save. Make it pair from scratch.
static void forgetStackKey(void)
{
    tBTM_SEC_DEV_REC* dev = btm_find_dev(status.bd_address);
    if (dev && (dev->sec_flags & BTM_SEC_LINK_KEY_KNOWN)) {
        dev->sec_flags &= ~(BTM_SEC_LINK_KEY_KNOWN | BTM_SEC_LINK_KEY_AUTHED);
        logEvent(BLOOPAIR_AUDIO_EV_KEY_ADD, 2, 0);
    }
}

// connect to status.bd_address (pairs on the way if the device is not known yet)
static void beginConnect(void)
{
    logEvent(BLOOPAIR_AUDIO_EV_TARGET, (status.bd_address[0] << 8) | status.bd_address[1],
        (status.bd_address[2] << 24) | (status.bd_address[3] << 16) | (status.bd_address[4] << 8) | status.bd_address[5]);

    // make sure the stack accepts pairing, then pair like a phone would (dedicated bonding)
    BTM_SetPairableMode(1, 0);
    logEvent(BLOOPAIR_AUDIO_EV_PAIRABLE, 0, 0);

    // observe the simple pairing exchange
    logEvent(BLOOPAIR_AUDIO_EV_LOCAL_IO, BTM_LOCAL_IO_CAPS, 0);
    installSpHook();

    bondAttempts = 0;
    retryPending = 0;
    sigConnected = 0;
    mediaCid = 0;
    avdtpWait = 0;
    avdtpLabel = 0;
    a2dpInit();

#if AUDIO_GENERAL_BONDING
    // connect straight away: the security manager pairs on demand while the channel is set up and the
    // link stays in use (after a dedicated bonding the stack hangs up and the earbuds drop the link)
    connectL2cap();
#else
    status.state = BLOOPAIR_AUDIO_STATE_PAIRING;
    startBond();
#endif
}

// armed test: the search is over, take the best audio device of the scan
static void startConnect(void)
{
    uint8_t addr[6];
    if (!scanFindFirstAudio(addr)) {
        logEvent(BLOOPAIR_AUDIO_EV_NO_TARGET, 0, 0);
        fail();
        return;
    }

    memcpy(status.bd_address, addr, 6);
    forgetStackKey();
    beginConnect();
}

static int audioBusy(void)
{
    return status.state == BLOOPAIR_AUDIO_STATE_PAIRING || status.state == BLOOPAIR_AUDIO_STATE_CONNECTING ||
           status.state == BLOOPAIR_AUDIO_STATE_CONFIGURING || status.state == BLOOPAIR_AUDIO_STATE_OPEN ||
           status.state == BLOOPAIR_AUDIO_STATE_STREAMING;
}

static BloopairAudioConnectData pendingConnect;

// IPC thread: only remembers the request, the work is done in the stack's context
void audioRequestConnect(const BloopairAudioConnectData* request)
{
    if (audioBusy()) {
        return;
    }

    pendingConnect = *request;
    bitpoolLimit = request->maxBitpool;
    batteryWanted = request->battery;
    a2dpInit();
    a2dpPostMessage(A2DP_MSG_CONNECT);
}

void audioRequestDisconnect(void)
{
    if (audioBusy()) {
        a2dpPostMessage(A2DP_MSG_STOP);
    }
}

void audioHandleMessage(uint8_t code)
{
    switch (code) {
    case A2DP_MSG_STOP:
        audioStopRequested();
        break;

    case A2DP_MSG_CONNECT:
        if (audioBusy()) {
            break;
        }

        memset(&status, 0, sizeof(status));
        status.enabled = 1;
        status.state = BLOOPAIR_AUDIO_STATE_CONNECTING;
        memcpy(status.bd_address, pendingConnect.bd_address, 6);
        if (pendingConnect.hasKey) {
            // hand the stored link key to the security manager, it arrives in the stack through the BTA queue
            logEvent(BLOOPAIR_AUDIO_EV_KEY_ADD, 0, pendingConnect.keyType);
            BTA_DmAddDevice(status.bd_address, NULL, pendingConnect.linkKey, 0, 0, pendingConnect.keyType, 0);
            a2dpPostMessage(A2DP_MSG_CONNECT2);
        } else {
            forgetStackKey();
            beginConnect();
        }
        break;

    case A2DP_MSG_CONNECT2:
        logEvent(BLOOPAIR_AUDIO_EV_KEY_ADD, 1, 0);
        if (status.state == BLOOPAIR_AUDIO_STATE_CONNECTING) {
            beginConnect();
        }
        break;
    }
}

void audioArmTest(uint8_t enabled)
{
    if (enabled) {
        a2dpStop();
        memset(&status, 0, sizeof(status));
        status.enabled = 1;
        status.state = BLOOPAIR_AUDIO_STATE_ARMED;
        return;
    }

    // a running connection has to be closed from the stack's own context
    if (status.state == BLOOPAIR_AUDIO_STATE_STREAMING || status.state == BLOOPAIR_AUDIO_STATE_OPEN ||
        status.state == BLOOPAIR_AUDIO_STATE_CONFIGURING || status.state == BLOOPAIR_AUDIO_STATE_CONNECTING) {
        a2dpPostStopRequest();
        return;
    }

    status.enabled = 0;
    removeSpHook();
    if (status.state == BLOOPAIR_AUDIO_STATE_ARMED) {
        status.state = BLOOPAIR_AUDIO_STATE_IDLE;
    }
}

// runs in the Bluetooth application task after audioArmTest(0) while a connection exists
void audioStopRequested(void)
{
    a2dpStop();
    if (status.state == BLOOPAIR_AUDIO_STATE_STREAMING && sigConnected) {
        logEvent(BLOOPAIR_AUDIO_EV_AVDTP_INFO, 6, 0);
        uint8_t payload = remoteSeid << 2;
        sendCommand(AVDTP_SIG_CLOSE, &payload, 1);
        return;
    }

    closeChannels();
    status.state = BLOOPAIR_AUDIO_STATE_CLOSED;
    status.enabled = 0;
    removeSpHook();
}

int audioWrite(const uint8_t* data, uint32_t length)
{
    if (status.state != BLOOPAIR_AUDIO_STATE_STREAMING) {
        return 0;
    }

    return a2dpWrite(data, length);
}

void audioGetStatus(BloopairAudioStatus* out)
{
    A2dpStats stats;
    a2dpGetStats(&stats);
    status.ringFrames = stats.queued;
    status.ringCapacity = A2DP_RING_FRAMES;
    status.framesSent = stats.sent;
    status.underruns = stats.underruns;
    status.congestions = stats.congestions;
    status.congestedMs = stats.congestedMs;
    status.maxCongestedMs = stats.maxCongestedMs;
    status.droppedFrames = stats.droppedFrames;
    hfpGetStatus(&status);
    memcpy(out, &status, sizeof(status));
}

void audioOnSearchStart(uint8_t services)
{
    searchServices = services;
}

int audioBlockSearch(void)
{
    // the radio must stay free while we pair and connect
    return status.state == BLOOPAIR_AUDIO_STATE_PAIRING ||
           status.state == BLOOPAIR_AUDIO_STATE_CONNECTING ||
           status.state == BLOOPAIR_AUDIO_STATE_CONFIGURING ||
           status.state == BLOOPAIR_AUDIO_STATE_OPEN ||
           status.state == BLOOPAIR_AUDIO_STATE_STREAMING;
}

#define BTA_DM_INQ_RES_EVT_ID   0
#define BTA_DM_DISC_RES_EVT_ID  2
#define BTA_DM_DISC_CMPL_EVT_ID 4

void audioOnSearchEvent(uint8_t event)
{
    if (status.state != BLOOPAIR_AUDIO_STATE_ARMED) {
        return;
    }

    // log how the search progresses, but not every single result
    if (event != BTA_DM_INQ_RES_EVT_ID && event != BTA_DM_DISC_RES_EVT_ID) {
        logEvent(BLOOPAIR_AUDIO_EV_SEARCH_EVENT, event, searchServices);
    }

    // wait until the console is completely done with the search (including the name and
    // service requests it makes to every device), otherwise the radio is still busy.
    // Without service discovery the end of the inquiry is the end of the search.
    uint8_t endEvent = searchServices ? BTA_DM_DISC_CMPL_EVT_ID : BTA_DM_INQ_CMPL_EVT;
    if (event != endEvent) {
        return;
    }

    startConnect();
}

int audioOnSecurityEvent(uint8_t event, void* p_data)
{
    if (status.state < BLOOPAIR_AUDIO_STATE_CONNECTING && status.state != BLOOPAIR_AUDIO_STATE_PAIRING) {
        return 0;
    }

    if (status.state == BLOOPAIR_AUDIO_STATE_FAILED || status.state == BLOOPAIR_AUDIO_STATE_CLOSED) {
        return 0;
    }

    uint32_t info = 0;
    if (event == BTA_DM_SP_CFM_REQ_EVT) {
        const tBTA_DM_SP_CFM_REQ* req = (const tBTA_DM_SP_CFM_REQ*) p_data;
        info = (req->just_works << 24) | (req->loc_io_caps << 16) | (req->rmt_io_caps << 8) | req->rmt_auth_req;
    } else if (event == BTA_DM_LINK_DOWN_EVT) {
        info = ((const uint8_t*) p_data)[6];
    }

    logEvent(BLOOPAIR_AUDIO_EV_SEC_EVENT, event, info);

    if (event == BTA_DM_LINK_UP_EVT || event == BTA_DM_AUTH_CMPL_EVT) {
        logEvent(BLOOPAIR_AUDIO_EV_BTM_STATE, (BTM_PAIRING_DISABLED << 8) | BTM_PAIRING_STATE, event);
    }

    if (event == BTA_DM_LINK_UP_EVT &&
        (status.state == BLOOPAIR_AUDIO_STATE_PAIRING || status.state == BLOOPAIR_AUDIO_STATE_CONNECTING)) {
        // the console switches pairing off again when the search ends, make sure it is on
        // now that the link is up and the pairing is about to begin
        BTM_SetPairableMode(1, 0);
        logEvent(BLOOPAIR_AUDIO_EV_PAIRABLE, 1, 0);
    }

    // The console's own link up handler hangs up on every device it does not know as a controller
    // (BTM_RemoveAcl, disconnect reason 0x13) before the pairing can finish. The link of our audio
    // device must never reach it, neither up nor down.
    if ((event == BTA_DM_LINK_UP_EVT || event == BTA_DM_LINK_DOWN_EVT) &&
        memcmp(p_data, status.bd_address, 6) == 0) {
        if (event == BTA_DM_LINK_DOWN_EVT && retryPending && status.state == BLOOPAIR_AUDIO_STATE_PAIRING) {
            // the failed link is gone, pair again with other settings
            startBond();
        }
        return 1;
    }

    if (event != BTA_DM_AUTH_CMPL_EVT) {
        return 0;
    }

    const AuthCmpl* auth = (const AuthCmpl*) p_data;
    if (memcmp(auth->bd_addr, status.bd_address, 6) != 0) {
        return 0;
    }

    logEvent(BLOOPAIR_AUDIO_EV_AUTH_CMPL, auth->success, auth->fail_reason | (auth->key_present << 8));

    if (auth->success && auth->key_present) {
        memcpy(status.linkKey, auth->key, sizeof(status.linkKey));
        status.keyType = auth->key_type;
        status.keyValid = 1;
    }

    if (status.state == BLOOPAIR_AUDIO_STATE_PAIRING) {
        if (auth->success) {
            connectL2cap();
        } else if (auth->fail_reason == 0x04 && bondAttempts < IO_PROFILE_COUNT) {
            // page timeout: no link was made, try again right away
            logEvent(BLOOPAIR_AUDIO_EV_RETRY, bondAttempts, auth->fail_reason);
            startBond();
        } else if (bondAttempts < IO_PROFILE_COUNT) {
            // pairing failed on an existing link: the stack closes it now, wait until it is gone
            // (a new attempt on the closing link ends with "terminated by local host")
            logEvent(BLOOPAIR_AUDIO_EV_RETRY, bondAttempts, auth->fail_reason);
            retryPending = 1;
        } else {
            fail();
        }
    }

    // this pairing was started by us, the original code should not see it
    return 1;
}
