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

#include "controllers.h"
#include "utils.h"
#include "info_store.h"

#define WPAD_PRO_AXIS_BASE            0x800
#define WPAD_PRO_AXIS_NORMALIZE_VALUE 1140
#define WPAD_PRO_AXIS_NORMALIZE_RANGE (WPAD_PRO_AXIS_NORMALIZE_VALUE * 2)

#define REPORT_THREAD_STACK_SIZE 0x800

// send a report every 10 ms
#define REPORT_INTERVAL (10 * 1000)

Controller controllers[BTA_HH_MAX_KNOWN] = { 0 };

static void* report_thread_stack_base;
static int report_thread_id;
static uint8_t report_thread_running = 0;

static int reportThread(void* arg)
{
    // create a message queue and timer
    uint32_t message_buf;
    int queue_id = IOS_CreateMessageQueue(&message_buf, 1);
    int timer_id = IOS_CreateTimer(REPORT_INTERVAL, REPORT_INTERVAL, queue_id, 0xdeafcafe);

    while (report_thread_running) {
        // wait until the timer sends a message
        uint32_t message;
        IOS_ReceiveMessage(queue_id, &message, 0);

        for (uint32_t i = 0; i < BTA_HH_MAX_KNOWN; i++) {
            Controller* controller = &controllers[i];
            // make sure the controller is initialized
            if (controller->isInitialized) {
                // update the controller
                if (controller->update) {
                    controller->update(controller);
                }

                // make sure the controller has the reporting mode set and is ready to send data
                if (controller->dataReportingMode == WM_REPORT_ID_EXTENSION_DATA_REPORT && 
                    controller->isReady) {
                    // send the current state
                    sendControllerInput(controller);
                }
            }
        }
    }

    IOS_DestroyTimer(timer_id);
    IOS_DestroyMessageQueue(queue_id);
    return 0;
}

void initReportThread(void)
{
    report_thread_running = 1;

    // allocate a stack
    report_thread_stack_base = IOS_AllocAligned(LOCAL_PROCESS_HEAP_ID, REPORT_THREAD_STACK_SIZE, 0x20);

    // create the thread (priority needs to be lower than the current thread)
    report_thread_id = IOS_CreateThread(reportThread, NULL, (uint8_t*) report_thread_stack_base + REPORT_THREAD_STACK_SIZE,
        REPORT_THREAD_STACK_SIZE, IOS_GetThreadPriority(0), 0);

    // start the thread
    IOS_StartThread(report_thread_id);
}

void deinitReportThread(void)
{
    // tell the thread to stop
    report_thread_running = 0;

    // wait until it finished
    IOS_JoinThread(report_thread_id, NULL);

    // free stack
    IOS_Free(LOCAL_PROCESS_HEAP_ID, report_thread_stack_base);
}

#define COMPARE_NAME(x) (strncmp(name, x, sizeof(x) - 1) == 0)
int isOfficialName(const char* name)
{
    return COMPARE_NAME("Nintendo RVL-CNT") || // wii remote / pro controller
           COMPARE_NAME("Nintendo RVL-WBC"); // balance board
}

int isSwitchControllerName(const char* name)
{
    return COMPARE_NAME("NintendoGamepad") ||
           COMPARE_NAME("Joy-Con") ||
           COMPARE_NAME("Pro Controller") ||
           COMPARE_NAME("Lic Pro Controller") ||
           COMPARE_NAME("NES Controller") ||
           COMPARE_NAME("HVC Controller") ||
           COMPARE_NAME("SNES Controller") ||
           COMPARE_NAME("N64 Controller") ||
           COMPARE_NAME("MD/Gen Control Pad");
}
#undef COMPARE_NAME

void controllerInit_switch(Controller* controller);
void controllerInit_xbox_one(Controller* controller);
void controllerInit_dualsense(Controller* controller);
void controllerInit_dualshock4(Controller* controller);
void controllerInit_dualshock3(Controller* controller);

