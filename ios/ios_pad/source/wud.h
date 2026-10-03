/*
 *   Copyright (C) 2026 GaryOderNichts
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
#include "imports.h"

enum {
    WUD_DEVICE_STATUS_REGISTERING   = 2,
    WUD_DEVICE_STATUS_CONNECTED     = 8,
    WUD_DEVICE_STATUS_AUTH_COMPLETE = 12,
};

enum {
    WUD_SYNC_STATE_INIT                    = 0,
    WUD_SYNC_STATE_REGISTER_DEVICE         = 18,
    WUD_SYNC_STATE_STORED_DEV_INFO_TO_NAND = 22,
    WUD_SYNC_STATE_COMPLETE                = 23,
    WUD_SYNC_STATE_ERROR                   = 255,
};

enum {
    WUD_MESSAGE_SYNC_REGISTER_DEVICE = 13,
    WUD_MESSAGE_SYNC_COMPLETE        = 22,
    WUD_MESSAGE_SYNC_ERROR           = 255,
};

typedef struct {
    int threadHandle; // This will be 0 since N memsets it after creating the thread lol
    int queueHandle;
    uint32_t queueMessages[64];
    int semaphoreHandle;
} WUDContext;
CHECK_SIZE(WUDContext, 0x10c);

typedef struct {
    char name[64];
    uint8_t address[6];
    uint8_t link_key[16];
    uint8_t handle;
    uint8_t sub_class;
    uint8_t app_id;
    uint8_t status;
    uint8_t unk0x5a;
    uint8_t unk0x5b;
    uint8_t _unk[56];
} WUDDevInfo;
CHECK_OFFSET(WUDDevInfo, 0x5b, unk0x5b);
CHECK_SIZE(WUDDevInfo, 0x94);

typedef struct wud_device {
    WUDDevInfo* info;
    struct wud_device* prev;
    struct wud_device* next;
} WUDDevice;
CHECK_SIZE(WUDDevice, 0xC);

typedef struct {
    uint32_t unk0x0;
    uint32_t unk0x4;
    uint8_t syncState;
    uint8_t deleteState;
    uint8_t linkKeyState;
    uint8_t unk0xb;
    uint8_t unk0xc;
    uint8_t unk0xd;
    uint8_t devNums;
    WUDDevice* firstDevice;
    WUDDevice* lastDevice;
    WUDDevice devices[10];
    WUDDevInfo deviceInfos[10];
    uint8_t connNums;
    uint8_t linkNums;
    uint8_t unk0x65a;
    uint8_t unk0x65b;
    uint8_t syncLoop;
    uint8_t syncType;
    uint8_t connMode;
    uint8_t discMode;
    uint32_t unk0x660;
    uint32_t unk0x664;
    uint8_t unk0x668[8];
    uint8_t unk0x670[6];
    uint8_t hostAddress[6];
    uint8_t unk0x67c;
    uint8_t unk0x67d;
    uint8_t unk0x67e;
    uint8_t unk0x67f;
    uint8_t unk0x680[8];
    uint8_t unk0x688;
    uint8_t unk0x689;
    uint16_t unk0x68a;
    uint8_t unk0x68c;
    uint8_t unk0x68d;
    uint16_t unk0x68e;
    uint16_t unk0x690;
    uint8_t maxDevices;
    uint8_t unk0x693;
} WUDControlBlock;
CHECK_OFFSET(WUDControlBlock, 0x676, hostAddress);
CHECK_OFFSET(WUDControlBlock, 0xe, devNums);
CHECK_SIZE(WUDControlBlock, 0x694);

extern WUDContext gWUDContext;

extern WUDControlBlock gWBC;

int WUDRegisterDevice(const uint8_t* addr, const uint8_t* link_key, const char* name);

WUDDevInfo* WUDiGetDiscoverDevice(void);

WUDDevInfo* WUDiGetDevInfo(const uint8_t* addr);

void WUDiSetDevAddrForHandle(uint8_t handle, const uint8_t* addr);

void WUDiSetQueueSizeForHandle(uint8_t handle, uint8_t queueSize);

void WUDiMoveTopStdDevInfoPtr(WUDDevInfo* device);

void WUDiMoveBottomStdDevInfoPtr(WUDDevInfo* device);

void WUDiMoveTopOfDisconnectedStdDevice(WUDDevInfo* device);

void WUDiRemoveDevice(const uint8_t* addr);

int WUDIsLinkedWBC(void);

void bt_rm_ppc_response_hid_event(uint32_t event, WUDDevInfo* info, int is_connected, uint8_t num_devices);

void bt_rm_ppc_response_DevNum(uint8_t devNum);

uint8_t WUD_ConnectDevice(uint8_t handle, const uint8_t* addr, uint8_t is_error);

void WUD_DisconnectDevice(uint8_t handle, const uint8_t* addr);
