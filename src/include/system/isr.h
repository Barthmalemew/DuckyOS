#ifndef ISR_H
#define ISR_H

#include <system/type.h>

// Interrupt handler function type
typedef void (*interrupt_service_routine_t)(void);

// Register interrupt handler
void isr_register_handler(uint8_t interrupt, interrupt_service_routine_t handler);

// Initialize interrupt system
void isr_init(void);

// Enable/disable interrupts
void isr_enable(void);
void isr_disable(void);

// Common interrupt numbers
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE 2
#define IRQ_COM2 3
#define IRQ_COM1 4
#define IRQ_LPT2 5
#define IRQ_FLOPPY 6
#define IRQ_LPT1 7
#define IRQ_CMOS 8
#define IRQ_PS2MOUSE 12
#define IRQ_FPU 13
#define IRQ_ATA1 14
#define IRQ_ATA2 15

#endif
