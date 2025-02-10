#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <system/type.h>

// Keyboard scan codes
#define KEY_SHIFT 0x2A
#define KEY_CTRL  0x1D
#define KEY_ALT   0x38
#define KEY_ENTER 0x1C

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

// Get keyboard state
boolean keyboard_is_shift_pressed(void);

#endif
