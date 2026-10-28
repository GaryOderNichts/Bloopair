OUTPUT_ARCH(arm)

INCLUDE "imports.sym"

SECTIONS {
    . = 0x11f86000;

    .text : {
        _pad_text_start = .;
        *(.text*);
        *(.rodata*);
        *(.data*);
    }

    /* the kernel will copy the crypto tables here */
    .crypto : {
        __ans_tbl = .;
        . += 0x2a;
        __sboxes = .;
        . += 0x900;
    }
    _pad_text_end = .;

    . = 0x12159000;

    .bss : {
        _pad_bss_start = .;
        *(.bss*);
    }
    _pad_bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}

/* The kernel maps 0x10000 for the text (pad.c, the region is free up to 0x11FC0000), the bss gets 0x8000 */
ASSERT(_pad_text_end - _pad_text_start < 0x10000, "ios_pad text is too big");
ASSERT(_pad_bss_end - _pad_bss_start < 0x8000, "ios_pad bss is too big");
