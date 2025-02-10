; Interrupt handling code
section .text
global keyboard_handler_int    ; Ensure proper export
global load_idt               ; Ensure proper export

extern keyboard_handler

keyboard_handler_int:
    pushad          ; Push all general purpose registers
    cld            ; Clear direction flag
    call keyboard_handler
    popad          ; Restore all general purpose registers
    iret           ; Return from interrupt

load_idt:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp + 8]  ; Get IDT pointer parameter
    lidt [eax]          ; Load IDT
    pop eax
    pop ebp
    ret
