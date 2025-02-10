#ifndef IDT_H
#define IDT_H

#include <stdint.h>

extern void idt_init(void);
extern void keyboard_handler_int(void);  // Add external assembly function declaration
extern void load_idt(uint32_t);

#endif
