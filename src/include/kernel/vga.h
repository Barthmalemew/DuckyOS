#ifndef VGA_H
#define VGA_H

#include <system/type.h>

// VGA dimensions and memory location
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000
#define VGA_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT)

// Terminal color variable
extern uint8_t terminal_color;

// Cursor position tracking
extern size_t terminal_row;
extern size_t terminal_column;

// VGA entry function declaration
uint16_t vga_entry(unsigned char uc, uint8_t color);

// VGA color constants
enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15,
};

// VGA functions
void vga_init(void);
void vga_putchar(char c);
void vga_write(const char* data, size_t size);
void vga_writestring(const char* data);
void vga_setcolor(uint8_t color);
void vga_clear(void);
void vga_update(void);  // New function to update screen

// Cursor control functions
void vga_set_cursor(int x, int y);
void vga_enable_cursor(void);
void vga_disable_cursor(void);
void vga_get_cursor(int* x, int* y);

#endif