int initController(uint8_t* bda, uint8_t handle)
{
    StoredInfo* info = store_get_device_info(bda);
    if (!info) {
        DEBUG_PRINT("Failed to get info for device\n");
        return -1;
    }

    uint8_t magic = info->magic;
    uint16_t vendor_id = info->vendor_id;
    uint16_t product_id = info->product_id;

    DEBUG_PRINT("initController handle %u magic %x vid %x pid %x\n", handle, magic, vendor_id, product_id);

    if (!report_thread_running) {
        initReportThread();
    }

    // Make sure the config is initialized at this point
    Configuration_Init();

    Controller* controller = &controllers[handle];

    // if this controller was already initialized, deinitialize it first
    if (controller->isInitialized) {
        if (controller->deinit) {
            controller->deinit(&controllers[handle]);
        }
        controller->isInitialized = 0;
    }

    memset(controller, 0, sizeof(Controller));

    controller->handle = handle;
    memcpy(controller->bda, bda, 6);
    controller->isInitialized = 1;

    controller->vendor_id = vendor_id;
    controller->product_id = product_id;

    if (magic == MAGIC_OFFICIAL) {
        controller->type = BLOOPAIR_CONTROLLER_OFFICIAL;
        return 0;
    } else if (magic == MAGIC_BLOOPAIR) {
        if ((vendor_id == 0x057e && product_id == 0x2006) || // joycon l
            (vendor_id == 0x057e && product_id == 0x2007) || // joycon r
            (vendor_id == 0x057e && product_id == 0x2009) || // switch pro controller
            (vendor_id == 0x057e && product_id == 0x2017) || // snes controller
            (vendor_id == 0x057e && product_id == 0x2019) || // n64 controller
            (vendor_id == 0x057e && product_id == 0x201a)) { // genesis/megadrive controller

            // switch controllers paired with older bloopair version won't use MAGIC_SWITCH yet
            info->magic = MAGIC_SWITCH;

            controllerInit_switch(controller);
            return 0;
        } else if ((vendor_id == 0x045e && product_id == 0x02e0) || // xbox one s controller
                   (vendor_id == 0x045e && product_id == 0x02fd) || // xbox one s controller
                   (vendor_id == 0x045e && product_id == 0x0b00) || // xbox one elite controller
                   (vendor_id == 0x045e && product_id == 0x0b05) || // xbox one elite controller
                   (vendor_id == 0x045e && product_id == 0x0b0a)) { // xbox one adaptive controller
            controllerInit_xbox_one(controller);
            return 0;
        } else if (vendor_id == 0x054c && product_id == 0x0ce6) { // dualsense
            controllerInit_dualsense(controller);
            return 0;
        } else if ((vendor_id == 0x054c && product_id == 0x05c4) || // dualshock 4 v1
                   (vendor_id == 0x054c && product_id == 0x09cc) || // dualshock 4 v2
                   (vendor_id == 0x0f0d && product_id == 0x00f6) || // hori onyx
                   (vendor_id == 0x1532 && product_id == 0x100a) || // razer raiju tournament
                   (vendor_id == 0x146b && product_id == 0x0d01)) { // nacon ps4
            controllerInit_dualshock4(controller);
            return 0;
        } else if (vendor_id == 0x054c && product_id == 0x0268) { // dualshock 3
            controllerInit_dualshock3(controller);
            return 0;
        }
    } else if (magic == MAGIC_SWITCH) {
        controllerInit_switch(controller);
        return 0;
    }

    // We don't support this device, close connection
    DEBUG_PRINT("unsupported device\n");
    controller->isInitialized = 0;
    return -1;
}

void sendControllerInput(Controller* controller)
{
    // map the raw controller input to the wii u pro mapping
    BloopairReportBuffer repBuf;
    mapControllerInput(controller, &controller->reportBuffer, &repBuf);

    WPADProReport report;
    memset(&report, 0, sizeof(report));

    report.report_id = WM_REPORT_ID_EXTENSION_DATA_REPORT;
    report.data.left_stick_x = bswap16(repBuf.left_stick_x + WPAD_PRO_AXIS_BASE);
    report.data.right_stick_x = bswap16(repBuf.right_stick_x + WPAD_PRO_AXIS_BASE);
    report.data.left_stick_y = bswap16(WPAD_PRO_AXIS_BASE - repBuf.left_stick_y);
    report.data.right_stick_y = bswap16(WPAD_PRO_AXIS_BASE - repBuf.right_stick_y);

    // These bits are all low-active
    report.data.buttons = ~repBuf.buttons & 0xffff;
    report.data.stick_buttons = ~(repBuf.buttons >> 16) & 0x3;
    report.data.usb_connected = report.data.charging = !controller->isCharging;

    report.data.battery = controller->battery;

    wiimoteEncrypt(&controller->cryptoState, &report.data, &report.data, 0, sizeof(report.data));
    sendInputData(controller->handle, &report, sizeof(report));
}

