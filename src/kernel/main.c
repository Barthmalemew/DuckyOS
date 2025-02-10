#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/vga.h>
#include <system/io.h>
#include <system/isr.h>

// Put a character at the current cursor position
void putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (c == '\b' && terminal_column > 0) {
        --terminal_column;
        vga_putchar(' ');
    } else {
        vga_putchar(c);
        if (++terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }
}

void print(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

// Kernel entry point
void kernel_main(void) {
    isr_init();
    idt_init();
    vga_init();
    vga_enable_cursor();
    keyboard_init();
    
    // Initialize VGA and clear screen

    // Print welcome message
    print("DuckyOS Keyboard Test\n");
    print("Type something: ");

    // Main loop
    while (1) {
        if (keyboard_available()) {
            char c = keyboard_getchar();
            if (c != 0) {
                putchar(c);
                if (c == '\n') {
                    print("Type something: ");
                }
                vga_update();
            }
        } else {
            // Wait for next interrupt
            __asm__ volatile("hlt");
        }
    }
}
