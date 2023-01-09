OUTPUT_ARCH(arm)

INCLUDE "imports.ld"

SECTIONS {
    . = 0x11f86000;

    .text : {
        _text_start = .;
        *(.text*);
        *(.rodata*);
        *(.data*);
    }

    /* the kernel will copy the crypto tables here */
    .crypto : {
        PROVIDE(__ans_tbl = .);
        . += 0x2a;
        PROVIDE(__sboxes = .);
        . += 0x900;
    }
    _text_end = .;

    . = 0x12159000;

    .bss : {
        _bss_start = .;
        *(.bss*);
    }
    _bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}

ASSERT(_text_end < 0x11fc0000, "ios_pad text is too big");
ASSERT(_bss_end < 0x12300000, "ios_pad bss is too big");
