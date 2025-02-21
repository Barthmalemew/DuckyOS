; =============================================================================
; 16-BIT FAT12 BOOTLOADER FOR A 1.44MB FLOPPY DISK
;
; This bootloader:
;   1. Sets up the CPU segments and stack.
;   2. Reads the disk geometry from the BIOS (INT 13h, AH=08h).
;   3. Locates the FAT12 root directory and finds "KERNEL.BIN".
;   4. Reads the kernel file from the FAT12 filesystem by following the FAT chain.
;   5. Jumps to the loaded kernel (at segment 0x2000).
;   6. On error, prints a message and waits for a keypress, then reboots.
;
; Assemble with: nasm -f bin boot.asm -o boot.bin
; Then write boot.bin to your floppy image or disk.
; =============================================================================

org 0x7C00             ; Boot sector is loaded by BIOS at 0000:7C00 (or 07C0:0000).
bits 16                ; 16-bit real mode code.

%define ENDL 0x0D, 0x0A  ; Macro for DOS/BIOS newline: CR LF

; =============================================================================
; FAT12 HEADER (BIOS PARAMETER BLOCK + EXTENDED BOOT RECORD)
; =============================================================================

; Jump to our boot code immediately (required by the FAT spec).
jmp short start
nop

; --- Standard 11-byte region + Basic BPB fields ---
bdb_oem:                    db 'MSWIN4.1'           ; OEM identifier (8 bytes)
bdb_bytes_per_sector:       dw 512                  ; Bytes per sector
bdb_sectors_per_cluster:    db 1                    ; Sectors per cluster
bdb_reserved_sectors:       dw 1                    ; Reserved sectors (boot sector)
bdb_fat_count:              db 2                    ; Number of FAT copies
bdb_dir_entries_count:      dw 0E0h                 ; 224 root dir entries
bdb_total_sectors:          dw 2880                 ; Total sectors on a 1.44MB floppy
bdb_media_descriptor_type:  db 0F0h                 ; 0xF0 -> 3.5" floppy
bdb_sectors_per_fat:        dw 9                    ; 9 sectors per FAT
bdb_sectors_per_track:      dw 18                   ; 18 sectors per track
bdb_heads:                  dw 2                    ; 2 heads
bdb_hidden_sectors:         dd 0                    ; Hidden sectors = 0
bdb_large_sector_count:     dd 0                    ; Large sector count = 0 (unused on floppy)

; --- Extended Boot Record (EBPB) ---
ebr_drive_number:           db 0                    ; 0x00 for floppy; 0x80 for HDD
                            db 0                    ; Reserved
ebr_signature:              db 0x29                 ; Indicates an extended BPB
ebr_volume_id:              db 0x12, 0x34, 0x56, 0x78 ; Volume serial number
ebr_volume_label:           db 'DuckyOS    '        ; 11-byte volume label (pad with spaces)
ebr_system_id:              db 'FAT12   '           ; 8-byte system ID string

; =============================================================================
; START OF BOOT CODE
; =============================================================================

start:
    ; -------------------------------------------------------------------------
    ; 1) Set up segment registers and the stack.
    ; -------------------------------------------------------------------------
    mov ax, 0            ; We'll use segment 0 for DS and ES for now
    mov ds, ax
    mov es, ax
    mov ss, ax           ; Set stack segment
    mov sp, 0x7C00       ; Place stack at 0x7C00 (downwards)

    ; Some BIOSes set the boot segment to 07C0:0000 instead of 0000:7C00.
    ; Force CS:IP = 0000:7C00 so our offsets match.
    push es              ; ES = 0 from above
    push word .after_reloc
    retf

.after_reloc:

    ; -------------------------------------------------------------------------
    ; 2) Save drive number (DL) and display a "Loading..." message.
    ; -------------------------------------------------------------------------
    mov [ebr_drive_number], dl  ; Store BIOS drive number to EBPB field
    mov si, msg_loading
    call puts                   ; Print "Loading..."

    ; -------------------------------------------------------------------------
    ; 3) Read drive geometry from BIOS (INT 13h, AH=08h).
    ;    - We only do this to get heads/sectors dynamically if needed.
    ; -------------------------------------------------------------------------
    push es
    mov ah, 0x08               ; Get drive parameters
    int 0x13
    jc floppy_error            ; Jump if CF is set (error)
    pop es

    and cl, 0x3F               ; Sector count is in CL (lower 6 bits)
    xor ch, ch
    mov [bdb_sectors_per_track], cx  ; Store #sectors/track
    inc dh
    mov [bdb_heads], dh              ; Store #heads (1-based)

    ; -------------------------------------------------------------------------
    ; 4) Compute LBA of the root directory
    ;    LBA(root dir) = reserved_sectors + (fat_count * sectors_per_fat)
    ; -------------------------------------------------------------------------
    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx                     ; AX = sectors_per_fat * fat_count
    add ax, [bdb_reserved_sectors] ; AX = root directory start LBA
    push ax                    ; Keep this on stack for later

    ; -------------------------------------------------------------------------
    ; 5) Compute # of sectors in the root directory:
    ;    root_dir_size_sectors = (32 * dir_entries) / bytes_per_sector (round up)
    ; -------------------------------------------------------------------------
    mov ax, [bdb_dir_entries_count]
    shl ax, 5         ; Multiply by 32 (size of one directory entry)
    xor dx, dx
    div word [bdb_bytes_per_sector]
    test dx, dx
    jz .root_dir_after
    inc ax            ; If remainder != 0, we need one more sector
