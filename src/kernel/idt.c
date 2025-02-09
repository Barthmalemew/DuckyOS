#include "../include/idt.h"
#include "../include/io.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_init(void) {
    // Set up PIC
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Clear IDT
    for(int i = 0; i < 256; i++) {
        idt[i].base_low = 0;
        idt[i].segment = 0;
        idt[i].reserved = 0;
        idt[i].flags = 0;
        idt[i].base_high = 0;
    }

    // Add keyboard handler
    uint32_t keyboard_address = (uint32_t)keyboard_handler_int;
    idt[33].base_low = keyboard_address & 0xFFFF;
    idt[33].segment = 0x08; // kernel code segment
    idt[33].reserved = 0;
    idt[33].flags = 0x8E; // interrupt gate
    idt[33].base_high = (keyboard_address >> 16) & 0xFFFF;

    // Load IDT
    load_idt((uint32_t)&idtp);
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20); // send EOI

    if (scancode < 0x80) {
        putchar('K'); // print K for each keypress
    }
}
