global load_idt
global keyboard_handler_int
extern keyboard_handler

section .text

load_idt:
    mov edx, [esp + 4]    ; Get pointer to IDT
    lidt [edx]            ; Load IDT
    sti                   ; Enable interrupts
    ret

keyboard_handler_int:
    pushad
    call keyboard_handler
    popad
    iret
