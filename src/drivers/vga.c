/**
 * @file vga.c
 * @brief VGA text mode driver implementation
 *
 * This file implements a basic VGA text mode driver for x86 systems.
 * It provides functions for writing text to the screen, controlling colors,
 * and managing the display buffer. The implementation uses direct memory
 * access to the VGA hardware buffer at 0xB8000.
 *
 * Hardware Details:
 * - Standard VGA text mode uses a 80x25 character display
 * - Each character cell is 16 bits (2 bytes):
 *   - Lower byte: ASCII character
 *   - Upper byte: Color attributes (4 bits bg, 4 bits fg)
 * - Buffer located at physical address 0xB8000
 */

#include "../include/kernel/vga.h"

/* 
 * VGA Hardware Constants
 * These define the fundamental parameters of VGA text mode
 */
#define VGA_BUFFER 0xB8000    /* Physical memory address of VGA buffer */
#define VGA_WIDTH 80          /* Screen width in characters */
#define VGA_HEIGHT 25         /* Screen height in characters */

/* 
 * Global State Variables
 * These track the current state of the VGA display
 */
static uint16_t* const vga_mem = (uint16_t*)VGA_BUFFER;  /* Direct pointer to VGA memory */
static uint8_t vga_x = 0;     /* Current cursor X position (0 to 79) */
static uint8_t vga_y = 0;     /* Current cursor Y position (0 to 24) */
static uint8_t vga_color = 0; /* Current color attributes */

/**
 * @brief Creates a VGA character entry combining char and color
 * 
 * @param c Character to display
 * @param color Color attribute byte
 * @return uint16_t Combined VGA entry ready for display memory
 * 
 * Creates a 16-bit VGA character entry where:
 * - Lower 8 bits contain the character
 * - Upper 8 bits contain the color attributes
 * This matches the VGA hardware's memory layout expectations
 */
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * @brief Combines foreground and background colors into a color byte
 * 
 * @param fg Foreground color (0-15)
 * @param bg Background color (0-15)
 * @return uint8_t Combined color attribute
 * 
 * Creates an 8-bit color attribute where:
 * - Lower 4 bits are the foreground color
 * - Upper 4 bits are the background color
 * Colors are defined in the vga.h header as enum vga_color
 */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

/**
 * @brief Handles screen scrolling when bottom is reached
 * 
 * This function is called when text reaches the bottom of the screen.
 * It performs the following operations:
 * 1. Moves all lines up one position
 * 2. Clears the bottom line
 * 3. Adjusts the cursor position
 * 
 * The function uses direct memory operations for efficiency,
 * copying entire character cells (char + color) at once.
 */
static void vga_scroll(void) {
    if (vga_y >= VGA_HEIGHT) {
        /* 
         * Move all lines up one position
         * Uses direct memory copy for efficiency
         */
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_mem[y * VGA_WIDTH + x] = vga_mem[(y + 1) * VGA_WIDTH + x];
            }
        }

        /* 
         * Clear the bottom line with spaces
         * Uses current color attributes
         */
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_mem[(VGA_HEIGHT-1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
        }
        vga_y = VGA_HEIGHT - 1;  /* Reset cursor to bottom line */
    }
}

/**
 * @brief Initializes the VGA text mode driver
 * 
 * This function must be called before any other VGA functions.
 * It performs the following initialization:
 * 1. Sets default colors (white on black)
 * 2. Clears the entire screen
 * 3. Resets cursor position to top-left
 */
void init_vga(void) {
    /* Set default color to white text on black background */
    vga_color = vga_entry_color(VGA_WHITE, VGA_BLACK);
    /* Clear screen with default colors */
    vga_clear();
}

/**
 * @brief Clears the entire screen
 * 
 * Fills the entire VGA buffer with spaces using the current color.
 * Also resets the cursor position to the top-left corner (0,0).
 */
void vga_clear(void) {
    /* Fill entire buffer with spaces using current color */
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_mem[y * VGA_WIDTH + x] = vga_entry(' ', vga_color);
        }
    }
    /* Reset cursor position */
    vga_x = 0;
    vga_y = 0;
}

/**
 * @brief Writes a single character to the screen
 * 
 * @param c Character to write
 * 
 * Handles special characters:
 * - '\n': New line (moves cursor to start of next line)
 * 
 * For normal characters:
 * 1. Writes character with current color to current position
 * 2. Advances cursor
 * 3. Handles line wrapping
 * 4. Triggers scrolling if needed
 */
void vga_putchar(char c) {
    if (c == '\n') {
        /* Handle newline character */
        vga_x = 0;           /* Return to start of line */
        vga_y++;            /* Move to next line */
        vga_scroll();       /* Check if scrolling needed */
        return;
    }

    /* Write character to current position */
    vga_mem[vga_y * VGA_WIDTH + vga_x] = vga_entry(c, vga_color);
    
    /* Handle cursor advancement and wrapping */
    if (++vga_x >= VGA_WIDTH) {
        vga_x = 0;          /* Return to start of line */
        vga_y++;           /* Move to next line */
        vga_scroll();      /* Check if scrolling needed */
    }
}

/**
 * @brief Writes a null-terminated string to the screen
 * 
 * @param data Pointer to null-terminated string
 * 
 * Iterates through the string, writing each character
 * using vga_putchar(). Continues until null terminator
 * is reached.
 */
void vga_write(const char* data) {
    for (; *data != '\0'; data++) {
        vga_putchar(*data);
    }
}

/**
 * @brief Sets the current text color
 * 
 * @param fg Foreground color for text
 * @param bg Background color for text
 * 
 * Updates the current color attribute used for all subsequent
 * text output. Colors are defined in vga.h as enum vga_color.
 * Does not affect already displayed text.
 */
void vga_setcolor(enum vga_color fg, enum vga_color bg) {
    vga_color = vga_entry_color(fg, bg);
}
