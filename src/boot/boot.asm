; ==================================
; DuckyOS Bootloader
; 32-bit protected mode entry point
; Assumes GRUB multiboot
; ==================================

; Multiboot constants
MBOOT_PAGE_ALIGN    equ 1<<0     ; Align loaded modules on page boundaries
MBOOT_MEM_INFO      equ 1<<1     ; Provide memory map
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; Multiboot Magic value
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

; VGA buffer constants
VGA_BUFFER          equ 0xB8000  ; Location of VGA text mode buffer
VGA_WIDTH           equ 80       ; Screen width in characters
VGA_HEIGHT          equ 25       ; Screen height in characters
VGA_BYTES_PER_CHAR  equ 2        ; Each character takes 2 bytes (char + attribute)
VGA_WHITE_ON_BLACK  equ 0x0F     ; Color attribute for white text on black background

; Tell NASM this is 32-bit code
[BITS 32]
[GLOBAL start]      ; Make entry point visible to linker
[EXTERN main]       ; Expect main to be defined elsewhere (in C)

; =====================================
; Multiboot header - must be first!
; =====================================
section .multiboot
align 4
    dd MBOOT_HEADER_MAGIC        ; Magic number
    dd MBOOT_HEADER_FLAGS        ; Flags
    dd MBOOT_CHECKSUM           ; Checksum
    dd 0                        ; Header address (unused)
    dd 0                        ; Load address (unused)
    dd 0                        ; Load end address (unused)
    dd 0                        ; BSS end address (unused)
    dd 0                        ; Entry address (unused)
    dd 0                        ; Mode type (unused)
    dd 0                        ; Width (unused)
    dd 0                        ; Height (unused)
    dd 0                        ; Depth (unused)

; =====================================
; Text section - code goes here
; =====================================
section .text
start:
    ; Setup stack - stack grows downward
    mov esp, stack_top      ; Set stack pointer to top of stack
    
    ; Save multiboot info pointer (passed in ebx by GRUB)
    push ebx
    
    ; Clear all segment registers
    ; Data segments in protected mode should point to the data segment selector
    mov ax, 0x10           ; 0x10 is typically the data segment selector setup by GRUB
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Clear screen
    call clear_screen

    ; Print welcome message
    mov esi, WELCOME_MSG
    call print_string

    ; We can either halt here or jump to C code
    ; Uncomment the following line to jump to C main function:
    ; call main

    ; Halt the CPU
    cli                     ; Disable interrupts
.hang:
    hlt                     ; Halt CPU
    jmp .hang              ; If we get interrupted, halt again

; =====================================
; Function: clear_screen
; Clears the entire VGA text buffer
; =====================================
clear_screen:
    push eax
    push ecx
    push edi

    mov edi, VGA_BUFFER    ; Point to start of VGA buffer
    mov ecx, VGA_WIDTH * VGA_HEIGHT  ; Calculate screen size
    mov ax, 0x0F20         ; Space character (0x20) with white on black attribute
    rep stosw              ; Repeat store word

    pop edi
    pop ecx
    pop eax
    ret

; =====================================
; Function: print_string
; Input: ESI = pointer to null-terminated string
; Prints a string to the VGA buffer
; =====================================
print_string:
    push eax
    push ebx
    push edi

    mov edi, VGA_BUFFER    ; Start at beginning of VGA buffer
    mov ah, VGA_WHITE_ON_BLACK  ; Set color attribute

.loop:
    lodsb                  ; Load next character into AL
    test al, al           ; Check if we've hit null terminator
    jz .done              ; If so, we're done
    
    mov [edi], al         ; Store character
    mov [edi + 1], ah     ; Store attribute
    add edi, 2            ; Move to next character position
    jmp .loop

.done:
    pop edi
    pop ebx
    pop eax
    ret

; =====================================
; Data section - initialized data
; =====================================
section .data
WELCOME_MSG db 'DuckyOS booting in 32-bit protected mode...', 0

; =====================================
; BSS section - uninitialized data
; =====================================
section .bss
align 16
stack_bottom:
    resb 16384            ; Reserve 16KB for stack
stack_top:                ; Stack grows downward from here
