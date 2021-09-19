OUTPUT_ARCH(arm)

SECTIONS {
    .text 0x101312D0 : {
        _text_start = .;
        crt0.o(.init);
        *(.text*);
        *(.rodata*);
    }
    _text_end = .;

    /DISCARD/ : {
        *(*);
    }
}
