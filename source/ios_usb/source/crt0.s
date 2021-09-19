.section ".init"
.arm
.align 4

.extern _main
.type _main, %function

_start:
    b _main
