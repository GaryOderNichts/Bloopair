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
#include <assert.h>

#include "utils.h"

#define LOCAL_PROCESS_HEAP_ID 0xcafe
#define CROSS_PROCESS_HEAP_ID 0xcaff

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

int IOS_CreateThread(int (*fun)(void* arg), void* arg, uint32_t* stack_top, uint32_t stacksize, int priority, uint32_t flags);
int IOS_JoinThread(int threadid, uint32_t *returned_value);
int IOS_CancelThread(int threadid, int return_value);
int IOS_GetCurrentThreadID(void);
int IOS_StartThread(int threadid);
int IOS_SuspendThread(int threadid);
int IOS_GetThreadPriority(int threadid);
int IOS_CreateMessageQueue(uint32_t *ptr, uint32_t n_msgs);
int IOS_DestroyMessageQueue(int queueid);
int IOS_SendMessage(int queueid, uint32_t message, uint32_t flags);
int IOS_ReceiveMessage(int queueid, uint32_t *message, uint32_t flags);
int IOS_CreateTimer(int time_us, int repeat_time_us, int queueid, uint32_t message);
int IOS_DestroyTimer(int timerid);
int IOS_GetUpTime64(uint64_t* outTime);
int IOS_Open(const char* device, int mode);
int IOS_Close(int fd);
int IOS_Ioctl(int fd, uint32_t request, void *input_buffer, uint32_t input_buffer_len, void *output_buffer, uint32_t output_buffer_len);
int IOS_Ioctlv(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t *vector);
int IOS_ResourceReply(void *ipc_handle, int result);
int IOS_CreateSemaphore(int32_t maxCount, int32_t initialCount);
int IOS_WaitSemaphore(int id, uint32_t tryWait);
int IOS_SignalSemaphore(int id);
int IOS_DestroySemaphore(int id);
uint32_t IOS_VirtToPhys(uint32_t address);
void* IOS_Alloc(uint32_t heap, uint32_t size);
void* IOS_AllocAligned(uint32_t heap, uint32_t size, uint32_t alignment);
void IOS_Free(uint32_t heap, void* ptr);
void bdcpy(uint8_t* a, const uint8_t* b);
void* GKI_getbuf(uint32_t size);
void GKI_freebuf(void* p_buf);
void* GKI_getpoolbuf(uint8_t pool_id);
uint8_t GKI_get_taskid(void);
void utl_freebuf(void **p);
void bta_sys_sendmsg(void* msg);
void bta_hh_snd_write_dev(uint8_t dev_handle, uint8_t t_type, uint8_t param, uint16_t data, uint8_t rpt_id, BT_HDR *p_data);
void BTA_HhSendData(uint8_t dev_handle, uint8_t* dev_bda, BT_HDR *p_buf);
void BTA_HhClose(uint8_t dev_handle);
void BTA_HhAddDev(uint8_t* bda, uint16_t attr_mask, uint8_t sub_class, uint8_t app_id, uint32_t dl_len, uint8_t* dsc_list);
uint8_t BTM_ReadRemoteDeviceName(uint8_t* remote_bda, void *p_cb);
uint8_t BTM_WriteStoredLinkKey(uint8_t num_keys, uint8_t *bd_addr, uint8_t *link_key, void *p_cb);
void BTA_DmSetAfhChannels(uint8_t first, uint8_t last);
void BTA_DmAddDevice(uint8_t* bd_addr, uint8_t* dev_class, uint8_t* link_key, uint32_t trusted_mask, uint8_t is_trusted, uint8_t key_type, uint8_t io_cap);
int smdIopSendMessage(int idx, void* ptr, uint32_t size);
int smdIopReceive(int idx, void* ptr);
uint8_t btm_remove_acl(uint8_t* bd_addr);
int deleteDevice(uint8_t* bd_addr);
int registerNewDevice(uint8_t* addr, uint8_t* link_key, uint8_t* name);

extern uint32_t isSmdReady;
extern uint32_t smdIopIndex;
extern uint8_t local_device_bdaddr[6];
