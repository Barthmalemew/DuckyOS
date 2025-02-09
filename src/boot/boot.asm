bits 16
org 0x7C00

; Constants
KERNEL_OFFSET equ 0x1000    ; Where we'll load the kernel
KERNEL_SECTORS equ 20       ; How many sectors to load (adjust as needed)

; Boot sector entry point
boot:
    ; Initialize segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00    ; Set up stack

    ; Save boot drive number
    mov [boot_drive], dl

    ; Print loading message
    mov si, loading_msg
    call print_string

    ; Load kernel from disk
    call load_kernel

    ; Print ready message
    mov si, ready_msg
    call print_string

    ; Prepare for protected mode
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load GDT
    
    ; Enable A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Switch to protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to flush pipeline and load CS
    jmp 0x08:protected_mode

; Load kernel from disk
load_kernel:
    pusha
    push es

    ; Set up segments for loading
    mov ax, KERNEL_OFFSET >> 4  ; Convert offset to segment
    mov es, ax
    xor bx, bx                  ; ES:BX = where to load kernel

    ; Set up disk read
    mov ah, 0x02        ; BIOS read sector function
    mov al, KERNEL_SECTORS ; Number of sectors to read
    mov ch, 0          ; Cylinder 0
    mov cl, 2          ; Start from sector 2 (sector 1 is boot sector)
    mov dh, 0          ; Head 0
    mov dl, [boot_drive] ; Drive number

    ; Perform read
    int 0x13
    jc disk_error      ; If carry flag set, there was an error

    pop es
    popa
    ret

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

bits 32
protected_mode:
    ; Set up segment registers for protected mode
    mov ax, 0x10      ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack for protected mode
    mov esp, 0x90000

    ; Print protected mode message
    mov esi, protected_mode_msg
    mov edi, 0xB8000
    mov ah, 0x0F
.print_pm:
    lodsb
    or al, al
    jz .done_pm
    mov [edi], ax
    add edi, 2
    jmp .print_pm
.done_pm:

    ; Jump to kernel
    jmp KERNEL_OFFSET

; Global Descriptor Table
gdt_start:
    ; Null descriptor
    dq 0

    ; Code segment descriptor
    dw 0xFFFF    ; Limit (bits 0-15)
    dw 0x0000    ; Base (bits 0-15)
    db 0x00      ; Base (bits 16-23)
    db 10011010b ; Access byte
    db 11001111b ; Flags (4 bits) + Limit (bits 16-19)
    db 0x00      ; Base (bits 24-31)

    ; Data segment descriptor
    dw 0xFFFF    ; Limit (bits 0-15)
    dw 0x0000    ; Base (bits 0-15)
    db 0x00      ; Base (bits 16-23)
    db 10010010b ; Access byte
    db 11001111b ; Flags (4 bits) + Limit (bits 16-19)
    db 0x00      ; Base (bits 24-31)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size
    dd gdt_start                 ; GDT address

; Data
boot_drive: db 0
loading_msg: db 'Loading kernel from disk...', 13, 10, 0
ready_msg: db 'Ready to switch to protected mode...', 13, 10, 0
disk_error_msg: db 'Error loading kernel from disk!', 13, 10, 0
protected_mode_msg: db 'In protected mode, jumping to kernel...', 0

; Padding and boot signature
times 510-($-$$) db 0
dw 0xAA55
