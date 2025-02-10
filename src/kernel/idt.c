#include <kernel/idt.h>
#include <system/io.h>
#include <system/type.h>

#define KEYBOARD_BUFFER_SIZE 256

// Circular keyboard buffer
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_start = 0;
static volatile int buffer_end = 0;
static volatile int buffer_size = 0;

#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1
#define PIC_EOI         0x20

// IDT entry structure
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// IDT entries array
static struct idt_entry idt[256];
static struct idt_ptr idtp;

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// Scancode to ASCII conversion table (US layout)
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// rest of the code...

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Only handle key press events (ignore key release)
    if (scancode < 0x80 && scancode < sizeof(scancode_to_ascii)) {
        char ascii = scancode_to_ascii[scancode];
        if (ascii != 0) {
            // Add to circular buffer if there's space
            if (buffer_size < KEYBOARD_BUFFER_SIZE) {
                keyboard_buffer[buffer_end] = ascii;
                buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
                buffer_size++;
                
                // Echo character to screen
                putchar(ascii);
            }
        }
    }

    // Send End Of Interrupt to Programmable Interrupt Controller
    outb(PIC1_COMMAND, PIC_EOI);
}

// Initialize the IDT
void idt_init(void) {
    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt[i].base_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].flags = 0;
        idt[i].base_high = 0;
    }

    // Remap PIC
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);

    // Set up keyboard interrupt
    idt_set_gate(33, (uint32_t)keyboard_handler_int, 0x08, 0x8E);

    // Load IDT
    load_idt((unsigned)&idtp);

    // Enable interrupts
    __asm__ volatile("sti");
}
