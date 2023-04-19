.arm

.extern btrmCustomLibHook
.extern btrmCheckCustomLib

.global _btrmCustomLibHook
_btrmCustomLibHook:
    @ check if this is a supported lib
    mov r0, r1
    bl btrmCheckCustomLib

    @ if not branch to fail
    cmp r0, #0
    ldreq pc, =0x11f03974

    @ call custom lib impl
    mov r0, r6
    mov r1, r11
    bl btrmCustomLibHook

    @ proceed with response
    mov r1, r0
    ldr pc, =0x11f028a8
