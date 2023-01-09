#pragma once

#include <stdint.h>

#define ALIGN4(x)                   (((x) + 3) & ~3)

#define ARM_B(addr, func)       (0xEA000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))                                                        // +-32MB
#define ARM_BL(addr, func)      (0xEB000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))                                                        // +-32MB
#define THUMB_B(addr, func)     ((0xE000 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x7FF)))                                                               // +-2KB
#define THUMB_BL(addr, func)    ((0xF000F800 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x0FFF)) | ((((uint32_t)(func) - (uint32_t)(addr) - 4) << 4) & 0x7FFF000))   // +-4MB


#define kernel_memcpy               ((void * (*)(void*, const void*, int))0x08131D04)
#define kernel_memset               ((void *(*)(void*, int, unsigned int))0x08131DA0)
#define kernel_strncpy              ((char *(*)(char*, const char*, unsigned int))0x081329B8)
#define disable_interrupts          ((int(*)())0x0812E778)
#define enable_interrupts           ((int(*)(int))0x0812E78C)
#define kernel_bsp_command_5        ((int (*)(const char*, int offset, const char*, int size, void *buffer))0x0812EC40)
#define kernel_ios_shutdown         ((void (*)(int)) 0xffffdc48)

static inline unsigned int disable_mmu(void)
{
    unsigned int control_register = 0;
    asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (control_register));
    asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (control_register & 0xFFFFEFFA));
    return control_register;
}

static inline void restore_mmu(unsigned int control_register)
{
    asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (control_register));
}

typedef struct {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t domain;
    uint32_t type;
    uint32_t cached;
} ios_map_shared_info_t;

#define _iosMapSharedUserExecution ((int (*)(void *descr)) 0x08124F88)
