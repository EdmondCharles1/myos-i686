/*
 * idt.c - Interrupt Descriptor Table
 */

#include "idt.h"
#include "printf.h"

// =============================================================================
// Variables globales
// =============================================================================

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

// =============================================================================
// Fonction pour charger l'IDT (assembleur inline)
// =============================================================================

static inline void idt_load(uint32_t idt_ptr_addr) {
    __asm__ volatile ("lidt (%0)" : : "r"(idt_ptr_addr));
}

// =============================================================================
// Implémentation
// =============================================================================

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    printf("[IDT] Initialisation de l'IDT...\n");
    
    // Initialiser le pointeur IDT
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Effacer l'IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].base_lo = 0;
        idt[i].base_hi = 0;
        idt[i].sel = 0;
        idt[i].always0 = 0;
        idt[i].flags = 0;
    }
    
    // Charger l'IDT
    idt_load((uint32_t)&idt_ptr);
    
    printf("[IDT] IDT chargee avec %d entrees\n", IDT_ENTRIES);
}
