/*
 * idt.c - Implementation de l'IDT
 */

#include "idt.h"
#include "printf.h"

// =============================================================================
// Variables globales
// =============================================================================

// Table IDT (256 entrées)
static struct idt_entry idt[IDT_ENTRIES];

// Pointeur IDT
static struct idt_ptr idtp;

// =============================================================================
// Fonctions internes
// =============================================================================

/**
 * Charge l'IDT dans le CPU (assembleur inline)
 */
extern void idt_load(uint32_t);

// =============================================================================
// Implémentation
// =============================================================================

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags | IDT_FLAG_PRESENT;
}

void idt_init(void) {
    printf("[IDT] Initialisation de l'IDT...\n");
    
    // Configurer le pointeur IDT
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;
    
    // Initialiser toutes les entrées à 0
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Charger l'IDT dans le CPU
    idt_load((uint32_t)&idtp);
    
    printf("[IDT] IDT chargee avec %d entrees\n", IDT_ENTRIES);
}
