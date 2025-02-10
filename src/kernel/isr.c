#include <system/isr.h>
#include <system/io.h>

#define MAX_INTERRUPTS 256
static interrupt_service_routine_t interrupt_handlers[MAX_INTERRUPTS];

void isr_init(void) {
    // Initialize all handlers to NULL
    for (int i = 0; i < MAX_INTERRUPTS; i++) {
        interrupt_handlers[i] = NULL;
    }
}

void isr_register_handler(uint8_t interrupt, interrupt_service_routine_t handler) {
    if (interrupt >= MAX_INTERRUPTS) return;
    interrupt_handlers[interrupt] = handler;
}

void isr_enable(void) {
    __asm__ volatile("sti");
}

void isr_disable(void) {
    __asm__ volatile("cli");
}

// Check if interrupts are enabled
boolean interrupts_enabled(void) {
    unsigned long flags;
    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=g"(flags));
    return flags & (1 << 9); // Check IF (Interrupt Flag)
}

// Called from assembly when an interrupt occurs
void isr_handler(uint8_t interrupt) {
    if (interrupt_handlers[interrupt] != NULL) {
        interrupt_handlers[interrupt]();
        outb(0x20, 0x20); // Send End Of Interrupt
    }
}
