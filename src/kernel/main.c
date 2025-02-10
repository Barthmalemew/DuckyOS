#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/vga.h>
#include <system/io.h>

#define VGA_BUFFER 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* const vga_buffer = (uint16_t*)VGA_BUFFER;
static int cursor_x = 0;
static int cursor_y = 0;

// Update hardware cursor position
static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 14);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}

// Scroll the screen if needed
static void scroll(void) {
    if (cursor_y >= VGA_HEIGHT) {
        // Move everything up one line
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        // Clear bottom line
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        }
        cursor_y--;
    }
}

// Put a character at the current cursor position
void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | ' ';
        }
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | c;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    scroll();
    update_cursor();
}

// Print a string
void print(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

// Kernel entry point
void kernel_main(void) {
    // Initialize hardware
    idt_init();
    keyboard_init();
    
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (0x0F << 8) | ' ';
    }

    // Print welcome message
    print("DuckyOS Keyboard Test\n");
    print("Type something: ");

    // Main loop
    while (1) {
        __asm__ volatile("hlt");
    }
}
