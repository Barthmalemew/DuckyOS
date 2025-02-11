/* Linker script for DuckOS kernel */
OUTPUT_FORMAT(elf32-i386)
ENTRY(start)           /* Entry point defined in boot.asm */

/* Define the program headers we want in our final ELF file */
PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2));  /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2));             /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2));  /* Write + Read */
}

SECTIONS
{
    /* Start loading at 1MB - standard location for kernels */
    . = 1M;

    /* First put the multiboot header, as it needs to be within the first 8KB */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)   /* Multiboot header first */
        *(.text)        /* Then all code */
    } :text

    /* Read-only data */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    } :rodata

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    } :data

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    } :data

    /* Get rid of unnecessary GCC bits */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
