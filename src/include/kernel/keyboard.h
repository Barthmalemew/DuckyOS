#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <system/type.h>

// Keyboard initialization
void keyboard_init(void);

// Get character from keyboard buffer
char keyboard_getchar(void);

// Check if keyboard input is available
boolean keyboard_available(void);

// Read a line of input (up to max_length)
int keyboard_readline(char* buffer, size_t max_length);

// Enable/disable keyboard echo
void keyboard_set_echo(boolean enable);

#endif
