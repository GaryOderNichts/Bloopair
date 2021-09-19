/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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

#include "main.h"
#include "crypto.h"
#include "controllers.h"
#include "utils.h"

typedef struct __attribute__ ((__packed__)) {
    uint8_t dev_handle;
    uint8_t mode;
    uint8_t sub_class;
    uint8_t app_id;
    uint16_t length;
    uint8_t data[58];
} padscore_input_data_t;

typedef struct __attribute__ ((__packed__)) {
    uint8_t __unk1[10];
    uint16_t length;
    uint16_t __unk2;
    uint8_t dev_handle;
    uint8_t __unk3;
    uint8_t data[120];
} padscore_output_data_t;

padscore_output_data_t* output_buf = (padscore_output_data_t*) 0x11fd5184;

const uint8_t wiiu_pro_controller_id[] = { 0x00, 0x00, 0xa4, 0x20, 0x01, 0x20 };

const uint8_t wiiu_pro_controller_cert[] = { 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0xa4, 0x20, 0x00, 0x05 };

#define HH_SEND_DATA_OFFSET 0x29

void sendInputData(uint8_t dev_handle, const uint8_t* data, uint16_t len)
{
    if (!len || len > 58) {
        return;
    }

    if (!(*isSmdReady)) {
        return;
    }

    padscore_input_data_t msg;
    msg.dev_handle = dev_handle;
    msg.length = len;
    msg.mode = 0;
    msg.sub_class = 4;
    msg.app_id = 3;

    _memcpy(msg.data, data, len);

    smdIopSendMessage(*smdIopIndex, &msg, sizeof(padscore_input_data_t));
}

void sendOutputData(uint8_t dev_handle, const uint8_t* data, uint16_t len)
{
    BT_HDR* p_buf = GKI_getpoolbuf(3);
    if (!p_buf) {
        return;
    }

    p_buf->len = len;
    p_buf->offset = HH_SEND_DATA_OFFSET;
    _memcpy(((uint8_t*) p_buf) + sizeof(BT_HDR) + HH_SEND_DATA_OFFSET, data, p_buf->len);
    BTA_HhSendData(dev_handle, NULL, p_buf);
}

static void sendAcknowledgeReport(uint8_t dev_handle, uint8_t report, uint8_t result)
{
    uint8_t data[5];
    data[0] = 0x22;
    data[1] = data[2] = 0;
    data[3] = report;
    data[4] = result;
    sendInputData(dev_handle, data, sizeof(data));
}

static void sendReadResponse(uint8_t dev_handle, uint8_t result, uint32_t address, const uint8_t* _data, uint8_t size)
{
    if (size > 16) {
        return;
    }

    if (!_data) {
        size = 1;
    }

    uint8_t data[22];
    _memset(data, 0, sizeof(data));

    data[0] = 0x21;
    data[1] = data[2] = 0;
    data[3] = ((size - 1) << 4) | result;
    data[4] = (address >> 8) & 0xff;
    data[5] = address & 0xff;

    if (_data) {
        _memcpy(&data[6], _data, size);
    }

    sendInputData(dev_handle, data, sizeof(data));
}

static void writeMemory(uint8_t dev_handle, uint32_t address, uint8_t* data, uint8_t len)
{
    DEBUG("writing to 0x%08X size %u\n", address, len);
    Controller_t* controller = &controllers[dev_handle];

    switch (address) {
    case 0x04a40040: // key part 1
        if (len == 6) {
            _memcpy(controller->key, data, 6);
            sendAcknowledgeReport(dev_handle, 0x16, 0);
            return;
        }
        break;
    case 0x04a40046: // key part 2
        if (len == 6) {
            _memcpy(controller->key + 6, data, 6);
            sendAcknowledgeReport(dev_handle, 0x16, 0);
            return;
        }
        break;
    case 0x04a4004c: // key part 3
        if (len == 4) {
            _memcpy(controller->key + 12, data, 4);
            cryptoInit(&controller->crypto, controller->key);
            sendAcknowledgeReport(dev_handle, 0x16, 0);
            return;
        }
        break;
    case 0x04a20001:
        sendAcknowledgeReport(dev_handle, 0x16, len == 1 ? 0 : 7);
        return;
    case 0x04a400f0: // init extension
    case 0x04a400fb: // init extension 2
    case 0x04b00000:
    case 0x04b0001a:
    case 0x04b00033:
    case 0x04b00030:
    case 0x04a20009:
        sendAcknowledgeReport(dev_handle, 0x16, 0);
        return;
    case 0x04a20008:
        sendAcknowledgeReport(dev_handle, 0x16, 7);
        return;
    }

    DEBUG("invalid write to 0x%x\n", address);
    sendAcknowledgeReport(dev_handle, 0x16, 3);
}

static void readMemory(uint8_t dev_handle, uint32_t address, uint16_t len)
{
    DEBUG("reading from 0x%08X size %u\n", address, len);
    Controller_t* controller = &controllers[dev_handle];

    switch (address) {
    case 0x04a400fa: // extension
        if (len == 6) {
            sendReadResponse(dev_handle, 0, address, wiiu_pro_controller_id, sizeof(wiiu_pro_controller_id));
            return;
        }
        break;
    case 0x04a600f0: // cert
        if (len == 16) {
            sendReadResponse(dev_handle, 0, address, wiiu_pro_controller_cert, sizeof(wiiu_pro_controller_cert));
            return;
        }
        break;
    case 0x04a40020: // calibration
        if (len == 32) {
            uint8_t tmp[16];
            
            _memset(tmp, 0xff, sizeof(tmp));
            encrypt(&controller->crypto, tmp, tmp, address, 16);
            sendReadResponse(dev_handle, 0, address, tmp, sizeof(tmp));

            _memset(tmp, 0xff, sizeof(tmp));
            encrypt(&controller->crypto, tmp, tmp, address + 16, 16);
            sendReadResponse(dev_handle, 0, address + 16, tmp, sizeof(tmp));
            return;
        }
        break;
    }

    DEBUG("invalid read from 0x%08X\n", address);
    sendReadResponse(dev_handle, 8, address, NULL, 0);
}

