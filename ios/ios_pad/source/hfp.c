/*
 *   Battery level of the headphones over the hands-free profile (BTAudio)
 *
 *   Headphones report their battery to a phone inside the hands-free profile: as HF indicator (AT+BIEV) or with
 *   the Apple extension (AT+IPHONEACCEV). The console's stack contains the core of RFCOMM, but not the functions
 *   an application needs to use it, so this file brings its own small RFCOMM client on top of L2CAP:
 *   SDP query for the hands-free channel, multiplexer, one data channel with credit based flow control, and an
 *   audio gateway that answers the AT commands of the service level connection. No calls, no audio over it.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 */

#include "hfp.h"
#include "l2cap.h"
#include "audio.h"
#include "stack/sdp.h"

#define RFCOMM_PSM              0x0003
#define HFP_SEC_SERVICE_ID      36
#define HFP_LOCAL_MTU           672
#define TX_OFFSET               32
#define BT_HDR_SIZE             8

#define UUID_HANDSFREE          0x111E
#define UUID_RFCOMM             0x0003
#define ATTR_SERVICE_CLASS_LIST 0x0001  // the stack finds a service by this one
#define ATTR_PROTOCOL_LIST      0x0004

// SDP attribute types (upper 4 bits of attr_len_type)
#define SDP_TYPE_UINT           1
#define SDP_TYPE_UUID           3
#define SDP_TYPE_SEQ            6
#define SDP_ATTR_TYPE(a)        ((a)->attr_len_type >> 12)

// RFCOMM frame types (control field, poll / final bit 0x10)
#define RFC_SABM                0x2F
#define RFC_UA                  0x63
#define RFC_DM                  0x0F
#define RFC_DISC                0x43
#define RFC_UIH                 0xEF
#define RFC_PF                  0x10

// multiplexer commands on channel 0 (type byte without C/R and EA bits)
#define MCC_PN                  0x20
#define MCC_MSC                 0x38

#define RFC_FRAME_SIZE          127     // largest data part of one frame we offer
#define RFC_CREDITS             7       // credits we give the device at a time
#define REPLY_MAX               200     // longest AT answer (the +CIND list is about 130 characters)

// features of our audio gateway (+BRSF): HF indicators only, no calls, no codecs
#define AG_FEATURES             "1024"

// Some headphones only report the battery to a "real" phone: one with an audio gateway record in its own SDP
// server. Off for now, the console then refuses connections the device may try to that record.
#define HFP_AG_RECORD           0

// SDP server of the console (found next to SDP_SetLocalDiRecord in the IOS-PAD binary)
extern uint32_t SDP_CreateRecord(void);
extern uint8_t SDP_DeleteRecord(uint32_t handle);
extern uint8_t SDP_AddAttribute(uint32_t handle, uint16_t attr_id, uint8_t type, uint32_t len, const uint8_t* value);
extern uint8_t SDP_AddServiceClassIdList(uint32_t handle, uint16_t num, const uint16_t* uuids);

// security manager (see audio.c)
extern uint8_t BTM_SetSecurityLevel(uint8_t is_originator, const char* p_name, uint8_t service_id,
                                    uint16_t sec_level, uint16_t psm, uint32_t mx_proto_id, uint32_t mx_chan_id);

static uint8_t state;
static uint8_t detail;
static uint8_t battery = 0xFF;
static uint8_t channel;             // RFCOMM server channel of the device
static uint8_t addr[6];
static uint16_t cid;
static uint8_t cfgLocal, cfgRemote;
static uint8_t registered;
static uint8_t dlci;
static uint8_t creditFlow;          // the device accepted credit based flow control
static uint8_t txCredits;           // frames we may still send
static uint8_t rxCredits;           // frames the device may still send us
static uint8_t mscSent;
static uint16_t frameSize = RFC_FRAME_SIZE; // largest data part per frame, as the device answered
static uint8_t txBuf[512];          // AT answers waiting for credits
static uint16_t txLen;
static char line[128];
static uint8_t lineLen;

