bits 16            ; Tell NASM we're using 16 bit code
org 0x7C00         ; Tell NASM where our code will be loaded

boot:
    ; Setup segments
    xor ax, ax     ; Zero AX register
    mov ds, ax     ; Set Data Segment to 0
    mov es, ax     ; Set Extra Segment to 0
    mov ss, ax     ; Set Stack Segment to 0
    mov sp, 0x7C00 ; Set Stack Pointer to 0x7C00

    ; Print message
    mov si, message   ; Put string address in SI
    call print_string ; Call print string function
    
    ; Infinite loop
    jmp $            ; Jump to current location (loop forever)

print_string:
    pusha           ; Push all registers onto stack
.loop:
    lodsb           ; Load byte from SI into AL and increment SI
    or al, al       ; Is AL = 0 ?
    jz .done        ; If AL = 0, jump to done
    mov ah, 0x0E    ; Int 10h/AH = 0Eh - teletype output
    mov bh, 0       ; Page number
    int 0x10        ; Call video interrupt
    jmp .loop       ; Jump to loop
.done:
    popa            ; Pop all registers from stack
    ret             ; Return to caller

message: db 'Hello from our bootloader!', 13, 10, 0  ; Our message to print (13,10 is CR/LF)

times 510-($-$$) db 0   ; Pad with zeros up to 510 bytes
dw 0xAA55              ; Add boot signature at the end
