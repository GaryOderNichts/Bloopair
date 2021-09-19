OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x08135000);

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