// the SDP database, the stack fills it during the query
static uint8_t sdpDb[2048] __attribute__ ((aligned(4)));

static uint8_t crcTable[256];

static void setState(uint8_t newState, uint8_t newDetail)
{
    state = newState;
    detail = newDetail;
    audioLogEvent(BLOOPAIR_AUDIO_EV_HFP, newState, newDetail);
}

static void failWith(uint8_t why)
{
    if (cid) {
        uint16_t c = cid;
        cid = 0;
        L2CA_DisconnectReq(c);
    }
    setState(BLOOPAIR_HFP_FAILED, why);
}

// ---------------------------------------------------------------------------------------------
// RFCOMM frames
// ---------------------------------------------------------------------------------------------
static void buildCrcTable(void)
{
    // reversed polynomial x^8 + x^2 + x + 1 of TS 07.10
    for (int i = 0; i < 256; i++) {
        uint8_t c = i;
        for (int b = 0; b < 8; b++) {
            c = (c & 1) ? (c >> 1) ^ 0xE0 : (c >> 1);
        }
        crcTable[i] = c;
    }
}

static uint8_t fcs(const uint8_t* d, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc = crcTable[crc ^ d[i]];
    }
    return 0xFF - crc;
}

// one frame on a channel; we started the session, so our commands carry C/R = 1.
// SABM and DISC need the poll bit, callers pass it with the control field.
static void sendFrame(uint8_t frameDlci, uint8_t control, uint8_t cr, const uint8_t* data, uint16_t len, uint8_t credits)
{
    if (!cid) {
        return;
    }

    uint16_t total = 2 + (len > 127 ? 2 : 1) + (credits ? 1 : 0) + len + 1;
    BT_HDR* p_buf = (BT_HDR*) GKI_getbuf(BT_HDR_SIZE + TX_OFFSET + total);
    if (!p_buf) {
        return;
    }
    p_buf->event = 0;
    p_buf->layer_specific = 0;
    p_buf->offset = TX_OFFSET;
    p_buf->len = total;

    uint8_t* d = ((uint8_t*) p_buf) + BT_HDR_SIZE + TX_OFFSET;
    uint16_t i = 0;
    d[i++] = (frameDlci << 2) | (cr << 1) | 1;
    d[i++] = control | (credits ? RFC_PF : 0);
    if (len > 127) {
        d[i++] = (len & 0x7F) << 1;
        d[i++] = len >> 7;
    } else {
        d[i++] = (len << 1) | 1;
    }
    if (credits) {
        d[i++] = credits;
    }
    memcpy(d + i, data, len);
    i += len;

    // data frames are only protected in address and control, the others include the length
    uint8_t isUih = (control & ~RFC_PF) == RFC_UIH;
    d[i] = fcs(d, isUih ? 2 : 3);

    L2CA_DataWrite(cid, p_buf);
}

static void sendMcc(uint8_t type, uint8_t command, const uint8_t* values, uint8_t len)
{
    uint8_t msg[12];
    msg[0] = (type << 2) | (command << 1) | 1;
    msg[1] = (len << 1) | 1;
    memcpy(msg + 2, values, len);
    sendFrame(0, RFC_UIH, 1, msg, 2 + len, 0);
}

// AT text is a stream: it waits here until the device grants credits, then goes out in frames of the agreed size
static void flushText(void)
{
    while (txLen && (!creditFlow || txCredits)) {
        uint16_t part = txLen > frameSize ? frameSize : txLen;

        // hand out new credits together with the data when the device runs low
        uint8_t give = 0;
        if (creditFlow && rxCredits <= 2) {
            give = RFC_CREDITS - rxCredits;
            rxCredits += give;
        }
        if (creditFlow) {
            txCredits--;
        }
        sendFrame(dlci, RFC_UIH, 1, txBuf, part, give);
        txLen -= part;
        for (uint16_t i = 0; i < txLen; i++) {
            txBuf[i] = txBuf[part + i]; // no memmove in IOS-PAD, copying forward is safe here
        }
    }
}

