#include <stdint.h>

void _main()
{
    void(*ios_shutdown)(int) = (void(*)(int)) 0x1012EE4C;

    int(*reply)(int, int) = (int(*)(int, int)) 0x1012ED04;

    int saved_handle = *(volatile int*) 0x0012F000;
    int myret = reply(saved_handle, 0);
    if (myret != 0) {
        ios_shutdown(1);
    }

    // stack pointer will be 0x1016AE30
    // link register will be 0x1012EACC
    asm("LDR SP, newsp\n"
        "LDR R0, newr0\n"
        "LDR LR, newlr\n"
        "LDR PC, newpc\n"
        "newsp: .word 0x1016AE30\n"
        "newlr: .word 0x1012EACC\n"
        "newr0: .word 0x10146080\n"
        "newpc: .word 0x10111164\n");
}
