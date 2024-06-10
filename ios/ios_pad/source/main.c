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

#include "main.h"
#include "wiimote_crypto.h"
#include "controllers.h"
#include "utils.h"

#define HH_SEND_DATA_OFFSET 0x29

static const uint8_t wiiu_pro_controller_id[] = { 0x00, 0x00, 0xa4, 0x20, 0x01, 0x20 };

// Motion Plus configuration dumped from a pro controller, unsure what for
static const uint8_t wiiu_pro_controller_mpls_config[] = {
    0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x01, 0x00, 0x00, 0x20,
    0x01, // mplsIsIntegrated
    0x00, 0xa4, 0x20, 0x00,
    0x05, // 0x05 == has mplus
};

void sendInputData(uint8_t dev_handle, const void* data, uint16_t len)
{
    if (!len || len > 58) {
        return;
    }

    if (!isSmdReady) {
        return;
    }

    SMDInputMessage msg;
    msg.dev_handle = dev_handle;
    msg.length = len;
    msg.mode = 0;
    msg.sub_class = 4;
    msg.app_id = 3;
    memcpy(msg.data, data, len);

    smdIopSendMessage(smdIopIndex, &msg, sizeof(msg));
}

void sendOutputData(uint8_t dev_handle, const void* data, uint16_t len)
{
    BT_HDR* p_buf = GKI_getpoolbuf(3);
    if (!p_buf) {
        return;
    }

    p_buf->len = len;
    p_buf->offset = HH_SEND_DATA_OFFSET;
    memcpy(((uint8_t*) p_buf) + sizeof(BT_HDR) + HH_SEND_DATA_OFFSET, data, p_buf->len);
    BTA_HhSendData(dev_handle, NULL, p_buf);
}

void setReport(uint8_t dev_handle, uint8_t type, const void* data, uint16_t len)
{
    BT_HDR* p_buf = GKI_getpoolbuf(3);
    if (!p_buf) {
        return;
    }

    p_buf->len = len;
    p_buf->offset = HH_SEND_DATA_OFFSET;
    memcpy(((uint8_t*) p_buf) + sizeof(BT_HDR) + HH_SEND_DATA_OFFSET, data, p_buf->len);
    bta_hh_snd_write_dev(dev_handle, HID_TRANS_SET_REPORT, type, 0, 0, p_buf);
}

static void sendAcknowledgeReport(uint8_t dev_handle, uint8_t report, uint8_t result)
{
    WMAcknowledgeReport ack;
    ack.report_id = WM_REPORT_ID_ACKNOWLEDGE;
    ack.core_buttons = 0; // no need to set buttons here, we only send the extension buttons
    ack.ack_report = report;
    ack.result = result;
    sendInputData(dev_handle, &ack, sizeof(ack));
}

static void sendReadResponse(uint8_t dev_handle, uint8_t result, uint32_t address, const void* data, uint8_t size)
{
    if (size > 16) {
        return;
    }

    if (!data || !size) {
        size = 1;
    }

    WMMemoryDataReport report;
    memset(&report, 0, sizeof(report));

    report.report_id = WM_REPORT_ID_MEMORY_DATA;
    report.core_buttons = 0; // no need to set buttons here, we only send the extension buttons
    report.size = size - 1;
    report.result = result;
    report.offset = address;

    if (data) {
        memcpy(report.data, data, size);
    }

    sendInputData(dev_handle, &report, sizeof(report));
}

static void writeMemory(uint8_t dev_handle, uint32_t address, uint8_t* data, uint8_t len)
{
    DEBUG_PRINT("writing to 0x%08lX size %d\n", address, len);
    Controller* controller = &controllers[dev_handle];

    switch (address) {
    case 0x04a40040: // key part 1
        if (len == 6) {
            memcpy(controller->extensionKey, data, 6);
            sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, 0);
            return;
        }
        break;
    case 0x04a40046: // key part 2
        if (len == 6) {
            memcpy(controller->extensionKey + 6, data, 6);
            sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, 0);
            return;
        }
        break;
    case 0x04a4004c: // key part 3
        if (len == 4) {
            memcpy(controller->extensionKey + 12, data, 4);
            wiimoteCryptoInit(&controller->cryptoState, controller->extensionKey);
            sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, 0);
            return;
        }
        break;
    case 0x04a20001:
        sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, len == 1 ? 0 : 7);
        return;
    case 0x04a400f0: // init extension
    case 0x04a400fb: // init extension 2
    case 0x04b00000:
    case 0x04b0001a:
    case 0x04b00033:
    case 0x04b00030:
    case 0x04a20009:
        sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, 0);
        return;
    case 0x04a20008:
        sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, 7);
        return;
    }

    DEBUG_PRINT("invalid write to 0x%lx\n", address);
    sendAcknowledgeReport(dev_handle, WM_REPORT_ID_MEMORY_WRITE, 3);
}

static void readMemory(uint8_t dev_handle, uint32_t address, uint16_t len)
{
    DEBUG_PRINT("reading from 0x%08lx size %d\n", address, len);
    Controller* controller = &controllers[dev_handle];

    switch (address) {
    case 0x04a400fa: // extension
        if (len == 6) {
            sendReadResponse(dev_handle, 0, address, wiiu_pro_controller_id, sizeof(wiiu_pro_controller_id));
            return;
        }
        break;
    case 0x04a600f0: // mpls config
        if (len == 16) {
            sendReadResponse(dev_handle, 0, address, wiiu_pro_controller_mpls_config, sizeof(wiiu_pro_controller_mpls_config));
            return;
        }
        break;
    case 0x04a40020: // config / calibration
        if (len == 32) {
            uint8_t tmp[16];
            
            // What is this calibration data used for exactly?
            // Let's just send 0xff for now
            memset(tmp, 0xff, sizeof(tmp));
            wiimoteEncrypt(&controller->cryptoState, tmp, tmp, address, 16);
            sendReadResponse(dev_handle, 0, address, tmp, sizeof(tmp));

            memset(tmp, 0xff, sizeof(tmp));
            wiimoteEncrypt(&controller->cryptoState, tmp, tmp, address + 16, 16);
            sendReadResponse(dev_handle, 0, address + 16, tmp, sizeof(tmp));
            return;
        }
        break;
    }

    DEBUG_PRINT("invalid read from 0x%08lx\n", address);
    sendReadResponse(dev_handle, 8, address, NULL, 0);
}