.root_dir_after:

    ; -------------------------------------------------------------------------
    ; 6) Read root directory into memory (in 'buffer').
    ; -------------------------------------------------------------------------
    mov cl, al        ; CL = # of sectors to read
    pop ax            ; AX = root directory start LBA
    mov dl, [ebr_drive_number] ; DL = drive
    mov bx, buffer    ; ES:BX = destination
    call disk_read

    ; -------------------------------------------------------------------------
    ; 7) Search for "KERNEL.BIN" in the root directory.
    ; -------------------------------------------------------------------------
    xor bx, bx
    mov di, buffer

.search_kernel:
    mov si, file_kernel_bin
    mov cx, 11              ; Compare up to 11 chars (DOS 8.3 filename)
    push di
    repe cmpsb              ; Compare string in [DI..] with [SI..]
    pop di
    je .found_kernel        ; If match, jump

    add di, 32              ; Next directory entry (32 bytes each)
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_kernel

    ; If we get here, we didn't find KERNEL.BIN
    jmp kernel_not_found_error

.found_kernel:
    ; DI points to the start of directory entry
    ; Offset 26 in a directory entry is the first cluster (WORD).
    mov ax, [di + 26]
    mov [kernel_cluster], ax

    ; -------------------------------------------------------------------------
    ; 8) Load the entire FAT into 'buffer' so we can parse the cluster chain.
    ; -------------------------------------------------------------------------
    mov ax, [bdb_reserved_sectors]  ; LBA of the first FAT
    mov bx, buffer
    mov cl, [bdb_sectors_per_fat]   ; # of sectors to read
    mov dl, [ebr_drive_number]
    call disk_read

    ; -------------------------------------------------------------------------
    ; 9) Read the kernel, following the FAT cluster chain.
    ;    We'll load the kernel to 0x2000:0x0000 in memory.
    ; -------------------------------------------------------------------------
    mov bx, KERNEL_LOAD_SEGMENT
    mov es, bx
    mov bx, KERNEL_LOAD_OFFSET

.load_kernel_loop:
    mov ax, [kernel_cluster]

    ; Hardcode LBA offset for cluster N:
    ;   LBA =  (N-2)*sectors_per_cluster + (reserved + fats + rootdir)
    ; Since sectors_per_cluster=1 for floppy, we do (N + constant).
    ; For a standard 1.44M layout, the data area starts at sector 33,
    ; so for cluster 2 => LBA=33. => offset = +31 from cluster number.
    add ax, 31
    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    ; Advance BX by 512 (bytes per sector) to read next sector in memory.
    add bx, [bdb_bytes_per_sector]

    ; -------------------------------------------------------------------------
    ; 10) Use the FAT (already in 'buffer') to find the next cluster.
    ;     FAT12 has 12-bit entries, so each cluster can be at an even/odd nibble.
    ; -------------------------------------------------------------------------
    mov ax, [kernel_cluster]
    mov cx, 3
    mul cx            ; AX = cluster * 3
    mov cx, 2
    div cx            ; AX = offset in bytes, DX = cluster mod 2 ?

    mov si, buffer
    add si, ax
    mov ax, [ds:si]   ; Read 2 bytes from the FAT

    or dx, dx
    jz .even
.odd:
    shr ax, 4         ; Odd cluster -> shift right 4 bits
    jmp .next_cluster_after

.even:
    and ax, 0x0FFF    ; Even cluster -> lower 12 bits

.next_cluster_after:
    cmp ax, 0x0FF8    ; 0xFF8..0xFFF => end of chain
    jae .read_finish

    mov [kernel_cluster], ax
    jmp .load_kernel_loop

