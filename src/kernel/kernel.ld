ENTRY(kernel_main)
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
    . = 0x100000;      /* Load kernel at 1MB */
    
    .text : {
        *(.text)       /* All text sections */
    }
    
    .rodata : {
        *(.rodata*)    /* All read-only data sections */
    }
    
    .data : {
        *(.data)       /* All data sections */
    }
    
    .bss : {
        *(COMMON)      /* All uninitialized data */
        *(.bss)
    }
}