static void sendText(const char* text)
{
    uint16_t len = strnlen(text, REPLY_MAX);
    if (txLen + len > sizeof(txBuf)) {
        return; // the device stopped listening, nothing to gain from more
    }
    memcpy(txBuf + txLen, text, len);
    txLen += len;
    flushText();
}

static void reply(const char* text)
{
    // every answer is framed by CR LF on both sides
    char buf[REPLY_MAX + 1];
    uint16_t len = strnlen(text, REPLY_MAX - 4);
    buf[0] = '\r';
    buf[1] = '\n';
    memcpy(buf + 2, text, len);
    buf[2 + len] = '\r';
    buf[3 + len] = '\n';
    buf[4 + len] = 0;
    sendText(buf);
}

// ---------------------------------------------------------------------------------------------
// audio gateway: the AT commands of the hands-free unit
// ---------------------------------------------------------------------------------------------
static int startsWith(const char* s, const char* prefix)
{
    return strncmp(s, prefix, strnlen(prefix, 32)) == 0;
}

static int parseInt(const char** p)
{
    int v = 0;
    while (**p == ' ') {
        (*p)++;
    }
    while (**p >= '0' && **p <= '9') {
        v = v * 10 + (**p - '0');
        (*p)++;
    }
    return v;
}

static void skipComma(const char** p)
{
    while (**p && **p != ',') {
        (*p)++;
    }
    if (**p == ',') {
        (*p)++;
    }
}

static void setBattery(int percent)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    battery = percent;
    audioLogEvent(BLOOPAIR_AUDIO_EV_HFP, 0x100 | percent, 0);
}

static void handleAt(const char* cmd)
{
    uint16_t len = strnlen(cmd, sizeof(line));
    audioLogEvent(BLOOPAIR_AUDIO_EV_HFP_AT, len,
        ((uint32_t) (len > 3 ? cmd[3] : 0)) << 24 | ((uint32_t) (len > 4 ? cmd[4] : 0)) << 16 |
        ((uint32_t) (len > 5 ? cmd[5] : 0)) << 8 | (len > 6 ? cmd[6] : 0));

    if (startsWith(cmd, "AT+BRSF=")) {
        reply("+BRSF: " AG_FEATURES);
    } else if (startsWith(cmd, "AT+CIND=?")) {
        reply("+CIND: (\"service\",(0,1)),(\"call\",(0,1)),(\"callsetup\",(0-3)),(\"callheld\",(0-2)),"
              "(\"signal\",(0-5)),(\"roam\",(0,1)),(\"battchg\",(0-5))");
    } else if (startsWith(cmd, "AT+CIND?")) {
        reply("+CIND: 1,0,0,0,5,0,5");
    } else if (startsWith(cmd, "AT+CMER=")) {
        // without three way calling the service level connection is complete here
        reply("OK");
        setState(BLOOPAIR_HFP_READY, 0);
        return;
    } else if (startsWith(cmd, "AT+CHLD=?")) {
        reply("+CHLD: (0,1,2,3)");
    } else if (startsWith(cmd, "AT+BIND=?")) {
        reply("+BIND: (2)");
    } else if (startsWith(cmd, "AT+BIND?")) {
        reply("+BIND: 2,1");
    } else if (startsWith(cmd, "AT+BIEV=")) {
        // HF indicator 2 is the battery level in percent
        const char* p = cmd + 8;
        int id = parseInt(&p);
        skipComma(&p);
        if (id == 2) {
            setBattery(parseInt(&p));
        }
    } else if (startsWith(cmd, "AT+XAPL=")) {
        // Apple accessory: we support battery reporting (feature bit 2)
        reply("+XAPL=iPhone,2");
    } else if (startsWith(cmd, "AT+IPHONEACCEV=")) {
        // count, then key / value pairs; key 1 is the battery level 0 - 9
        const char* p = cmd + 15;
        int count = parseInt(&p);
        for (int i = 0; i < count && *p; i++) {
            skipComma(&p);
            int key = parseInt(&p);
            skipComma(&p);
            int value = parseInt(&p);
            if (key == 1) {
                setBattery((value + 1) * 10);
            }
        }
    } else if (startsWith(cmd, "AT+XEVENT=BATTERY,")) {
        // level, number of levels
        const char* p = cmd + 18;
        int level = parseInt(&p);
        skipComma(&p);
        int levels = parseInt(&p);
        if (levels > 1) {
            setBattery(level * 100 / (levels - 1));
        }
    } else if (startsWith(cmd, "AT+COPS?")) {
        reply("+COPS: 0");
    }

    // everything else (CLIP, CCWA, NREC, VGS, VGM, CMEE, BIA, ...) is simply accepted
    reply("OK");
}