// replacement for processing smd messages from padscore
void processSmdMessages(void)
{
    if (!isSmdReady) {
        return;
    }

    SMDOutputMessage msg;
    while (smdIopReceive(smdIopIndex, &msg) != -0xc0005) {
        WMReport* report = &msg.report;
        DEBUG_PRINT("output request for handle %u size %u, cmd: 0x%X\n", msg.dev_handle, msg.length, report->report_id);

        Controller* controller = &controllers[msg.dev_handle];
        if (!controller->isInitialized) {
            continue;
        }

        if (controller->type == BLOOPAIR_CONTROLLER_OFFICIAL) {
#ifdef TESTING
            if (report->report_id == WM_REPORT_ID_MEMORY_WRITE) {
                uint32_t address = report->memory_write.address;
                uint8_t len = report->memory_write.size;
                uint8_t* data = report->memory_write.data;

                switch (address) {
                case 0x04a40040: // key part 1
                    if (len == 6) {
                        memcpy(controller->extensionKey, data, 6);
                    }
                    break;
                case 0x04a40046: // key part 2
                    if (len == 6) {
                        memcpy(controller->extensionKey + 6, data, 6);
                    }
                    break;
                case 0x04a4004c: // key part 3
                    if (len == 4) {
                        memcpy(controller->extensionKey + 12, data, 4);
                        wiimoteCryptoInit(&controller->cryptoState, controller->extensionKey);
                    }
                    break;
                }
            }
#endif
            // we can just directly send the report to official controllers
            sendOutputData(msg.dev_handle, report->data, msg.length);
        } else {
            // handle the rumble bit
            if (controller->rumble) {
                controller->rumble(controller, report->rumble.rumble);
            }

            // clear the rumble bit
            report->rumble.rumble = 0;

            switch (report->report_id) {
            case WM_REPORT_ID_RUMBLE: {
                // rumble will already be handled based on the rumble bit above, so just acknowledge the report
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_RUMBLE, 0);
                break;
            }
            case WM_REPORT_ID_LED: {
                if (controller->setPlayerLed) {
                    controller->setPlayerLed(controller, report->led.led_mask);
                }
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_LED, 0);
                break;
            }
            case WM_REPORT_ID_REPORT_MODE: {
                DEBUG_PRINT("data reporting set to %x:%x\n", report->report_mode.continous, report->report_mode.mode);
                controller->dataReportingMode = report->report_mode.mode;
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_REPORT_MODE, 0);
                break;
            }
            case WM_REPORT_ID_IR_ENABLE_1: {
                controller->irEnabled = 1;
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_IR_ENABLE_1, 0);
                break;
            }
            case WM_REPORT_ID_SPEAKER_ENABLE: {
                // this always returns 7, since the pro controller doesn't have a speaker
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_SPEAKER_ENABLE, 7);
                break;
            }
            case WM_REPORT_ID_REQUEST_STATUS: {
                WMStatusReport status;
                status.report_id = WM_REPORT_ID_STATUS;
                status.core_buttons = 0; // no need to set buttons here, we only send the extension buttons
                // TODO
                status.led_mask = 0x1;
                status.flags = 0x2;
                if (controller->irEnabled) {
                    status.flags |= 0x8;
                }
                status.unused = 0;
                // TODO
                status.battery_level = 0xff;
                sendInputData(msg.dev_handle, &status, sizeof(status));
                break;
            }
            case WM_REPORT_ID_MEMORY_WRITE: {
                writeMemory(msg.dev_handle, report->memory_write.address, report->memory_write.data, report->memory_write.size);
                break;
            }
            case WM_REPORT_ID_MEMORY_READ: {
                readMemory(msg.dev_handle, report->memory_read.address, report->memory_read.size);
                break;
            }
            case WM_REPORT_ID_SPEAKER_MUTE: {
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_SPEAKER_MUTE, report->speaker_mute.mute == 6 ? 7 : 0);
                break;
            }
            case WM_REPORT_ID_IR_ENABLE_2: {
                sendAcknowledgeReport(msg.dev_handle, WM_REPORT_ID_IR_ENABLE_2, 0);
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

    Controller* controller = &controllers[dev_handle];
    if (!controller->isInitialized) {
        return;
    }

    if (controller->type == BLOOPAIR_CONTROLLER_OFFICIAL) {
        // we can just pass received data from official controllers to padscore
        sendInputData(dev_handle, p_rpt, len);
#ifdef TESTING
        if (p_rpt[0] == WM_REPORT_ID_STATUS) {
            dumpHex(p_rpt, len);
        } else if (p_rpt[0] == WM_REPORT_ID_EXTENSION_DATA_REPORT) {
            static int cnt = 0;
            if (cnt++ % 16 == 0) {
                wiimoteDecrypt(&controller->cryptoState, p_rpt + 1, p_rpt + 1, 0, len - 1);
                dumpHex(p_rpt, len);
            }
        }
#endif
    } else {
        // pass received data to the controller
        if (controller->data) {
            controller->data(controller, p_rpt, len);
        }
    }
}