static int16_t getStickAxis(BloopairReportBuffer* in, uint8_t from)
{
    int16_t axis = 0;
    switch (from) {
    case BLOOPAIR_PRO_STICK_L_UP:
        axis = -in->left_stick_y;
        break;
    case BLOOPAIR_PRO_STICK_L_DOWN:
        axis = in->left_stick_y;
        break;
    case BLOOPAIR_PRO_STICK_L_LEFT:
        axis = -in->left_stick_x;
        break;
    case BLOOPAIR_PRO_STICK_L_RIGHT:
        axis = in->left_stick_x;
        break;
    case BLOOPAIR_PRO_STICK_R_UP:
        axis = -in->right_stick_y;
        break;
    case BLOOPAIR_PRO_STICK_R_DOWN:
        axis = in->right_stick_y;
        break;
    case BLOOPAIR_PRO_STICK_R_LEFT:
        axis = -in->right_stick_x;
        break;
    case BLOOPAIR_PRO_STICK_R_RIGHT:
        axis = in->right_stick_x;
        break;
    }

    return CLAMP(axis, 0, WPAD_PRO_AXIS_NORMALIZE_VALUE);
}

static void setStickAxis(BloopairReportBuffer* out, uint8_t to, int16_t value)
{
#define SET_STICK(stick, op) \
    out->stick = CLAMP(out->stick op value, -WPAD_PRO_AXIS_NORMALIZE_VALUE, WPAD_PRO_AXIS_NORMALIZE_VALUE)

    switch (to) {
    case BLOOPAIR_PRO_STICK_L_UP:
        SET_STICK(left_stick_y, -);
        break;
    case BLOOPAIR_PRO_STICK_L_DOWN:
        SET_STICK(left_stick_y, +);
        break;
    case BLOOPAIR_PRO_STICK_L_LEFT:
        SET_STICK(left_stick_x, -);
        break;
    case BLOOPAIR_PRO_STICK_L_RIGHT:
        SET_STICK(left_stick_x, +);
        break;
    case BLOOPAIR_PRO_STICK_R_UP:
        SET_STICK(right_stick_y, -);
        break;
    case BLOOPAIR_PRO_STICK_R_DOWN:
        SET_STICK(right_stick_y, +);
        break;
    case BLOOPAIR_PRO_STICK_R_LEFT:
        SET_STICK(right_stick_x, -);
        break;
    case BLOOPAIR_PRO_STICK_R_RIGHT:
        SET_STICK(right_stick_x, +);
        break;
    }

#undef SET_STICK
}

void mapControllerInput(Controller* controller, BloopairReportBuffer* in, BloopairReportBuffer* out)
{
    memset(out, 0, sizeof(*out));

    const MappingConfiguration* mapping = controller->mapping;
    if (!mapping) {
        printf("No mapping for controller %d\n", controller->handle);
        return;
    }

    for (uint8_t i = 0; i < mapping->num; i++) {
        const BloopairMappingEntry* e = &mapping->mappings[i];
        if (e->from >= BLOOPAIR_PRO_STICK_MIN) {
            if (e->to >= BLOOPAIR_PRO_STICK_MIN) {
                // map stick to stick
                setStickAxis(out, e->to, getStickAxis(in, e->from));
            } else {
                // map stick to button
                if (getStickAxis(in, e->from) >= controller->commonConfig->stickAsButtonDeadzone) {
                    out->buttons |= BTN(e->to);
                }
            }
        } else {
            if (in->buttons & BTN(e->from)) {
                if (e->to >= BLOOPAIR_PRO_STICK_MIN) {
                    // map button to stick
                    setStickAxis(out, e->to, WPAD_PRO_AXIS_NORMALIZE_VALUE);
                } else {
                    // map button to button
                    out->buttons |= BTN(e->to);
                }
            }
        }
    }
}

uint8_t ledMaskToPlayerNum(uint8_t mask)
{
    switch (mask & 0xf) {
    case 0b0001: return 1;
    case 0b0010: return 2;
    case 0b0100: return 3;
    case 0b1000: return 4;
    case 0b0011: return 5;
    case 0b0101: return 6;
    case 0b1001: return 7;
    }

    return 0;
}

int16_t scaleStickAxis(uint32_t val, uint32_t range)
{
    return (int16_t) (((int32_t) val - (range / 2)) * WPAD_PRO_AXIS_NORMALIZE_RANGE / range);
}
