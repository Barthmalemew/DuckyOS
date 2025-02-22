; *****************************************************************************
; FILE: x86_lowlevel.asm
; DESCRIPTION:
;   Contains two low-level routines for 16-bit x86 environments (small model):
;     1) _x86_div64_32 - 64-bit / 32-bit division.
;     2) _x86_Video_WriteCharTeletype - Teletype-based character output via INT 10h.
;
; ASSEMBLY MODE:
;   - bits 16: indicates 16-bit code. However, this code uses 32-bit registers
;     (EAX, ECX, EDX, etc.), which is permissible on 386+ CPUs even in 16-bit
;     real mode (with size prefixes) or in some 16-bit protected-mode setups.
;   - section _TEXT class=CODE: the code is placed in a code segment named _TEXT
;     with class=CODE, which is typical in some 16-bit toolchains.
;   - The calling convention resembles small-model C on 16-bit compilers, where
;     the callee sets up a stack frame with `push bp` / `mov bp, sp` and 
;     function parameters appear at [bp + 4], [bp + 6], etc.
;
; USAGE:
;   - _x86_div64_32:
;       void _cdecl x86_div64_32(uint64_t dividend,
;                                uint32_t divisor,
;                                uint64_t* quotientOut,
;                                uint32_t* remainderOut);
;
;   - _x86_Video_WriteCharTeletype:
;       void _cdecl x86_Video_WriteCharTeletype(char character, int page);
;     Uses INT 10h AH=0Eh to write 'character' to the screen at 'page'.
;
; NOTE:
;   Each function preserves BP-based local stack frames as is typical for C 
;   function calls in 16-bit environments. Code must be linked accordingly.
;
; *****************************************************************************

bits 16                      ; Specify 16-bit mode

section _TEXT class=CODE     ; Define a code section named "_TEXT" with class=CODE

; -----------------------------------------------------------------------------
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor,
;                          uint64_t* quotientOut, uint32_t* remainderOut);
;
;   dividend: 64-bit value (lower 32 bits first, then upper 32 bits)
;   divisor : 32-bit value
;   quotientOut: pointer to 64-bit storage for quotient
;   remainderOut: pointer to 32-bit storage for remainder
;
; This routine:
;   1) Divides the upper 32 bits of dividend by divisor -> partial quotient
;      and remainder.
;   2) Stores that partial quotient in the high 32 bits of the output quotient.
;   3) Divides the lower 32 bits of the original dividend (plus the remainder
;      from step 1 in EDX) by divisor.
;   4) Stores that result into the low 32 bits of the output quotient and
;      stores the final remainder in remainderOut.
;
; Stack frame layout (small model, near call):
;   [BP + 0]   = old BP
;   [BP + 2]   = return IP
;   [BP + 4]   = lower 32 bits of dividend  (as one 32-bit value)
;   [BP + 8]   = upper 32 bits of dividend  (as one 32-bit value)
;   [BP + 12]  = divisor                    (32-bit)
;   [BP + 16]  = quotientOut pointer        (16-bit pointer in small model)
;   [BP + 18]  = remainderOut pointer       (16-bit pointer in small model)
; -----------------------------------------------------------------------------
global _x86_div64_32
_x86_div64_32:

    ; Prologue: set up stack frame
    push bp             ; save old BP
    mov bp, sp          ; initialize new BP (start of new frame)

    push bx             ; save BX (we'll use it temporarily)

    ; -------------------------------------------------------------------------
    ; 1) Divide the upper 32 bits of the dividend by the divisor
    ; -------------------------------------------------------------------------
    mov eax, [bp + 8]   ; load EAX with the 32-bit 'upper' portion of dividend
    mov ecx, [bp + 12]  ; load ECX with 'divisor'
    xor edx, edx        ; clear EDX (upper part = 0 for this division)
    div ecx             ; 64-bit division: EDX:EAX / ECX
                        ; result: EAX = quotient, EDX = remainder

    ; -------------------------------------------------------------------------
    ; 2) Store the 32-bit partial quotient in the HIGH 32 bits of *quotientOut
    ; -------------------------------------------------------------------------
    mov bx, [bp + 16]   ; load BX with quotientOut pointer (small model => 16 bits)
    mov [bx + 4], eax   ; store the partial quotient into [quotientOut + 4]

    ; -------------------------------------------------------------------------
    ; 3) Divide the LOWER 32 bits of the dividend by the divisor
    ;    EAX = lower 32 bits, EDX = remainder from previous step
    ; -------------------------------------------------------------------------
    mov eax, [bp + 4]   ; EAX <- lower 32 bits of dividend
                        ; EDX is automatically the remainder from above (in EDX)
    div ecx             ; EDX:EAX / ECX => EAX = new quotient, EDX = new remainder

    ; -------------------------------------------------------------------------
    ; 4) Store the final results:
    ;    - EAX -> low 32 bits of *quotientOut
    ;    - EDX -> *remainderOut
    ; -------------------------------------------------------------------------
    mov [bx], eax       ; store the low 32-bit quotient into [quotientOut + 0]

    mov bx, [bp + 18]   ; load BX with remainderOut pointer
    mov [bx], edx       ; store remainder into remainderOut

    ; -------------------------------------------------------------------------
    ; Epilogue: restore registers and return
    ; -------------------------------------------------------------------------
    pop bx              ; restore BX
    mov sp, bp          ; restore stack pointer
    pop bp              ; restore old BP
    ret                 ; return to caller

; -----------------------------------------------------------------------------
; void _cdecl x86_Video_WriteCharTeletype(char character, int page);
;
;  Uses the BIOS INT 10h AH=0Eh function to write a single character at the
;  current cursor position, then advance the cursor (teletype mode).
;
;  Stack frame layout (small model, near call):
;   [BP + 0] = old BP
;   [BP + 2] = return IP
;   [BP + 4] = character (only 8 bits used, but stored as a 16-bit parameter)
;   [BP + 6] = page (only 8 bits used, but stored as a 16-bit parameter)
; -----------------------------------------------------------------------------
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    
    ; Prologue: set up stack frame
    push bp             ; save old BP
    mov bp, sp          ; initialize new BP

    ; Save BX (we’ll temporarily overwrite BH)
    push bx

    ; In a typical 16-bit small-model C function:
    ;   [BP + 4] is the first argument (char character)
    ;   [BP + 6] is the second argument (int page)
    ;
    ; We load them into registers AH and AL (and BH, if needed).

    mov ah, 0Eh         ; BIOS function 0Eh => teletype output
    mov al, [bp + 4]    ; AL = character
    mov bh, [bp + 6]    ; BH = page number (usually 0 if not using multiple pages)

    int 10h             ; Call BIOS interrupt for video teletype output

    ; Epilogue: restore registers and return
    pop bx
    mov sp, bp
    pop bp
    ret

