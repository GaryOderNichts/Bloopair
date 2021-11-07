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

#include "imports.h"

void (*const pad_printf)(const char* fmt, ...) = (void*) 0x11f7efd0;
void (*const usleep)(uint32_t usecs) = (void*) 0x11f84134;
int (*const IOS_CreateThread)(int (*fun)(void* arg), void* arg, uint32_t* stack_top, uint32_t stacksize, int priority, uint32_t flags) = (void*) 0x11f82e68;
int (*const IOS_JoinThread)(int threadid, uint32_t *returned_value) = (void*) 0x11f82e70;
int (*const IOS_CancelThread)(int threadid, int return_value) = (void*) 0x11f82e78;
int (*const IOS_GetCurrentThreadID)(void) = (void*) 0x11f82e80;
int (*const IOS_StartThread)(int threadid) = (void*) 0x11f82ea0;
int (*const IOS_SuspendThread)(int threadid) = (void*) 0x11f82ea8;
int (*const IOS_GetThreadPriority)(int threadid) = (void*) 0x11f82eb8;	
int (*const IOS_CreateMessageQueue)(uint32_t *ptr, uint32_t n_msgs) = (void*) 0x11f82ec8;
int (*const IOS_DestroyMessageQueue)(int queueid) = (void*) 0x11f82ed0;
int (*const IOS_SendMessage)(int queueid, uint32_t message, uint32_t flags) = (void*) 0x11f82ed8;
int (*const IOS_ReceiveMessage)(int queueid, uint32_t *message, uint32_t flags) = (void*) 0x11f82ee8;
int (*const IOS_CreateTimer)(int time_us, int repeat_time_us, int queueid, uint32_t message) = (void*) 0x11f82f00;
int (*const IOS_DestroyTimer)(int timerid) = (void*) 0x11f82f18;
int (*const IOS_Open)(const char* device, int mode)	= (void*) 0x11f83000;
int (*const IOS_Close)(int fd) = (void*) 0x11f83008;
int (*const IOS_Ioctl)(int fd, uint32_t request, void *input_buffer, uint32_t input_buffer_len, void *output_buffer, uint32_t output_buffer_len) = (void*) 0x11f83028;
int (*const IOS_Ioctlv)(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, IOSVec_t *vector) = (void*) 0x11f83030;
int (*const IOS_ResourceReply)(void *ipc_handle, int result) = (void*) 0x11f830b0;
int (*const IOS_CreateSemaphore)(int32_t maxCount, int32_t initialCount) = (void*) 0x11f83120;
int (*const IOS_WaitSemaphore)(int id, uint32_t tryWait) = (void*) 0x11f83128;
int (*const IOS_SignalSempahore)(int id) = (void*) 0x11f83130;
int (*const IOS_DestroySempahore)(int id) = (void*) 0x11f83138;
uint32_t (*const IOS_VirtToPhys)(uint32_t address) = (void*) 0x11f83118;
void* (*const IOS_Alloc)(uint32_t heap, uint32_t size) = (void*) 0x11f82fa0;
void* (*const IOS_AllocAligned)(uint32_t heap, uint32_t size, uint32_t alignment) = (void*) 0x11f82fa8;
void (*const IOS_Free)(uint32_t heap, void* ptr) = (void*) 0x11f82fb0;
void* (*const _memcpy)(void* dst, const void* src, uint32_t size) = (void*) 0x11f833ec;
void* (*const _memset)(void* ptr, int value, uint32_t size) = (void*) 0x11f83488;
int (*const _memcmp)(const void * ptr1, const void * ptr2, size_t num) = (void*) 0x11f83304;
int (*const _strncmp)(const char* str1, const char* str2, uint32_t size) = (void*) 0x11f83edc;
char* (*const _strncpy)(char* destination, const char* source, uint32_t num) = (void*) 0x11f83e80;
int (*const _vsnprintf)(char * s, size_t n, const char * format, va_list arg) = (void*) 0x11f83e04;
void (*const bdcpy)(uint8_t* a, const uint8_t* b) = (void*) 0x11f082a0;
void* (*const GKI_getbuf)(uint32_t size) = (void*) 0x11f2ecb0;
void (*const GKI_freebuf)(void* p_buf) = (void*) 0x11f2eb5c;
void* (*const GKI_getpoolbuf)(uint8_t pool_id) = (void*) 0x11f2ee64;
uint8_t (*const GKI_get_taskid)(void) = (void*) 0x11f2f9c4;
void (*const utl_freebuf)(void **p) = (void*) 0x11f08ec0;
void (*const bta_sys_sendmsg)(void* msg) = (void*) 0x11f08818;
void (*const bta_hh_snd_write_dev)(uint8_t dev_handle, uint8_t t_type, uint8_t param, uint16_t data, uint8_t rpt_id, BT_HDR *p_data) = (void*) 0x11f07744;
void (*const BTA_HhSendData)(uint8_t dev_handle, uint8_t* dev_bda, BT_HDR *p_buf) = (void*) 0x11f077c4;
void (*const BTA_HhClose)(uint8_t dev_handle) = (void*) 0x11f07634;
void (*const BTA_HhAddDev)(uint8_t* bda, uint16_t attr_mask, uint8_t sub_class, uint8_t app_id, uint32_t dl_len, uint8_t* dsc_list) = (void*) 0x11f07678;
uint8_t (*const BTM_ReadRemoteDeviceName)(uint8_t* remote_bda, void *p_cb) = (void*) 0x11f11820;
uint8_t (*const BTM_WriteStoredLinkKey)(uint8_t num_keys, uint8_t *bd_addr, uint8_t *link_key, void *p_cb) = (void*) 0x11f0e3ec;
void (*const BTA_DmSetAfhChannels)(uint8_t first, uint8_t last) = (void*) 0x11f04cdc;
void (*const BTA_DmAddDevice)(uint8_t* bd_addr, uint8_t* dev_class, uint8_t* link_key, uint32_t trusted_mask, uint8_t is_trusted, uint8_t key_type, uint8_t io_cap) = (void*) 0x11f04d88;
int (*const smdIopSendMessage)(int idx, void* ptr, uint32_t size) = (void*) 0x11f7e104;
int (*const smdIopReceive)(int idx, void* ptr) = (void*) 0x11f7e154;
uint8_t (*const btm_remove_acl)(uint8_t* bd_addr) = (void*) 0x11f09b58;
int (*const deleteDevice)(uint8_t* bd_addr) = (void*) 0x11f04ec8;
int (*const registerNewDevice)(uint8_t* addr, uint8_t* link_key, uint8_t* name) = (void*) 0x11f41370;

uint32_t* isSmdReady = (uint32_t*) 0x11fd5218;
uint32_t* bluetoothFlags = (uint32_t*) 0x11fd5180;
uint32_t* smdIopIndex = (uint32_t*) 0x11fd5210;
uint8_t* local_device_bdaddr = (uint8_t*) 0x1215775a;
