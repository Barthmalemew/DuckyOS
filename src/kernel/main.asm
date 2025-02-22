; =============================================================================
; Minimal 16-bit Real Mode Program
; Prints a message to the screen and then halts forever.
; =============================================================================

org 0x0000            ; Assemble assuming we load at linear address 0x0000.
bits 16               ; 16-bit code.

%define ENDL 0x0D, 0x0A  ; DOS/BIOS newline sequence: CR LF

; -----------------------------------------------------------------------------
; start:
;   Main entry point. It loads DS:SI with the address of our string and then
;   calls the "puts" routine. After printing, it disables interrupts and halts
;   the CPU.
; -----------------------------------------------------------------------------
start:
    mov si, msg_hello   ; DS:SI -> the string we want to print
    call puts           ; Print the string

.halt:
    cli                 ; Disable interrupts
    hlt                 ; Halt the CPU (it will stay here forever)

; -----------------------------------------------------------------------------
; puts:
;   Prints a null-terminated string pointed to by DS:SI in 16-bit real mode
;   using BIOS interrupt 0x10, AH=0x0E (teletype output).
; -----------------------------------------------------------------------------
puts:
    ; Save registers that we'll modify in the function
    push si
    push ax
    push bx

.loop:
    lodsb               ; AL = [DS:SI], SI++
    or al, al           ; Check if AL == 0 (null terminator)
    jz .done

    mov ah, 0x0E        ; BIOS Teletype function
    mov bh, 0           ; Display page = 0
    int 0x10            ; Print character in AL

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret

; -----------------------------------------------------------------------------
; Our message (null-terminated). We add a DOS/BIOS newline (CR, LF) before the 0.
; -----------------------------------------------------------------------------
msg_hello: db 'hello from the kernel fellow duck', ENDL, 0

; No more code. When the CPU reaches .halt, it stops forever.
; =============================================================================