static void handleData(const uint8_t* d, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        char c = d[i];
        if (c == '\r' || c == '\n') {
            if (lineLen) {
                line[lineLen] = 0;
                handleAt(line);
                lineLen = 0;
            }
        } else if (lineLen < sizeof(line) - 1) {
            line[lineLen++] = c;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// RFCOMM session
// ---------------------------------------------------------------------------------------------
static void sendPn(void)
{
    // ask for credit based flow control and give the device its first credits
    uint8_t v[8] = {dlci, 0xF0, 0, 0, RFC_FRAME_SIZE & 0xFF, RFC_FRAME_SIZE >> 8, 0, RFC_CREDITS};
    rxCredits = RFC_CREDITS;
    sendMcc(MCC_PN, 1, v, 8);
}

static void sendMsc(void)
{
    // modem status: ready to communicate, ready to receive, valid data
    uint8_t v[2] = {(uint8_t) ((dlci << 2) | 0x03), 0x8D};
    sendMcc(MCC_MSC, 1, v, 2);
    mscSent = 1;
}

static void handleMcc(const uint8_t* d, uint16_t len)
{
    if (len < 2) {
        return;
    }
    uint8_t type = d[0] >> 2;
    uint8_t command = (d[0] >> 1) & 1;
    uint8_t vlen = d[1] >> 1;
    const uint8_t* v = d + 2;
    if (2 + vlen > len) {
        return;
    }

    if (type == MCC_PN && !command && vlen >= 8) {
        // the answer to our parameters: credit based flow control is on if the device says 0xE0
        creditFlow = (v[1] & 0xF0) == 0xE0;
        txCredits = creditFlow ? (v[7] & 0x07) : 0;
        frameSize = v[4] | (v[5] << 8);
        if (frameSize == 0 || frameSize > RFC_FRAME_SIZE) {
            frameSize = RFC_FRAME_SIZE;
        }
        setState(BLOOPAIR_HFP_DLC, creditFlow);
        sendFrame(dlci, RFC_SABM | RFC_PF, 1, NULL, 0, 0);
        return;
    }

    if (command) {
        // answer every command of the device with the same values (modem status, line settings ...)
        uint8_t msg[16];
        if (vlen > sizeof(msg) - 2) {
            return;
        }
        msg[0] = (type << 2) | 1;
        msg[1] = (vlen << 1) | 1;
        memcpy(msg + 2, v, vlen);
        sendFrame(0, RFC_UIH, 1, msg, 2 + vlen, 0);
    }
}

static void handleFrame(const uint8_t* d, uint16_t len)
{
    if (len < 4) {
        return;
    }
    audioLogEvent(BLOOPAIR_AUDIO_EV_HFP_FRAME, (d[1] << 8) | d[0], len);

    uint8_t frameDlci = d[0] >> 2;
    uint8_t control = d[1] & ~RFC_PF;
    uint8_t pf = d[1] & RFC_PF;
    uint16_t hdr = 3;
    uint16_t plen = d[2] >> 1;
    if (!(d[2] & 1)) {
        plen |= d[3] << 7;
        hdr = 4;
    }
    // with credit flow a data frame with the P/F bit carries a credit byte that the length does not count
    uint8_t credits = 0;
    if (creditFlow && pf && control == RFC_UIH && frameDlci != 0) {
        if (hdr + 1 > len) {
            return;
        }
        credits = d[hdr];
        hdr++;
    }
    if (hdr + plen + 1 > len) {
        return;
    }
    const uint8_t* payload = d + hdr;

    if (control == RFC_UA) {
        if (frameDlci == 0 && state == BLOOPAIR_HFP_MUX) {
            sendPn();
        } else if (frameDlci == dlci && state == BLOOPAIR_HFP_DLC) {
            setState(BLOOPAIR_HFP_SLC, 0);
            sendMsc();
        }
        return;
    }

    if (control == RFC_DM) {
        failWith(0x10 | (frameDlci ? 1 : 0)); // the device refused the channel
        return;
    }

    if (control == RFC_DISC) {
        sendFrame(frameDlci, RFC_UA | RFC_PF, 0, NULL, 0, 0);
        if (frameDlci == 0 || frameDlci == dlci) {
            failWith(0x20);
        }
        return;
    }

    if (control != RFC_UIH) {
        return;
    }

    if (frameDlci == 0) {
        handleMcc(payload, plen);
        return;
    }

    if (frameDlci != dlci) {
        return;
    }

    if (credits) {
        txCredits += credits;
        flushText();
    }
    if (!plen) {
        return;
    }
    if (creditFlow && rxCredits) {
        rxCredits--;
    }
    handleData(payload, plen);

    // the device must not run out of credits while it waits for answers
    if (creditFlow && rxCredits <= 2) {
        uint8_t give = RFC_CREDITS - rxCredits;
        rxCredits += give;
        sendFrame(dlci, RFC_UIH, 1, NULL, 0, give);
    }
}

// ---------------------------------------------------------------------------------------------
// L2CAP callbacks
// ---------------------------------------------------------------------------------------------
static void startConfig(uint16_t c)
{
    L2capCfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    CFG_MTU_PRESENT(&cfg) = 1;
    CFG_MTU(&cfg) = HFP_LOCAL_MTU;
    L2CA_ConfigReq(c, &cfg);
}

static void checkOpen(void)
{
    if (!cfgLocal || !cfgRemote || state != BLOOPAIR_HFP_L2CAP) {
        return;
    }

    // start the multiplexer on channel 0
    setState(BLOOPAIR_HFP_MUX, 0);
    sendFrame(0, RFC_SABM | RFC_PF, 1, NULL, 0, 0);
}

static void cbConnectInd(uint8_t* bd_addr, uint32_t lcid, uint32_t psm, uint32_t id)
{
    // nobody on the console offers RFCOMM services: refuse, instead of leaving the device waiting
    L2CA_ConnectRsp(bd_addr, id & 0xFF, lcid & 0xFFFF, L2CAP_CONN_NO_RESOURCES, 0);
}

static void cbConnectCfm(uint32_t lcid, uint32_t result)
{
    if ((lcid & 0xFFFF) != cid) {
        return;
    }
    if (result & 0xFFFF) {
        cid = 0;
        setState(BLOOPAIR_HFP_FAILED, 0x30);
        return;
    }
    startConfig(cid);
}

static void cbConfigInd(uint32_t lcid, L2capCfg* cfg)
{
    if ((lcid & 0xFFFF) != cid) {
        return;
    }
    CFG_RESULT(cfg) = 0;
    CFG_MTU_PRESENT(cfg) = 0;
    CFG_FLUSH_PRESENT(cfg) = 0;
    L2CA_ConfigRsp(cid, cfg);
    cfgRemote = 1;
    checkOpen();
}

static void cbConfigCfm(uint32_t lcid, L2capCfg* cfg)
{
    if ((lcid & 0xFFFF) != cid) {
        return;
    }
    if (CFG_RESULT(cfg) != 0) {
        failWith(0x31);
        return;
    }
    cfgLocal = 1;
    checkOpen();
}

static void cbDisconnectInd(uint32_t lcid, uint32_t ackNeeded)
{
    if (ackNeeded & 0xFF) {
        L2CA_DisconnectRsp(lcid & 0xFFFF);
    }
    if ((lcid & 0xFFFF) == cid) {
        cid = 0;
        if (state != BLOOPAIR_HFP_FAILED && state != BLOOPAIR_HFP_OFF) {
            setState(BLOOPAIR_HFP_FAILED, 0x32);
        }
    }
}

static void cbDataInd(uint32_t lcid, BT_HDR* p_buf)
{
    if ((lcid & 0xFFFF) == cid) {
        handleFrame(((uint8_t*) p_buf) + BT_HDR_SIZE + p_buf->offset, p_buf->len);
    }
    GKI_freebuf(p_buf);
}

static const L2capApplInfo applInfo = {
    cbConnectInd,
    cbConnectCfm,
    NULL,
    cbConfigInd,
    cbConfigCfm,
    cbDisconnectInd,
    NULL,
    NULL,
    cbDataInd,
    NULL,
    NULL,
};

// ---------------------------------------------------------------------------------------------
// SDP: on which RFCOMM channel does the device offer hands-free?
// ---------------------------------------------------------------------------------------------
static uint8_t findChannel(void)
{
    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_HANDSFREE;
    tSDP_DISC_REC* rec = SDP_FindServiceUUIDInDb((tSDP_DISCOVERY_DB*) sdpDb, &uuid, NULL);
    if (!rec) {
        return 0;
    }

    // protocol descriptor list: ((L2CAP), (RFCOMM, channel))
    tSDP_DISC_ATTR* list = SDP_FindAttributeInRec(rec, ATTR_PROTOCOL_LIST);
    if (!list || SDP_ATTR_TYPE(list) != SDP_TYPE_SEQ) {
        return 0;
    }
    for (tSDP_DISC_ATTR* proto = list->attr_value.v.p_sub_attr; proto; proto = proto->p_next_attr) {
        if (SDP_ATTR_TYPE(proto) != SDP_TYPE_SEQ) {
            continue;
        }
        tSDP_DISC_ATTR* id = proto->attr_value.v.p_sub_attr;
        if (!id || SDP_ATTR_TYPE(id) != SDP_TYPE_UUID || id->attr_value.v.u16 != UUID_RFCOMM) {
            continue;
        }
        tSDP_DISC_ATTR* ch = id->p_next_attr;
        if (ch && SDP_ATTR_TYPE(ch) == SDP_TYPE_UINT) {
            return ch->attr_value.v.u8;
        }
    }
    return 0;
}

static void sdpDone(uint16_t result)
{
    if (state != BLOOPAIR_HFP_SDP) {
        return;
    }
    if (result != 0) {
        setState(BLOOPAIR_HFP_FAILED, 0x40 | (result & 0x0F));
        return;
    }

    channel = findChannel();
    if (!channel || channel > 30) {
        // detail: how many records came back, to tell "no service" from "not understood"
        uint8_t records = 0;
        for (tSDP_DISC_REC* r = ((tSDP_DISCOVERY_DB*) sdpDb)->p_first_rec; r && records < 0xFF; r = r->p_next_rec) {
            records++;
        }
        setState(BLOOPAIR_HFP_UNSUPPORTED, records);
        return;
    }

    // RFCOMM of the console's stack is unused: our own client takes over its L2CAP psm
    BTM_SetSecurityLevel(1, "HFP", HFP_SEC_SERVICE_ID, BTM_SEC_NONE, RFCOMM_PSM, 0, 0);
    if (!registered) {
        if ((L2CA_Register(RFCOMM_PSM, &applInfo) & 0xFFFF) == 0) {
            setState(BLOOPAIR_HFP_FAILED, 0x33);
            return;
        }
        registered = 1;
    }

    // we start the session, the device is the server: its channel number times two
    dlci = channel << 1;
    cfgLocal = cfgRemote = 0;
    creditFlow = txCredits = rxCredits = mscSent = 0;
    frameSize = RFC_FRAME_SIZE;
    txLen = 0;
    lineLen = 0;
    setState(BLOOPAIR_HFP_L2CAP, channel);
    cid = L2CA_ConnectReq(RFCOMM_PSM, addr) & 0xFFFF;
    if (!cid) {
        setState(BLOOPAIR_HFP_FAILED, 0x34);
    }
}

#if HFP_AG_RECORD
static uint32_t agRecord;

// our audio gateway as the device sees it in the console's SDP server: hands-free 1.7 on RFCOMM channel 1
static void createAgRecord(void)
{
    if (agRecord) {
        return;
    }
    agRecord = SDP_CreateRecord();
    if (!agRecord) {
        return;
    }

    static const uint16_t classes[2] = {0x111F, 0x1203};
    static const uint8_t protocols[] = {0x35, 0x03, 0x19, 0x01, 0x00, 0x35, 0x05, 0x19, 0x00, 0x03, 0x08, 0x01};
    static const uint8_t browse[] = {0x19, 0x10, 0x02};
    static const uint8_t profile[] = {0x35, 0x06, 0x19, 0x11, 0x1E, 0x09, 0x01, 0x07};
    static const uint8_t features[] = {0x00, 0x00};
    static const uint8_t network[] = {0x01};
    SDP_AddServiceClassIdList(agRecord, 2, classes);
    SDP_AddAttribute(agRecord, ATTR_PROTOCOL_LIST, SDP_TYPE_SEQ, sizeof(protocols), protocols);
    SDP_AddAttribute(agRecord, 0x0005, SDP_TYPE_SEQ, sizeof(browse), browse);
    SDP_AddAttribute(agRecord, 0x0009, SDP_TYPE_SEQ, sizeof(profile), profile);
    SDP_AddAttribute(agRecord, 0x0311, SDP_TYPE_UINT, sizeof(features), features);
    SDP_AddAttribute(agRecord, 0x0301, SDP_TYPE_UINT, sizeof(network), network);
}
#endif

void hfpStart(const uint8_t* bd_addr)
{
    if (state != BLOOPAIR_HFP_OFF && state != BLOOPAIR_HFP_FAILED && state != BLOOPAIR_HFP_UNSUPPORTED) {
        return;
    }
    if (!crcTable[1]) {
        buildCrcTable();
    }
#if HFP_AG_RECORD
    createAgRecord();
#endif

    memcpy(addr, bd_addr, 6);
    battery = 0xFF;
    channel = 0;
    cid = 0;

    tBT_UUID uuid;
    uuid.len = LEN_UUID_16;
    uuid.uu.uuid16 = UUID_HANDSFREE;
    uint16_t attrs[2] = {ATTR_SERVICE_CLASS_LIST, ATTR_PROTOCOL_LIST};
    setState(BLOOPAIR_HFP_SDP, 0);
    if (!SDP_InitDiscoveryDb((tSDP_DISCOVERY_DB*) sdpDb, sizeof(sdpDb), 1, &uuid, 2, attrs) ||
        !SDP_ServiceSearchRequest(addr, (tSDP_DISCOVERY_DB*) sdpDb, sdpDone)) {
        setState(BLOOPAIR_HFP_FAILED, 0x41);
    }
}

void hfpStop(void)
{
    if (cid) {
        uint16_t c = cid;
        cid = 0;
        L2CA_DisconnectReq(c);
    }
    state = BLOOPAIR_HFP_OFF;
    battery = 0xFF;
}

void hfpGetStatus(BloopairAudioStatus* out)
{
    out->hfpState = state;
    out->hfpChannel = channel;
    out->battery = battery;
    out->hfpDetail = detail;
}
