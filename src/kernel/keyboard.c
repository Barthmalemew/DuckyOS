#include <kernel/keyboard.h>
#include <system/type.h>
#include <system/io.h>
#include <system/isr.h>

// Scancode to ASCII mapping table
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Keyboard state
static struct {
    boolean shift_pressed;
    boolean ctrl_pressed;
    boolean alt_pressed;
    boolean caps_lock;
    boolean num_lock;
    boolean scroll_lock;
} keyboard_state = {0};

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256
#define KEYBOARD_BUFFER_THRESHOLD (KEYBOARD_BUFFER_SIZE - 16)

// Keyboard buffer
static char buffer[KEYBOARD_BUFFER_SIZE];
static volatile size_t buffer_start = 0;
static volatile size_t buffer_end = 0;
static volatile size_t buffer_size = 0;
static boolean echo_enabled = true;
static volatile uint32_t last_interrupt_time = 0;
static volatile boolean interrupt_in_progress = false;

void keyboard_handler(void) {
    interrupt_in_progress = true;
    static uint32_t debounce_time = 0;
    debounce_time++;
    
    // Basic debouncing
    if (debounce_time - last_interrupt_time < 5) {
        outb(0x20, 0x20);  // End Of Interrupt
        return;
    }
    last_interrupt_time = debounce_time;
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Handle modifier keys
    switch(scancode) {
        case 0x2A: keyboard_state.shift_pressed = true; return;
        case 0xAA: keyboard_state.shift_pressed = false; return;
        case 0x3A: keyboard_state.caps_lock = !keyboard_state.caps_lock; return;
    }
    
    // Buffer overflow protection
    if (buffer_size >= KEYBOARD_BUFFER_THRESHOLD) {
        outb(0x20, 0x20);  // Send End Of Interrupt
        return;
    }
    
    if (scancode < 0x80 && scancode < sizeof(scancode_to_ascii)) {
        char ascii = scancode_to_ascii[scancode];
        if (ascii != 0 && buffer_size < KEYBOARD_BUFFER_SIZE) {
            buffer[buffer_end] = ascii;
            buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
            buffer_size++;
            
            // Remove automatic echo - let main loop handle it
        }
    }
    
    // Send End Of Interrupt to Programmable Interrupt Controller
    outb(0x20, 0x20);
    interrupt_in_progress = false;
}

void keyboard_init(void) {
    // Wait for keyboard controller to be ready
    while ((inb(KEYBOARD_STATUS_PORT) & 2) != 0) {
        io_wait();
    }
    
    // Reset keyboard controller
    outb(KEYBOARD_DATA_PORT, 0xFF);
    
    // Register keyboard handler
    isr_register_handler(IRQ_KEYBOARD, keyboard_handler);
    
    // Enable keyboard interrupt
    outb(0x21, inb(0x21) & ~(1 << IRQ_KEYBOARD));
}

char keyboard_getchar(void) {
    char c = 0;
    
    while (!keyboard_available() && !interrupt_in_progress) {
        if (!interrupts_enabled()) return 0;  // Prevent deadlock
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
