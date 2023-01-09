OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x08139200);

    __KERNEL_CODE_START = .;
    .text : {
        crt0.o(.init)
        *(.text*);
    }
    .rodata : {
        *(.rodata*);
    }
    .data : {
        *(.data*);
    }
    .bss : {
        *(.bss*);
    }
    __KERNEL_CODE_END = .;

    /DISCARD/ : {
        *(*);
    }
}

ASSERT((SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data)  + SIZEOF(.bss)) < 0x800, "ios_kernel is too big");