.read_finish:
    ; -------------------------------------------------------------------------
    ; 11) Jump to the loaded kernel at 0x2000:0x0000.
    ; -------------------------------------------------------------------------
    mov dl, [ebr_drive_number]  ; Keep drive # in DL for the kernel's usage

    mov ax, KERNEL_LOAD_SEGMENT
    mov ds, ax
    mov es, ax

    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

    ; If for some reason we return here, just hang or reboot
    jmp wait_key_and_reboot

    cli
    hlt

; =============================================================================
; Error Handlers
; =============================================================================

floppy_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

kernel_not_found_error:
    mov si, msg_kernel_not_found
    call puts
    jmp wait_key_and_reboot

; Wait for key press and then jump to the BIOS reboot vector FFFF:0000
wait_key_and_reboot:
    mov ah, 0
    int 16h             ; Wait for keystroke
    jmp 0FFFFh:0        ; Jump to BIOS, causing reboot

; =============================================================================
; Display String Routine (BIOS Teletype)
; DS:SI -> Null-terminated string
; =============================================================================
puts:
    push si
    push ax
    push bx

.puts_loop:
    lodsb               ; Load next char into AL from DS:SI
    or al, al
    jz .puts_done       ; If AL=0, end of string

    mov ah, 0x0E        ; BIOS teletype function
    mov bh, 0           ; Page number
    int 0x10            ; Print AL

    jmp .puts_loop

.puts_done:
    pop bx
    pop ax
    pop si
    ret

; =============================================================================
; Disk I/O Routines
; =============================================================================

; -------------------------------------------------------------------------
; Convert LBA -> CHS for a floppy (or similarly structured disk).
; Input:
;   AX = LBA
;   [bdb_sectors_per_track], [bdb_heads] used
; Output:
;   CH, CL, DH = geometry-based cylinder/head/sector
;   (DL is restored after call)
; -------------------------------------------------------------------------
lba_to_chs:
    push ax
    push dx

    xor dx, dx
    div word [bdb_sectors_per_track]   ; AX = LBA / SPT, DX = LBA % SPT
    inc dx                             ; Sector is 1-based
    mov cx, dx                         ; Save sector in CX (lower 6 bits)
    
    xor dx, dx
    div word [bdb_heads]              ; AX = cylinder, DX = head
    mov dh, dl                         ; DH = head
    mov ch, al                         ; CH = cylinder (low 8 bits)
    shl ah, 6
    or cl, ah                          ; Put top 2 bits of cylinder into CL

    pop ax
    mov dl, al                         ; Restore DL
    pop ax
    ret

; -------------------------------------------------------------------------
; Reads CL sectors from LBA=AX into ES:BX from drive DL.
; Uses 3 retries on error.
; -------------------------------------------------------------------------
disk_read:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx               ; Save CL (# of sectors)
    call lba_to_chs       ; Convert LBA -> CHS
    pop ax                ; AX now has # of sectors to read in AL

    mov ah, 0x02          ; BIOS: Read sectors (INT 13h)
    mov di, 3             ; Retry count

.retry:
    pusha
    stc                   ; Some BIOSes need CF set
    int 0x13
    jnc .done             ; If no carry, read succeeded

    popa
    call disk_reset       ; Attempt to reset
    dec di
    test di, di
    jnz .retry

.fail:
    jmp floppy_error      ; All retries failed

.done:
    popa

    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; -------------------------------------------------------------------------
; Reset disk drive controller (INT 13h, AH=0).
; -------------------------------------------------------------------------
disk_reset:
    pusha
    mov ah, 0
    stc
    int 0x13
    jc floppy_error
    popa
    ret

; =============================================================================
; Embedded Messages and Variables
; =============================================================================

msg_loading:            db 'Loading...', ENDL, 0
msg_read_failed:        db 'Read from disk failed!', ENDL, 0
msg_kernel_not_found:   db 'KERNEL.BIN file not found!', ENDL, 0

; DOS 8.3 filename (11 bytes: 8 for name + 3 for extension)
; "KERNEL  BIN" has two spaces to align the extension in an 8.3 name.
file_kernel_bin:        db 'KERNEL  BIN'

; Will store the first cluster of KERNEL.BIN
kernel_cluster:         dw 0

; Define where to load the kernel (physical = 0x2000 * 16 = 0x20000)
KERNEL_LOAD_SEGMENT     equ 0x2000
KERNEL_LOAD_OFFSET      equ 0

; Reserve space at the end of the 512-byte sector
; up to byte 510, then put the 0xAA55 signature (2 bytes).
times 510-($-$$) db 0
dw 0xAA55
; =============================================================================
; END OF BOOT SECTOR (512 BYTES)
; =============================================================================
