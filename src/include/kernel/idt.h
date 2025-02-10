#ifndef IDT_H
#define IDT_H

#include <stdint.h>

extern void idt_init(void);
extern void keyboard_handler_int(void);  // Add external assembly function declaration
void putchar(char c);
extern void load_idt(uint32_t);
extern void keyboard_handler(void);
char keyboard_getchar(void);
int keyboard_available(void);

#endif
