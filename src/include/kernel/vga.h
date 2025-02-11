/**
 * @file vga.h
 * @brief VGA text mode interface definitions and declarations
 *
 * This header defines the public interface for the VGA text mode driver.
 * It provides:
 * - Color definitions for 16-color VGA text mode
 * - Function declarations for VGA manipulation
 * - Clear separation between interface and implementation
 *
 * Usage:
 * 1. Include this header in files needing VGA text output
 * 2. Call init_vga() before any other VGA functions
 * 3. Use provided functions to manipulate screen output
 *
 * Hardware Requirements:
 * - Standard VGA-compatible display adapter
 * - Text mode 80x25 support
 * - 16-color palette support
 */

#ifndef VGA_H
#define VGA_H

#include <system/stdint.h>  /* Required for fixed-width integer types */

/**
 * @brief VGA color definitions
 *
 * Standard 16-color VGA palette enumeration. These colors can be used for
 * both foreground (text) and background colors. The values correspond to
 * the standard VGA color attributes:
 * - Values 0-7: Standard colors
 * - Values 8-15: Bright/light versions of standard colors
 *
 * Color Properties:
 * - 4-bit values (0-15)
 * - Can be combined for fg/bg pairs
 * - Hardware-defined values (cannot be changed)
 *
 * Usage:
 * - Pass to vga_setcolor() as fg or bg parameter
 * - Used internally to create color attributes
 */
enum vga_color {
    VGA_BLACK = 0,          /* RGB: 0,0,0 */
    VGA_BLUE = 1,          /* RGB: 0,0,170 */
    VGA_GREEN = 2,         /* RGB: 0,170,0 */
    VGA_CYAN = 3,          /* RGB: 0,170,170 */
    VGA_RED = 4,           /* RGB: 170,0,0 */
    VGA_MAGENTA = 5,       /* RGB: 170,0,170 */
    VGA_BROWN = 6,         /* RGB: 170,85,0 */
    VGA_LIGHT_GREY = 7,    /* RGB: 170,170,170 */
    VGA_DARK_GREY = 8,     /* RGB: 85,85,85 */
    VGA_LIGHT_BLUE = 9,    /* RGB: 85,85,255 */
    VGA_LIGHT_GREEN = 10,  /* RGB: 85,255,85 */
    VGA_LIGHT_CYAN = 11,   /* RGB: 85,255,255 */
    VGA_LIGHT_RED = 12,    /* RGB: 255,85,85 */
    VGA_LIGHT_MAGENTA = 13,/* RGB: 255,85,255 */
    VGA_LIGHT_BROWN = 14,  /* RGB: 255,255,85 */
    VGA_WHITE = 15,        /* RGB: 255,255,255 */
};

/**
 * @brief Initialize the VGA text mode driver
 *
 * Must be called before any other VGA functions.
 * Initializes the display with:
 * - Clear screen
 * - Default colors (white on black)
 * - Cursor at position (0,0)
 */
extern void init_vga(void);

/**
 * @brief Write a single character to the screen
 *
 * @param c Character to display
 *
 * Writes character at current cursor position using current colors.
 * Special characters:
 * - '\n': New line
 * 
 * Handles:
 * - Cursor advancement
 * - Line wrapping
 * - Screen scrolling
 */
extern void vga_putchar(char c);

/**
 * @brief Write a string to the screen
 *
 * @param data Null-terminated string to display
 *
 * Writes entire string using current colors.
 * Processes special characters.
 * Continues until null terminator.
 */
extern void vga_write(const char* data);

/**
 * @brief Set text colors for subsequent output
 *
 * @param fg Foreground (text) color from vga_color enum
 * @param bg Background color from vga_color enum
 *
 * Changes colors for all subsequent text output.
 * Does not affect already displayed text.
 * Both colors must be valid vga_color values (0-15).
 */
extern void vga_setcolor(enum vga_color fg, enum vga_color bg);

/**
 * @brief Clear the entire screen
 *
 * Fills screen with spaces using current colors.
 * Resets cursor to position (0,0).
 * Useful for screen initialization or clearing.
 */
extern void vga_clear(void);

#endif /* VGA_H */
