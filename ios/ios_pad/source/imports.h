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

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"

#define LOCAL_PROCESS_HEAP_ID 0xcafe
#define CROSS_PROCESS_HEAP_ID 0xcaff

typedef struct {
	void* ptr;
	uint32_t len;
	uint32_t unk;
} IOSVec_t;

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
} IOSCalendarTime_t;

int IOS_CreateThread(int (*fun)(void* arg), void* arg, void* stack_top, uint32_t stacksize, int priority, uint32_t flags);
int IOS_JoinThread(int threadid, uint32_t *returned_value);
int IOS_CancelThread(int threadid, int return_value);
int IOS_GetCurrentThreadID(void);
int IOS_StartThread(int threadid);
int IOS_SuspendThread(int threadid);
int IOS_YieldCurrentThread(void);
int IOS_GetThreadPriority(int threadid);
int IOS_CreateMessageQueue(uint32_t *ptr, uint32_t n_msgs);
int IOS_DestroyMessageQueue(int queueid);
int IOS_SendMessage(int queueid, uint32_t message, uint32_t flags);
int IOS_ReceiveMessage(int queueid, uint32_t *message, uint32_t flags);
int IOS_CreateTimer(int time_us, int repeat_time_us, int queueid, uint32_t message);
int IOS_DestroyTimer(int timerid);
int IOS_GetUpTime64(uint64_t* outTime);
int IOS_GetAbsTimeCalendar(IOSCalendarTime_t* time);
int IOS_GetAbsTime64(uint64_t* outTime);
int IOS_Open(const char* device, int mode);
int IOS_Close(int fd);
int IOS_Ioctl(int fd, uint32_t request, void *input_buffer, uint32_t input_buffer_len, void *output_buffer, uint32_t output_buffer_len);
int IOS_Ioctlv(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t *vector);
int IOS_ResourceReply(void *ipc_handle, int result);
int IOS_CreateSemaphore(int32_t maxCount, int32_t initialCount);
int IOS_WaitSemaphore(int id, uint32_t tryWait);
int IOS_SignalSemaphore(int id);
int IOS_DestroySemaphore(int id);
void IOS_FlushDCache(void* ptr, uint32_t len);
uint32_t IOS_VirtToPhys(uint32_t address);
void* IOS_Alloc(uint32_t heap, uint32_t size);
void* IOS_AllocAligned(uint32_t heap, uint32_t size, uint32_t alignment);
void IOS_Free(uint32_t heap, void* ptr);

typedef enum {
	IOSC_AES_MODE_ECB = 0x00,
	IOSC_AES_MODE_CBC = 0x01,
	IOSC_AES_MODE_CTR = 0x02,
} IOSCAesMode;

int _ioscOpen(void);
int IOSC_CreateObject(int* handle, int type, int subtype);
int IOSC_DeleteObject(int* handle);
int IOSC_ImportSecretKey(int importedHandle, int verifyHandle, int decryptHandle, int flags, void* signature,
	uint32_t signatureSize, void* ivData, uint32_t ivSize, void* key, uint32_t keySize); 
int IOSC_EncryptBlocks(int handle, IOSCAesMode mode, void* ivOrNonce, uint32_t ivOrNonceSize, void* inData,
	uint32_t inSize, void* outData, uint32_t outSize);

int smdIopSendMessage(int idx, void* ptr, uint32_t size);
int smdIopReceive(int idx, void* ptr);
int deleteDevice(uint8_t* bd_addr);
const char* bdaddr_to_string(uint8_t* bd_addr);

extern int cryptoHandle;
extern uint32_t isSmdReady;
extern uint32_t smdIopIndex;
