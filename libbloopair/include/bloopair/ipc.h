/*
 *   Copyright (C) 2021-2023 GaryOderNichts
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

#include <stdint.h>

#define BLOOPAIR_LIB 0x10

#define BLOOPAIR_FUNC_GET_VERSION                   0
#define BLOOPAIR_FUNC_READ_CONSOLE_BDADDR           1
#define BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING        2
#define BLOOPAIR_FUNC_GET_COMMIT_HASH               3
#define BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION    4
#define BLOOPAIR_FUNC_READ_RAW_REPORT               5
#define BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG       6
#define BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING      7
#define BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION    8
#define BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG         9
#define BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING        10
#define BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION      11
#define BLOOPAIR_FUNC_SET_SCAN_MODE                 12
#define BLOOPAIR_FUNC_GET_SCAN_RESULTS              13
#define BLOOPAIR_FUNC_AUDIO_TEST_ARM                14
#define BLOOPAIR_FUNC_AUDIO_TEST_GET_STATUS         15
#define BLOOPAIR_FUNC_AUDIO_WRITE                   16
#define BLOOPAIR_FUNC_AUDIO_CONNECT                 17
#define BLOOPAIR_FUNC_AUDIO_DISCONNECT              18

#define BLOOPAIR_VERSION_MAJOR(v) (((v) >> 16) & 0xff)
#define BLOOPAIR_VERSION_MINOR(v) (((v) >> 8) & 0xff)
#define BLOOPAIR_VERSION_PATCH(v) ((v) & 0xff)
#define BLOOPAIR_VERSION(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

typedef struct __attribute__ ((__packed__)) {
    uint8_t data[4096];
    uint8_t lib;
    uint8_t func;
    uint16_t unk;
    uint32_t unk1;
} BtrmRequest;

typedef struct __attribute__ ((__packed__)) {
    uint8_t data[4096];
    uint8_t unk[12];
} BtrmResponse;

// structure associated with BLOOPAIR_FUNC_ADD_CONTROLLER_PAIRING
typedef struct {
    uint8_t bd_address[6];
    uint8_t link_key[16];
    uint8_t name[64];
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairPairingData;

// structure associated with BLOOPAIR_FUNC_GET_CONTROLLER_INFORMATION
typedef struct {
    uint8_t controllerType;
    uint16_t vendor_id;
    uint16_t product_id;
} BloopairControllerInformationData;

// structure associated with
// - BLOOPAIR_FUNC_READ_RAW_REPORT
// - BLOOPAIR_FUNC_GET_CONTROLLER_CONFIG
// - BLOOPAIR_FUNC_GET_CONTROLLER_MAPPING
// - BLOOPAIR_FUNC_GET_CUSTOM_CONFIGURATION
typedef struct {
    uint8_t handle;
    uint8_t controllerType;
} BloopairControllerRequestData;

// structure associated with
// - BLOOPAIR_FUNC_APPLY_CONTROLLER_CONFIG
// - BLOOPAIR_FUNC_APPLY_CONTROLLER_MAPPING
// - BLOOPAIR_FUNC_APPLY_CUSTOM_CONFIGURATION
typedef struct {
    uint8_t controllerType;
    uint8_t bd_address[6];
    uint32_t dataSize;
    uint8_t data[];
} BloopairApplyControllerConfigurationData;

// Bluetooth device scan (BTAudio)
#define BLOOPAIR_SCAN_MAX_RESULTS   16
#define BLOOPAIR_SCAN_NAME_SIZE     40

#define BLOOPAIR_SCAN_FLAG_NAME_KNOWN   (1 << 0)

// structure associated with BLOOPAIR_FUNC_SET_SCAN_MODE
typedef struct __attribute__ ((__packed__)) {
    uint8_t enabled;
    uint8_t clear;
} BloopairScanModeData;

typedef struct __attribute__ ((__packed__)) {
    uint8_t bd_address[6];
    uint8_t dev_class[3];
    int8_t rssi;
    uint8_t flags;
    uint8_t reserved;
    char name[BLOOPAIR_SCAN_NAME_SIZE];
} BloopairScanEntry;

// structure associated with BLOOPAIR_FUNC_GET_SCAN_RESULTS
typedef struct __attribute__ ((__packed__)) {
    uint8_t enabled;
    uint8_t count;
    BloopairScanEntry entries[BLOOPAIR_SCAN_MAX_RESULTS];
} BloopairScanResults;

// Bluetooth audio connection (BTAudio)
#define BLOOPAIR_AUDIO_LOG_SIZE     200
#define BLOOPAIR_AUDIO_RESP_SIZE    48

// states of the audio connection test
#define BLOOPAIR_AUDIO_STATE_IDLE           0
#define BLOOPAIR_AUDIO_STATE_ARMED          1   // waiting for the next search to finish
#define BLOOPAIR_AUDIO_STATE_CONNECTING     2
#define BLOOPAIR_AUDIO_STATE_CONFIGURING    3
#define BLOOPAIR_AUDIO_STATE_OPEN           4
#define BLOOPAIR_AUDIO_STATE_DONE           5   // got an answer to the AVDTP discover command
#define BLOOPAIR_AUDIO_STATE_FAILED         6
#define BLOOPAIR_AUDIO_STATE_CLOSED         7
#define BLOOPAIR_AUDIO_STATE_PAIRING        8
#define BLOOPAIR_AUDIO_STATE_STREAMING      9   // AVDTP stream started, SBC frames can be written

// events in the audio test log
#define BLOOPAIR_AUDIO_EV_NO_TARGET         1   // no audio device in the scan results
#define BLOOPAIR_AUDIO_EV_TARGET            2   // value: first two address bytes, data: last four
#define BLOOPAIR_AUDIO_EV_SECURITY          3   // data: result of BTM_SetSecurityLevel
#define BLOOPAIR_AUDIO_EV_L2CAP_REGISTER    4   // value: registered psm (0 = failed)
#define BLOOPAIR_AUDIO_EV_CONNECT_REQ       5   // value: channel id (0 = failed)
#define BLOOPAIR_AUDIO_EV_CONNECT_CFM       6   // value: channel id, data: result
#define BLOOPAIR_AUDIO_EV_CONFIG_REQ        7   // value: channel id, data: result
#define BLOOPAIR_AUDIO_EV_CONFIG_IND        8   // value: channel id, data: remote mtu
#define BLOOPAIR_AUDIO_EV_CONFIG_CFM        9   // value: channel id, data: result
#define BLOOPAIR_AUDIO_EV_CHANNEL_OPEN      10  // value: channel id
#define BLOOPAIR_AUDIO_EV_DISCOVER_SENT     11  // data: result of L2CA_DataWrite
#define BLOOPAIR_AUDIO_EV_DATA_IND          12  // value: channel id, data: length
#define BLOOPAIR_AUDIO_EV_DISCONNECT_REQ    13  // value: channel id
#define BLOOPAIR_AUDIO_EV_DISCONNECT_IND    14  // value: channel id, data: ack needed
#define BLOOPAIR_AUDIO_EV_DISCONNECT_CFM    15  // value: channel id, data: result
#define BLOOPAIR_AUDIO_EV_CONNECT_IND       16  // unexpected incoming connection, value: psm
#define BLOOPAIR_AUDIO_EV_NO_BUFFER         17
#define BLOOPAIR_AUDIO_EV_BOND_REQ          18  // data: result of BTM_SecBond (0 = already paired, 1 = started)
#define BLOOPAIR_AUDIO_EV_SEC_EVENT         19  // value: security event id, data: extra info
#define BLOOPAIR_AUDIO_EV_AUTH_CMPL         20  // value: success, data: fail reason | key present << 8
#define BLOOPAIR_AUDIO_EV_PAIRABLE          21  // pairing was enabled
#define BLOOPAIR_AUDIO_EV_SP_EVENT          25  // value: pairing state << 8 | simple pairing event id, data: details
#define BLOOPAIR_AUDIO_EV_LOCAL_IO          26  // value: local io capabilities of the console
#define BLOOPAIR_AUDIO_EV_SEARCH_EVENT      23  // value: search event id
#define BLOOPAIR_AUDIO_EV_RETRY             24  // value: attempt number, data: fail reason
#define BLOOPAIR_AUDIO_EV_BTM_STATE         22  // value: pairing disabled << 8 | pairing state, data: security event id

// HCI traffic during pairing: the low byte of "event" is the kind, the high byte the parameter length.
// HCI_CMD: value = opcode, HCI_EVT: value = event code; data = first four parameter bytes (big endian).
// HCI_MORE follows with the next four parameter bytes (value = index of the entry, 1 or 2).
#define BLOOPAIR_AUDIO_EV_HCI_CMD           30
#define BLOOPAIR_AUDIO_EV_HCI_EVT           31
#define BLOOPAIR_AUDIO_EV_HCI_MORE          32
#define BLOOPAIR_AUDIO_EV_ACL_TX            34  // sent ACL packet: value = handle, data = first bytes of the L2CAP frame (length, cid, payload)
#define BLOOPAIR_AUDIO_EV_ACL_RX            35  // received ACL packet, same layout
#define BLOOPAIR_AUDIO_EV_AVDTP_CMD         36  // sent AVDTP command: value = signal id, data = first payload bytes
#define BLOOPAIR_AUDIO_EV_AVDTP_RSP         37  // received AVDTP message: value = signal id | message type << 8, data = first bytes
#define BLOOPAIR_AUDIO_EV_AVDTP_INFO        38  // value: 1 = device endpoint (data: seid | in use << 8 | type << 16 | media << 24),
                                                //        2 = chosen configuration (data: freq | mode << 8 | bitpool << 16 | blocks << 24),
                                                //        3 = SBC capabilities (data: the four capability bytes),
                                                //        4 = frame length / media mtu, 5 = stream started, 6 = stream stopped
#define BLOOPAIR_AUDIO_EV_HFP               41  // value: hands-free state, data: detail
#define BLOOPAIR_AUDIO_EV_HFP_AT            42  // AT command of the device: value = length, data = first four characters
#define BLOOPAIR_AUDIO_EV_HFP_FRAME         43  // RFCOMM frame of the device: value = control << 8 | address, data = length
#define BLOOPAIR_AUDIO_EV_KEY_ADD           40  // value: 0 = stored key handed to the stack, 1 = stack has taken it, 2 = old key of the stack dropped before pairing
#define BLOOPAIR_AUDIO_EV_HCI_DISC_CALL     33  // the stack asks for a disconnect: value = reason, data = address of the caller

// structure associated with BLOOPAIR_FUNC_AUDIO_TEST_ARM
typedef struct __attribute__ ((__packed__)) {
    uint8_t enabled;
} BloopairAudioTestData;

typedef struct __attribute__ ((__packed__)) {
    uint16_t event;
    uint16_t value;
    uint32_t data;
} BloopairAudioLogEntry;

// structure associated with BLOOPAIR_FUNC_AUDIO_TEST_GET_STATUS
typedef struct __attribute__ ((__packed__)) {
    uint8_t enabled;
    uint8_t state;
    uint16_t cid;
    uint8_t bd_address[6];
    uint16_t responseLength;                        // length of the AVDTP answer
    uint8_t response[BLOOPAIR_AUDIO_RESP_SIZE];     // first bytes of the AVDTP answer
    uint8_t logCount;
    uint8_t logGeneration;      // counts up when the log starts again during one connection
    BloopairAudioLogEntry log[BLOOPAIR_AUDIO_LOG_SIZE];

    // SBC settings of the started stream (values as used by libsbc), valid in state STREAMING
    uint8_t sbcFrequency;       // 0 = 16 kHz, 1 = 32 kHz, 2 = 44.1 kHz, 3 = 48 kHz
    uint8_t sbcMode;            // 0 = mono, 1 = dual channel, 2 = stereo, 3 = joint stereo
    uint8_t sbcBlocks;          // 4, 8, 12 or 16
    uint8_t sbcSubbands;        // 4 or 8
    uint8_t sbcAllocation;      // 0 = loudness, 1 = snr
    uint8_t sbcBitpool;
    uint16_t sbcFrameLength;    // bytes of one SBC frame
    uint16_t mediaMtu;
    uint16_t ringFrames;        // frames waiting to be sent
    uint16_t ringCapacity;
    uint32_t framesSent;
    uint32_t underruns;

    // link key of the last successful pairing, to be stored by the PPC side
    uint8_t keyValid;
    uint8_t keyType;
    uint8_t linkKey[16];

    // radio statistics of the stream, for finding dropouts
    uint32_t congestions;       // times the link reported it is full
    uint32_t congestedMs;       // time spent congested in total
    uint32_t maxCongestedMs;    // longest single congestion
    uint32_t droppedFrames;     // old frames skipped after a congestion to keep the delay low

    // battery level of the headphones, read over the hands-free profile (only if asked for when connecting)
    uint8_t hfpState;           // BLOOPAIR_HFP_*
    uint8_t hfpChannel;         // RFCOMM server channel of the device
    uint8_t battery;            // percent, 0xFF = not known
    uint8_t hfpDetail;          // why it stopped (step or result code)
} BloopairAudioStatus;

#define BLOOPAIR_HFP_OFF            0
#define BLOOPAIR_HFP_SDP            1   // asking the device for its hands-free channel
#define BLOOPAIR_HFP_L2CAP          2   // opening the L2CAP channel for RFCOMM
#define BLOOPAIR_HFP_MUX            3   // starting the RFCOMM multiplexer
#define BLOOPAIR_HFP_DLC            4   // opening the hands-free channel
#define BLOOPAIR_HFP_SLC            5   // channel open, exchanging AT commands
#define BLOOPAIR_HFP_READY          6   // service level connection up, battery reports may come
#define BLOOPAIR_HFP_FAILED         7
#define BLOOPAIR_HFP_UNSUPPORTED    8   // the device has no hands-free service

// structure associated with BLOOPAIR_FUNC_AUDIO_CONNECT: connect to one audio device and start streaming.
// With a link key the device is expected to be paired already, without one the pairing is done on the way.
typedef struct __attribute__ ((__packed__)) {
    uint8_t bd_address[6];
    uint8_t hasKey;
    uint8_t keyType;
    uint8_t linkKey[16];
    uint8_t maxBitpool;     // limit for the SBC bitpool (less data, more robust radio), 0 = best the device offers
    uint8_t battery;        // 1 = also open a hands-free connection to read the battery level
} BloopairAudioConnectData;

// structure associated with BLOOPAIR_FUNC_AUDIO_WRITE: whole SBC frames, the result is the number of frames accepted
typedef struct __attribute__ ((__packed__)) {
    uint16_t length;
    uint8_t data[];
} BloopairAudioWriteData;
#define BLOOPAIR_AUDIO_WRITE_MAX    4000
