/*
 * isr.h - Interrupt Service Routines (Exceptions CPU)
 */

#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// =============================================================================
// Structure des registres sauvegardés lors d'une interruption
// =============================================================================

typedef struct {
    uint32_t ds;                                    // Segment de données
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Registres généraux (pusha)
    uint32_t int_no, err_code;                      // Numéro d'interruption et error code
    uint32_t eip, cs, eflags, useresp, ss;          // Pushés automatiquement par le CPU
} registers_t;

// =============================================================================
// Type pour les handlers d'interruptions
// =============================================================================

typedef void (*isr_t)(registers_t*);

// =============================================================================
// Prototypes des stubs assembleur (définis dans isr_asm.asm)
// =============================================================================

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise les ISR (exceptions CPU 0-31)
 */
void isr_init(void);

/**
 * Handler appelé par le stub assembleur
 */
void isr_handler(registers_t* regs);

/**
 * Enregistre un handler personnalisé pour une exception
 */
void isr_register_handler(uint8_t n, isr_t handler);

#endif // ISR_H
