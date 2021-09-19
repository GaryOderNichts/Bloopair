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

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "utils.h"

typedef struct {
    uint16_t event;
    uint16_t len;
    uint16_t offset;
    uint16_t layer_specific;
} BT_HDR;

typedef struct {
	void* ptr;
	uint32_t len;
	uint32_t unk;
} IOSVec_t;

extern void (*const pad_printf)(const char* fmt, ...);
extern void (*const usleep)(uint32_t usecs);
extern int (*const IOS_CreateThread)(int (*fun)(void* arg), void* arg, uint32_t* stack_top, uint32_t stacksize, int priority, uint32_t flags);
extern int (*const IOS_JoinThread)(int threadid, uint32_t *returned_value);
extern int (*const IOS_CancelThread)(int threadid, int return_value);
extern int (*const IOS_StartThread)(int threadid);
extern int (*const IOS_GetThreadPriority)(int threadid);
extern int (*const IOS_CreateMessageQueue)(uint32_t *ptr, uint32_t n_msgs);
extern int (*const IOS_DestroyMessageQueue)(int queueid);
extern int (*const IOS_ReceiveMessage)(int queueid, uint32_t *message, uint32_t flags);
extern int (*const IOS_CreateTimer)(int time_us, int repeat_time_us, int queueid, uint32_t message);
extern int (*const IOS_DestroyTimer)(int timerid);
extern int (*const IOS_Open)(const char* device, int mode);
extern int (*const IOS_Close)(int fd);
extern int (*const IOS_Ioctl)(int fd, uint32_t request, void *input_buffer, uint32_t input_buffer_len, void *output_buffer, uint32_t output_buffer_len);
extern int (*const IOS_Ioctlv)(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t *vector);
extern int (*const IOS_CreateSemaphore)(int32_t maxCount, int32_t initialCount);
extern int (*const IOS_WaitSemaphore)(int id, uint32_t tryWait);
extern int (*const IOS_SignalSempahore)(int id);
extern int (*const IOS_DestroySempahore)(int id);
extern uint32_t (*const IOS_VirtToPhys)(uint32_t address);
extern void* (*const IOS_Alloc)(uint32_t heap, uint32_t size);
extern void* (*const IOS_AllocAligned)(uint32_t heap, uint32_t size, uint32_t alignment);
extern void (*const IOS_Free)(uint32_t heap, void* ptr);
extern void* (*const _memcpy)(void* dst, const void* src, uint32_t size);
extern void* (*const _memset)(void* ptr, int value, uint32_t size);
extern int (*const _memcmp)(const void * ptr1, const void * ptr2, size_t num);
extern int (*const _strncmp)(const char* str1, const char* str2, uint32_t size);
extern char* (*const _strncpy)(char* destination, const char* source, uint32_t num);
extern int (*const _vsnprintf)(char * s, size_t n, const char * format, va_list arg);
extern void (*const bdcpy)(uint8_t* a, const uint8_t* b);
extern void* (*const GKI_getbuf)(uint32_t size);
extern void (*const GKI_freebuf)(void* p_buf);
extern void* (*const GKI_getpoolbuf)(uint8_t pool_id);
extern uint8_t (*const GKI_get_taskid)(void);
extern void (*const utl_freebuf)(void **p);
extern void (*const bta_sys_sendmsg)(void* msg);
extern void (*const BTA_HhSendData)(uint8_t dev_handle, uint8_t* dev_bda, BT_HDR *p_buf);
extern void (*const BTA_HhClose)(uint8_t dev_handle);
extern uint8_t (*const BTM_ReadRemoteDeviceName)(uint8_t* remote_bda, void *p_cb);
extern void (*const BTA_DmSetAfhChannels)(uint8_t first, uint8_t last);
extern int (*const smdIopSendMessage)(int idx, void* ptr, uint32_t size);
extern int (*const smdIopReceive)(int idx, void* ptr);
extern uint8_t (*const btm_remove_acl)(uint8_t* bd_addr);
extern int (*const deleteDevice)(uint8_t* bd_addr);

extern uint32_t* isSmdReady;
extern uint32_t* bluetoothFlags;
extern uint32_t* smdIopIndex;
