OUTPUT_ARCH(arm)

INCLUDE "imports.ld"

SECTIONS {
    . = 0x11F86000;

    .text : {
        _text_start = .;
        *(.text*);
        *(.rodata*);
        *(.data*);
    }
    _text_end = .;

    /* the kernel will copy the crypto tables here */
    .crypto : {
        PROVIDE(__ans_tbl = .);
        . += 0x2a;
        PROVIDE(__sboxes = .);
        . += 0x900;
    }

    . = 0x12159000;

    .bss : {
        *(.bss*);
    }

    /DISCARD/ : {
        *(*);
    }
}
