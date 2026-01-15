/*
 * idt.h - Interrupt Descriptor Table
 * 
 * Gère la table des descripteurs d'interruptions (IDT)
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// =============================================================================
// Structures
// =============================================================================

/**
 * Entrée de l'IDT (8 octets)
 */
struct idt_entry {
    uint16_t base_low;      // Bits 0-15 de l'adresse du handler
    uint16_t selector;      // Sélecteur de segment (0x08 = kernel code segment)
    uint8_t  always0;       // Toujours 0
    uint8_t  flags;         // Type et attributs
    uint16_t base_high;     // Bits 16-31 de l'adresse du handler
} __attribute__((packed));

/**
 * Pointeur vers l'IDT (utilisé par l'instruction LIDT)
 */
struct idt_ptr {
    uint16_t limit;         // Taille de l'IDT - 1
    uint32_t base;          // Adresse de l'IDT
} __attribute__((packed));

// =============================================================================
// Constantes
// =============================================================================

#define IDT_ENTRIES 256

// Flags pour les entrées IDT
#define IDT_FLAG_PRESENT    0x80    // Bit 7: Present
#define IDT_FLAG_RING0      0x00    // Bits 5-6: Ring 0 (kernel)
#define IDT_FLAG_RING3      0x60    // Bits 5-6: Ring 3 (user)
#define IDT_FLAG_GATE_INT   0x0E    // Bits 0-3: 32-bit Interrupt Gate
#define IDT_FLAG_GATE_TRAP  0x0F    // Bits 0-3: 32-bit Trap Gate

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise l'IDT
 */
void idt_init(void);

/**
 * Définit une entrée dans l'IDT
 * 
 * @param num    Numéro de l'interruption (0-255)
 * @param base   Adresse du handler
 * @param sel    Sélecteur de segment (généralement 0x08)
 * @param flags  Flags (type de gate, privilege, etc.)
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif // IDT_H
