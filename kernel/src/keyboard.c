/*
 * keyboard.c - Driver clavier PS/2 (version simplifiée pour test)
 */

#include "keyboard.h"
#include "irq.h"
#include "printf.h"

// =============================================================================
// Ports I/O
// =============================================================================

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// =============================================================================
// Table de conversion scancode → ASCII (US QWERTY)
// =============================================================================

static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// =============================================================================
// Buffer circulaire
// =============================================================================

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t buffer_read_pos = 0;
static volatile uint32_t buffer_write_pos = 0;

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;

// Compteur de debug
static volatile uint32_t irq_count = 0;

// =============================================================================
// Fonctions internes
// =============================================================================

static void buffer_put(char c) {
    uint32_t next_pos = (buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
    
    if (next_pos != buffer_read_pos) {
        keyboard_buffer[buffer_write_pos] = c;
        buffer_write_pos = next_pos;
    }
}

static bool buffer_has_data(void) {
    return buffer_read_pos != buffer_write_pos;
}

static char buffer_get(void) {
    if (!buffer_has_data()) {
        return 0;
    }
    
    char c = keyboard_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

// =============================================================================
// Handler IRQ1 (clavier)
// =============================================================================

static void keyboard_handler(registers_t* regs) {
    (void)regs;
    
    irq_count++;
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Key released (bit 7 set)
    if (scancode & 0x80) {
        scancode &= 0x7F;
        
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            shift_pressed = false;
        } else if (scancode == KEY_CTRL) {
            ctrl_pressed = false;
        } else if (scancode == KEY_ALT) {
            alt_pressed = false;
        }
        return;
    }
    
    // Key pressed
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = true;
        return;
    } else if (scancode == KEY_CTRL) {
        ctrl_pressed = true;
        return;
    } else if (scancode == KEY_ALT) {
        alt_pressed = true;
        return;
    } else if (scancode == KEY_CAPSLOCK) {
        caps_lock = !caps_lock;
        return;
    }
    
    // Convertir en ASCII
    char c = 0;
    
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
        
        if (caps_lock && c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }
    
    if (c != 0) {
        buffer_put(c);
    }
}

// =============================================================================
// API publique
// =============================================================================

void keyboard_init(void) {
    printf("[KEYBOARD] Initialisation du driver clavier...\n");
    
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    caps_lock = false;
    irq_count = 0;
    
    irq_register_handler(1, keyboard_handler);
    
    printf("[KEYBOARD] Driver clavier initialise (IRQ1)\n");
}

char keyboard_getchar(void) {
    // VERSION SIMPLE : boucle d'attente avec compteur de sécurité
    uint32_t timeout = 0;
    const uint32_t MAX_TIMEOUT = 10000000; // ~10 secondes
    
    while (!buffer_has_data() && timeout < MAX_TIMEOUT) {
        timeout++;
        // Pause CPU
        __asm__ volatile ("pause");
    }
    
    if (timeout >= MAX_TIMEOUT) {
        // Timeout atteint, retourner un caractère neutre
        return 0;
    }
    
    return buffer_get();
}

bool keyboard_has_char(void) {
    return buffer_has_data();
}

void keyboard_flush(void) {
    buffer_read_pos = buffer_write_pos;
}

uint32_t keyboard_get_irq_count(void) {
    return irq_count;
}
