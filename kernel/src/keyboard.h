/*
 * keyboard.h - Support clavier PS/2
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

#define KEYBOARD_BUFFER_SIZE  256

#define KEY_ESCAPE      0x01
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_ENTER       0x1C
#define KEY_CTRL        0x1D
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_ALT         0x38
#define KEY_CAPSLOCK    0x3A
#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F10         0x44
#define KEY_NUMLOCK     0x45
#define KEY_SCROLLLOCK  0x46

void keyboard_init(void);
char keyboard_getchar(void);
bool keyboard_has_char(void);
void keyboard_flush(void);
uint32_t keyboard_get_irq_count(void);

#endif // KEYBOARD_H
