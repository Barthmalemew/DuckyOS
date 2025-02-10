#include <kernel/keyboard.h>
#include <system/type.h>
#include <kernel/vga.h>
#include <system/io.h>
#include <system/isr.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256

// Keyboard buffer
static char buffer[KEYBOARD_BUFFER_SIZE];
static volatile size_t buffer_start = 0;
static volatile size_t buffer_end = 0;
static volatile size_t buffer_size = 0;
static boolean echo_enabled = true;

// US keyboard layout scancode table
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (scancode < 0x80 && scancode < sizeof(scancode_to_ascii)) {
        char ascii = scancode_to_ascii[scancode];
        if (ascii != 0 && buffer_size < KEYBOARD_BUFFER_SIZE) {
            buffer[buffer_end] = ascii;
            buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
            buffer_size++;
            
            if (echo_enabled) {
                vga_putchar(ascii);
            }
        }
    }
}

void keyboard_init(void) {
    // Register keyboard handler
    isr_register_handler(IRQ_KEYBOARD, keyboard_handler);
    
    // Enable keyboard interrupt
    outb(0x21, inb(0x21) & ~(1 << IRQ_KEYBOARD));
}

char keyboard_getchar(void) {
    char c = 0;
    while (!keyboard_available()) {
        __asm__ volatile("hlt");
    }
    
    isr_disable();
    c = buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    buffer_size--;
    isr_enable();
    
    return c;
}

boolean keyboard_available(void) {
    return buffer_size > 0;
}

int keyboard_readline(char* buffer, size_t max_length) {
    size_t count = 0;
    while (count < max_length - 1) {
        char c = keyboard_getchar();
        if (c == '\n') {
            buffer[count] = '\0';
            return count;
        } else if (c == '\b' && count > 0) {
            count--;
        } else {
            buffer[count++] = c;
        }
    }
    buffer[count] = '\0';
    return count;
}

void keyboard_set_echo(boolean enable) {
    echo_enabled = enable;
}
