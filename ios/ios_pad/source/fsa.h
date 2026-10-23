#pragma once
#include <imports.h>

enum {
    FSA_MOUNT_FLAG_LOCAL_MOUNT  = 0,
    FSA_MOUNT_FLAG_BIND_MOUNT   = 1,
    FSA_MOUNT_FLAG_GLOBAL_MOUNT = 2,
};

enum {
    FSA_UNMOUNT_FLAG_NONE       = 0x00000000,
    FSA_UNMOUNT_FLAG_FORCE      = 0x00000002,
    FSA_UNMOUNT_FLAG_BIND_MOUNT = 0x80000000,
};

typedef struct PACKED {
    uint32_t flags;
    uint32_t mode;
    uint32_t owner;
    uint32_t group;
    uint32_t size;
    uint32_t allocSize;
    uint64_t quotaSize;
    uint32_t entryId;
    uint64_t created;
    uint64_t modified;
    uint8_t unknown[0x30];
} FSAStat;
CHECK_OFFSET(FSAStat, 0x00, flags);
CHECK_OFFSET(FSAStat, 0x04, mode);
CHECK_OFFSET(FSAStat, 0x08, owner);
CHECK_OFFSET(FSAStat, 0x0C, group);
CHECK_OFFSET(FSAStat, 0x10, size);
CHECK_OFFSET(FSAStat, 0x14, allocSize);
CHECK_OFFSET(FSAStat, 0x18, quotaSize);
CHECK_OFFSET(FSAStat, 0x20, entryId);
CHECK_OFFSET(FSAStat, 0x24, created);
CHECK_OFFSET(FSAStat, 0x2C, modified);
CHECK_SIZE(FSAStat, 0x64);

typedef struct PACKED {
    FSAStat info;
    char name[256];
} FSADirectoryEntry;
CHECK_OFFSET(FSADirectoryEntry, 0x64, name);
CHECK_SIZE(FSADirectoryEntry, 0x164);

int FSA_Open(void);
int FSA_Close(int fd);

int FSA_Mount(int fd, const char* device_path, char* volume_path, uint32_t flags, char* arg_string, int arg_string_len);
int FSA_Unmount(int fd, const char* path, uint32_t flags);
int FSA_FlushVolume(int fd, const char* volume_path);

int FSA_MakeDir(int fd, char* path, uint32_t flags);
int FSA_OpenDir(int fd, char* path, int* outHandle);
int FSA_ReadDir(int fd, int handle, FSADirectoryEntry* out_data);
int FSA_CloseDir(int fd, int handle);

int FSA_OpenFile(int fd, const char* path, const char* mode, int* outHandle);
int FSA_ReadFile(int fd, void* data, uint32_t size, uint32_t cnt, int fileHandle, uint32_t flags);
int FSA_WriteFile(int fd, void* data, uint32_t size, uint32_t cnt, int fileHandle, uint32_t flags);
int FSA_FlushFile(int fd, int fileHandle);
int FSA_CloseFile(int fd, int fileHandle);
int FSA_SetPosFile(int fd, int fileHandle, uint32_t position);
int FSA_Remove(int fd, const char* path);
int FSA_ChangeMode(int fd, const char* path, int mode);
