/*
 * irq.h - Hardware Interrupt Requests
 */

#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include "isr.h"

// =============================================================================
// Prototypes des stubs assembleur (définis dans isr_asm.asm)
// =============================================================================

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

// =============================================================================
// Type pour les handlers d'IRQ
// =============================================================================

typedef void (*irq_handler_t)(registers_t*);

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise les IRQs (hardware interrupts)
 */
void irq_init(void);

/**
 * Handler appelé par le stub assembleur
 */
void irq_handler(registers_t* regs);

/**
 * Enregistre un handler personnalisé pour une IRQ
 * 
 * @param irq     Numéro de l'IRQ (0-15)
 * @param handler Fonction handler
 */
void irq_register_handler(uint8_t irq, irq_handler_t handler);

/**
 * Supprime un handler d'IRQ
 */
void irq_unregister_handler(uint8_t irq);

#endif // IRQ_H
