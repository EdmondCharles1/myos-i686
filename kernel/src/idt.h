/*
 * idt.h - Interrupt Descriptor Table
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// =============================================================================
// Constantes
// =============================================================================

#define IDT_ENTRIES 256

// Flags IDT
#define IDT_FLAG_PRESENT   0x80
#define IDT_FLAG_RING0     0x00
#define IDT_FLAG_RING3     0x60
#define IDT_FLAG_GATE_INT  0x0E    // Interrupt gate 32-bit
#define IDT_FLAG_GATE_TRAP 0x0F    // Trap gate 32-bit

// =============================================================================
// Structures
// =============================================================================

// Entrée dans l'IDT (8 octets)
typedef struct {
    uint16_t base_lo;   // Bits 0-15 de l'adresse du handler
    uint16_t sel;       // Sélecteur de segment (code segment)
    uint8_t  always0;   // Toujours 0
    uint8_t  flags;     // Flags (présent, DPL, type)
    uint16_t base_hi;   // Bits 16-31 de l'adresse du handler
} __attribute__((packed)) idt_entry_t;

// Pointeur vers l'IDT (6 octets)
typedef struct {
    uint16_t limit;     // Taille de l'IDT - 1
    uint32_t base;      // Adresse de l'IDT
} __attribute__((packed)) idt_ptr_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise l'IDT
 */
void idt_init(void);

/**
 * Configure une entrée dans l'IDT
 * 
 * @param num   Numéro de l'interruption (0-255)
 * @param base  Adresse du handler
 * @param sel   Sélecteur de segment (généralement 0x08)
 * @param flags Flags (présent, DPL, type de gate)
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif // IDT_H
