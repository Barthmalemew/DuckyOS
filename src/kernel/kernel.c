#include <stdint.h>

// Video memory address
volatile uint16_t* video_memory = (uint16_t*)0xB8000;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;

// Current position in video memory
int cursor_x = 0;
int cursor_y = 0;

// Colors
#define COLOR_BLACK 0
#define COLOR_WHITE 15
#define MAKE_COLOR(fg, bg) ((bg << 4) | fg)

void clear_screen() {
    uint16_t blank = MAKE_COLOR(COLOR_WHITE, COLOR_BLACK) << 8 | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void putchar(char c) {
    uint16_t attrib = MAKE_COLOR(COLOR_WHITE, COLOR_BLACK);
    uint16_t *location;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        location = video_memory + (cursor_y * VGA_WIDTH + cursor_x);
        *location = c | (attrib << 8);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        // Scroll screen
        for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT-1); i++) {
            video_memory[i] = video_memory[i + VGA_WIDTH];
        }
        cursor_y--;
    }
}

void print(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

void kernel_main() {
    clear_screen();
    print("Initialized IDT...\n");
    idt_init();
    print("Kernel loaded successfully!\n");
    print("Welcome to our OS kernel\n");
    print("IDT initialized. Keyboard ready!\n");
    print("Press any key to see it working...\n");

    while(1) {
        // Halt CPU until next interrupt
        __asm__ volatile("hlt");
    }
}
