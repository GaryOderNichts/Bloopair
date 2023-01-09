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

#include <imports.h>

#define IOS_COMMAND_INVALID     0x00
#define IOS_OPEN                0x01
#define IOS_CLOSE               0x02
#define IOS_READ                0x03
#define IOS_WRITE               0x04
#define IOS_SEEK                0x05
#define IOS_IOCTL               0x06
#define IOS_IOCTLV              0x07
#define IOS_REPLY               0x08
#define IOS_IPC_MSG0            0x09
#define IOS_IPC_MSG1            0x0A
#define IOS_IPC_MSG2            0x0B
#define IOS_SUSPEND             0x0C
#define IOS_RESUME              0x0D
#define IOS_SVCMSG              0x0E

typedef struct PACKED {
	uint32_t command;
	uint32_t result;
	uint32_t fd;
	uint32_t flags;
	uint32_t client_cpu;
	uint32_t client_pid;
	uint64_t client_gid;
	uint32_t server_handle;

	union {
	    uint32_t args[5];

		struct {
			char* device;
			uint32_t mode;
			uint32_t resultfd;
		} open;

		struct {
			void* data;
			uint32_t length;
		} read, write;

		struct {
			int32_t offset;
			int32_t origin;
		} seek;

		struct {
			uint32_t command;

			uint32_t* buffer_in;
			uint32_t  length_in;
			uint32_t* buffer_out;
			uint32_t  length_out;
		} ioctl;

		struct {
			uint32_t command;

			uint32_t num_in;
			uint32_t num_out;
			IOSVec_t* vecs;
		} ioctlv;
	};

	uint32_t prev_command;
	uint32_t prev_fd;
	uint32_t virt0;
	uint32_t virt1;
} IPCMessage_t;
