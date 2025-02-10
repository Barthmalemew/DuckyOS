#include <kernel/vga.h>
#include <system/io.h>
#include <system/type.h>
#include <system/string.h>

uint16_t* const vga_memory = (uint16_t*)VGA_MEMORY;
static uint16_t back_buffer[VGA_BUFFER_SIZE];
size_t terminal_row;    // Make these visible to other modules
size_t terminal_column;
uint8_t terminal_color;
static boolean update_needed = false;

uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void vga_init(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = VGA_LIGHT_GREY | (VGA_BLACK << 4);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            back_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    update_needed = true;
    vga_update();
}

void vga_setcolor(uint8_t color) {
    terminal_color = color;
}

static void scroll(void) {
    if (terminal_row >= VGA_HEIGHT) {
        // Move all lines up by one
        for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            back_buffer[i] = back_buffer[i + VGA_WIDTH];
        }
        // Clear the last line
        for (size_t i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            back_buffer[i] = vga_entry(' ', terminal_color);
        }
        terminal_row = VGA_HEIGHT - 1;
        update_needed = true;
    }
}

void vga_putchar(char c) {
    // Boundary check
    if (terminal_row >= VGA_HEIGHT || terminal_column >= VGA_WIDTH) {
        terminal_row = 0;
        terminal_column = 0;
    }

    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        scroll();
        return;
    }
    
    // Handle backspace
    if (c == '\b' && terminal_column > 0) {
        terminal_column--;
        return;
    }

    size_t index = terminal_row * VGA_WIDTH + terminal_column;
    back_buffer[index] = vga_entry(c, terminal_color);
    update_needed = true;
    
    if (++terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

void vga_write(const char* data, size_t size) {
    if (!data) return;
    for (size_t i = 0; i < size; i++) {
        vga_putchar(data[i]);
    }
}

void vga_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        vga_putchar(data[i]);
    }
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            back_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
    update_needed = true;
}

void vga_update(void) {
    static uint16_t prev_buffer[VGA_BUFFER_SIZE];
    
    if (!update_needed) return;
    
    for (size_t i = 0; i < VGA_BUFFER_SIZE; i++) {
        if (back_buffer[i] != prev_buffer[i]) {
            vga_memory[i] = back_buffer[i];
            prev_buffer[i] = back_buffer[i];
        }
    }
    update_needed = false;
    
    // Always update cursor position
    vga_set_cursor(terminal_column, terminal_row);
}

void vga_set_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 14);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}

void vga_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void vga_get_cursor(int* x, int* y) {
    if (x) *x = terminal_column;
    if (y) *y = terminal_row;
}