// replacement for processing smd messages from padscore
void processSmdMessages(void)
{
    if (!(*isSmdReady)) {
        return;
    }

    while(smdIopReceive(*smdIopIndex, output_buf) != -0xc0005) {
        DEBUG("output request for handle %u size %u, cmd: 0x%X\n", output_buf->dev_handle, output_buf->length, output_buf->data[0]);

        Controller_t* controller = &controllers[output_buf->dev_handle];

        if (controller->isOfficialController) {
#ifdef TESTING
            if (output_buf->data[0] == 0x16) {
                uint32_t address;
                _memcpy(&address, &output_buf->data[1], sizeof(address));

                uint8_t len = output_buf->data[5];
                uint8_t* data = &output_buf->data[6];

                switch (address) {
                case 0x04a40040: // key part 1
                    if (len == 6) {
                        _memcpy(controller->key, data, 6);
                    }
                    break;
                case 0x04a40046: // key part 2
                    if (len == 6) {
                        _memcpy(controller->key + 6, data, 6);
                    }
                    break;
                case 0x04a4004c: // key part 3
                    if (len == 4) {
                        _memcpy(controller->key + 12, data, 4);
                        cryptoInit(&controller->crypto, controller->key);
                    }
                    break;
                }
            }
#endif
            sendOutputData(output_buf->dev_handle, output_buf->data, output_buf->length);
        }
        else {
            if (controller->rumble) {
                controller->rumble(controller, output_buf->data[1] & 0x01);
            }

            // clear the rumble bit
            output_buf->data[1] &= ~0x01;

            switch (output_buf->data[0]) {
            case 0x10: { // rumble
                sendAcknowledgeReport(output_buf->dev_handle, 0x10, 0);
                break;
            }
            case 0x11: { // leds
                if (controller->setPlayerLed) {
                    controller->setPlayerLed(controller, output_buf->data[1] >> 4);
                }
                sendAcknowledgeReport(output_buf->dev_handle, 0x11, 0);
                break;
            }
            case 0x12: { // set report format
                DEBUG("data reporting set to %x\n", output_buf->data[2]);
                sendAcknowledgeReport(output_buf->dev_handle, 0x12, 0);
                break;
            }
            case 0x13: { // ir enable
                controller->irEnabled = 1;
                sendAcknowledgeReport(output_buf->dev_handle, 0x13, 0);
                break;
            }
            case 0x14: { // speaker enable
                sendAcknowledgeReport(output_buf->dev_handle, 0x14, 7);
                break;
            }
            case 0x15: { // request status report
                uint8_t data[7];
                data[0] = 0x20;
                data[1] = data[2] = 0;
                data[3] = 0x10 | 0x2;
                if (controller->irEnabled) {
                    data[3] |= 0x8;
                }
                data[4] = data[5] = 0;
                data[6] = 0xff;
                sendInputData(output_buf->dev_handle, data, sizeof(data));
                break;
            }
            case 0x16: { // write memory
                uint32_t address;
                _memcpy(&address, &output_buf->data[1], sizeof(address));
                writeMemory(output_buf->dev_handle, address, &output_buf->data[6], output_buf->data[5]);
                break;
            }
            case 0x17: { // read memory
                uint32_t address;
                _memcpy(&address, &output_buf->data[1], sizeof(address));
                uint16_t len;
                _memcpy(&len, &output_buf->data[5], sizeof(len));
                readMemory(output_buf->dev_handle, address, len);
                break;
            }
            case 0x19: { // speaker mute
                sendAcknowledgeReport(output_buf->dev_handle, 0x19, output_buf->data[1] == 6 ? 7 : 0);
                break;
            }
            case 0x1a: { // ir enable 2
                sendAcknowledgeReport(output_buf->dev_handle, 0x1a, 0);
            }
            default:
                break;
            }
        }
    }
}

// custom data callback
void bta_hh_co_data(uint8_t dev_handle, uint8_t *p_rpt, uint16_t len, uint8_t mode,
                    uint8_t sub_class, uint8_t ctry_code, uint8_t* peer_addr, uint8_t app_id)
{
    if (!len) {
        return;
    }

    Controller_t* controller = &controllers[dev_handle];
    if (controller->isOfficialController) {
        sendInputData(dev_handle, p_rpt, len);
#ifdef TESTING
        if (p_rpt[0] == 0x20) {
            dumpHex(p_rpt, len);
        }
        else if (p_rpt[0] == 0x3d) {
            static int cnt = 0;
            if (cnt++ % 16 == 0) {
                decrypt(&controller->crypto, p_rpt + 1, p_rpt + 1, 0, len - 1);
                dumpHex(p_rpt, len);
            }
        }
#endif
    }
    else {
        if (controller->data) {
            controller->data(controller, p_rpt, len);
        }
    }
}
