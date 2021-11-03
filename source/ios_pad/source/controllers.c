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

#include "controllers.h"
#include "utils.h"
#include "info_store.h"

Controller_t controllers[BTA_HH_MAX_KNOWN];

static void* report_thread_stack_base;
static int report_thread;
static uint8_t report_thread_running = 0;

#define CONTINUOUS_REPORT_THREAD_STACK_SIZE 1024

// send a report every 10 ms
#define REPORT_INTERVAL (10 * 1000)

static int continuous_report_thread(void* arg)
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
            Controller_t* controller = &controllers[i];
            ReportBuffer_t* report_buf = controller->reportData;
            // make sure the controller is initialized, has the reporting mode set, and has a report buf
            if (controller->isInitialized && controller->dataReportingMode == 0x3d && report_buf) {
                // send the current state
                sendControllerInput(controller, report_buf->buttons, report_buf->left_stick_x, report_buf->right_stick_x, report_buf->left_stick_y, report_buf->right_stick_y);
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
    report_thread_stack_base = IOS_AllocAligned(0xcaff, CONTINUOUS_REPORT_THREAD_STACK_SIZE, 0x20);

    // create the thread (priority needs to be lower than the current thread)
    report_thread = IOS_CreateThread(continuous_report_thread, NULL, (uint32_t*) ((uint8_t*) report_thread_stack_base + CONTINUOUS_REPORT_THREAD_STACK_SIZE),
        CONTINUOUS_REPORT_THREAD_STACK_SIZE, IOS_GetThreadPriority(0), 1);
    
    // start the thread
    IOS_StartThread(report_thread);
}

void deinitReportThread(void)
{
    // tell the thread to stop
    report_thread_running = 0;

    // wait until it finished
    IOS_JoinThread(report_thread, NULL);

    // free stack
    IOS_Free(0xcaff, report_thread_stack_base);
}

int isOfficialName(const char* name)
{
    // wii remote / pro controller
    if (_memcmp(name, "Nintendo RVL-CNT", 0x10) == 0) {
        return 1;
    }
    // balance board
    else if (_memcmp(name, "Nintendo RVL-WBC", 0x10) == 0) {
        return 1;
    }

    return 0;
}

void controllerInit_switch(Controller_t* controller);
void controllerInit_xbox_one(Controller_t* controller);
void controllerInit_dualsense(Controller_t* controller);
void controllerInit_dualshock4(Controller_t* controller);
void controllerInit_dualshock3(Controller_t* controller);

int initController(uint8_t handle, uint8_t magic, uint16_t vendor_id, uint16_t product_id)
{
    DEBUG("initController handle %u magic %x vid %x pid %x\n", handle, magic, vendor_id, product_id);

    if (!report_thread_running) {
        initReportThread();
    }

    Controller_t* controller = &controllers[handle];

    // if this controller was already initialized, deinitialize it first
    if (controller->isInitialized) {
        if (controller->deinit) {
            controller->deinit(&controllers[handle]);
        }
        controller->isInitialized = 0;
    }

    _memset(controller, 0, sizeof(Controller_t));

    controller->handle = handle;
    controller->isInitialized = 1;
    
    if (magic == MAGIC_OFFICIAL) {
        controller->isOfficialController = 1;
        return 0;
    }
    else if (magic == MAGIC_BLOOPAIR) {
        if ((vendor_id == 0x057e && product_id == 0x2009) || // switch pro controller
            (vendor_id == 0x057e && product_id == 0x2007) || // joycon r
            (vendor_id == 0x057e && product_id == 0x2006) || // joycon l
            (vendor_id == 0x057e && product_id == 0x2017)) { // snes controller
            controllerInit_switch(controller);
            return 0;
        }
        else if ((vendor_id == 0x045e && product_id == 0x02e0) || // xbox one s controller
                 (vendor_id == 0x045e && product_id == 0x02fd) || // xbox one s controller
                 (vendor_id == 0x045e && product_id == 0x0b00) || // xbox one elite controller
                 (vendor_id == 0x045e && product_id == 0x0b05) || // xbox one elite controller
                 (vendor_id == 0x045e && product_id == 0x0b0a)) { // xbox one adaptive controller
            controllerInit_xbox_one(controller);
            return 0;
        }
        else if (vendor_id == 0x054c && product_id == 0x0ce6) { // dualsense
            controllerInit_dualsense(controller);
            return 0;
        }
        else if ((vendor_id == 0x054c && product_id == 0x05c4) || // dualshock 4 v1
                 (vendor_id == 0x054c && product_id == 0x09cc) || // dualshock 4 v2
                 (vendor_id == 0x0f0d && product_id == 0x00f6) || // hori onyx
                 (vendor_id == 0x1532 && product_id == 0x100a) || // razer raiju tournament
                 (vendor_id == 0x146b && product_id == 0x0d01)) { // nacon ps4
            controllerInit_dualshock4(controller);
            return 0;
        }
        else if (vendor_id == 0x054c && product_id == 0x0268) { // dualshock 3
            controllerInit_dualshock3(controller);
            return 0;
        }
    }

    // We don't support this device, close connection
    DEBUG("unsupported device\n");
    controller->isInitialized = 0;
    return -1;
}

#define AXIS_BASE 0x800

void sendControllerInput(Controller_t* controller, uint32_t buttons, int16_t left_stick_x, int16_t right_stick_x, int16_t left_stick_y, int16_t right_stick_y)
{
    uint8_t data[22];
    _memset(data, 0, sizeof(data));

    left_stick_x = bswap16(left_stick_x + AXIS_BASE);
    right_stick_x = bswap16(right_stick_x + AXIS_BASE);
    left_stick_y = bswap16(left_stick_y * -1 + AXIS_BASE);
    right_stick_y = bswap16(right_stick_y * -1 + AXIS_BASE);

    _memcpy(&data[1], &left_stick_x, 2);
    _memcpy(&data[3], &right_stick_x, 2);
    _memcpy(&data[5], &left_stick_y, 2);
    _memcpy(&data[7], &right_stick_y, 2);

    data[9] = ~((buttons >> 8) & 0xff);
    data[10] = ~(buttons & 0xff);
    data[11] = (buttons >> 16) & 0xff;
    
    if (controller->isCharging) {
        data[11] |= 0xc;
    }

    data[11] = (~data[11]) & 0xf;

    data[11] |= controller->battery << 4;

    data[0] = 0x3d;
    encrypt(&controller->crypto, &data[1], &data[1], 0, sizeof(data) - 1);
    sendInputData(controller->handle, data, sizeof(data));
}

void initContinuousReports(Controller_t* controller)
{
    ReportBuffer_t* report_buf = IOS_Alloc(0xcaff, sizeof(ReportBuffer_t));
    _memset(report_buf, 0, sizeof(ReportBuffer_t));
    controller->reportData = report_buf;
}

void deinitContinuousReports(Controller_t* controller)
{
    IOS_Free(0xcaff, controller->reportData);
    controller->reportData = NULL;
}
