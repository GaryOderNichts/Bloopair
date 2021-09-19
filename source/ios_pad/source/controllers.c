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

Controller_t controllers[BTA_HH_MAX_KNOWN];

int report_semaphore = -1;

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

        IOS_WaitSemaphore(report_semaphore, 0);

        for (uint32_t i = 0; i < BTA_HH_MAX_KNOWN; i++) {
            Controller_t* controller = &controllers[i];
            ReportBuffer_t* report_buf = controller->reportData;
            // make sure the controller is initialized, has data reporting set, and has a report buf
            if (controller->isInitialized && report_buf) {
                // send the current state
                sendControllerInput(controller, report_buf->buttons, report_buf->left_stick_x, report_buf->right_stick_x, report_buf->left_stick_y, report_buf->right_stick_y);
            }
        }

        IOS_SignalSempahore(report_semaphore);
    }

    IOS_DestroyTimer(timer_id);
    IOS_DestroyMessageQueue(queue_id);
    return 0;
}

void initReportThread(void)
{
    // create a semaphore for sync
    report_semaphore = IOS_CreateSemaphore(1, 1);

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

    IOS_DestroySempahore(report_semaphore);

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

void controllerInit_switch(Controller_t* controller, uint8_t isProController, uint8_t right_joycon);
void controllerInit_xbox_one(Controller_t* controller);
void controllerInit_dualsense(Controller_t* controller);

int initController(uint8_t handle, uint8_t* name, uint16_t vendor_id, uint16_t product_id)
{
    DEBUG("initController handle %u name %s vid %x pid %x\n", handle, name, vendor_id, product_id);

    if (!report_thread_running) {
        initReportThread();
    }

    if (controllers[handle].isInitialized) {
        DEBUG("already initialized\n");
        BTA_HhClose(handle);
        return -1;
    }

    _memset(&controllers[handle], 0, sizeof(Controller_t));

    controllers[handle].handle = handle;
    controllers[handle].isInitialized = 1;
    
    if (isOfficialName((const char*) name)) {
        controllers[handle].isOfficialController = 1;
        return 0;
    }
    else {
        // TODO reading the vid and pid is annoying if we already have ongoing hid connections, for now just rely on the name

        if ((_strncmp((const char*) name, "Pro Controller", 0x40) == 0) || // switch pro controller
            (_strncmp((const char*) name, "Joy-Con (R)", 0x40) == 0) || // joycon r
            (_strncmp((const char*) name, "Joy-Con (L)", 0x40) == 0)) { // joycon l
            controllerInit_switch(&controllers[handle], 
                _strncmp((const char*) name, "Pro Controller", 0x40) == 0,
                _strncmp((const char*) name, "Joy-Con (R)", 0x40) == 0);
            return 0;
        }
        else if ((_strncmp((const char*) name, "Xbox Wireless Controller", 0x40) == 0)) { // xbox one controller
            controllerInit_xbox_one(&controllers[handle]);
            return 0;
        }
        else if ((_strncmp((const char*) name, "Wireless Controller", 0x40) == 0)) { // dualsense (i wonder how many other controllers are named "Wireless Controller" :P)
            controllerInit_dualsense(&controllers[handle]);
            return 0;
        }
    }

    DEBUG("unsupported device\n");

    // We don't support this device, close connection
    controllers[handle].isInitialized = 0;
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
    _memset(report_buf, 0, sizeof(report_buf));
    controller->reportData = report_buf;
}

void deinitContinuousReports(Controller_t* controller)
{
    IOS_Free(0xcaff, controller->reportData);
}
