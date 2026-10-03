#pragma once
#include <imports.h>

enum {
    UC_DATATYPE_UNDEFINED      = 0x00,
    UC_DATATYPE_UNSIGNED_BYTE  = 0x01,
    UC_DATATYPE_UNSIGNED_SHORT = 0x02,
    UC_DATATYPE_UNSIGNED_INT   = 0x03,
    UC_DATATYPE_SIGNED_INT     = 0x04,
    UC_DATATYPE_FLOAT          = 0x05,
    UC_DATATYPE_STRING         = 0x06,
    UC_DATATYPE_HEXBINARY      = 0x07,
    UC_DATATYPE_COMPLEX        = 0x08,
    UC_DATATYPE_INVALID        = 0xFF,
};

typedef struct {
    char name[64];
    uint32_t access;
    int dataType;
    int error;
    uint32_t dataSize;
    void* data;
} UCSysConfig;
CHECK_OFFSET(UCSysConfig, 0x00, name);
CHECK_OFFSET(UCSysConfig, 0x40, access);
CHECK_OFFSET(UCSysConfig, 0x44, dataType);
CHECK_OFFSET(UCSysConfig, 0x48, error);
CHECK_OFFSET(UCSysConfig, 0x4C, dataSize);
CHECK_OFFSET(UCSysConfig, 0x50, data);
CHECK_SIZE(UCSysConfig, 0x54);

int UCOpen(void);

int UCClose(int handle);

int UCReadSysConfig(int handle, uint32_t count, UCSysConfig* config);

int UCWriteSysConfig(int handle, uint32_t count, UCSysConfig* config);
