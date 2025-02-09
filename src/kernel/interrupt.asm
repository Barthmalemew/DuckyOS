global load_idt
global keyboard_handler_int
extern keyboard_handler

section .text

load_idt:
    mov edx, [esp + 4]    ; Get pointer to IDT
    lidt [edx]            ; Load IDT
    sti                   ; Enable interrupts - add this line
    ret                   ; Remove the pushfd/popfd stuff

keyboard_handler_int:
    pushad               ; Push all general-purpose registers
    cld                  ; Clear direction flag
    call keyboard_handler
    mov al, 0x20        ; Send EOI (End of Interrupt)
    out 0x20, al        ; to PIC
    popad               ; Restore all general-purpose registers
    iret                ; Return from interrupt

section .data
    align 4             ; Align on a 4-byte boundary
