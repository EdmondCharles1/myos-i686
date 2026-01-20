/*
 * irq.h - Gestion des IRQ (Interruptions matérielles)
 */

#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include "isr.h"

// Type pour les handlers IRQ
typedef void (*irq_handler_t)(registers_t* regs);

/**
 * Initialise le système IRQ
 * (Remap le PIC et installe les handlers dans l'IDT)
 */
void irq_init(void);

/**
 * Enregistre un handler pour une IRQ spécifique
 * 
 * @param irq     Numéro de l'IRQ (0-15)
 * @param handler Fonction à appeler lors de l'interruption
 */
void irq_register_handler(uint8_t irq, irq_handler_t handler);

/**
 * Désenregistre un handler IRQ
 * 
 * @param irq Numéro de l'IRQ (0-15)
 */
void irq_unregister_handler(uint8_t irq);

/**
 * Handler IRQ principal (appelé depuis isr_asm.asm)
 */
void irq_handler(registers_t* regs);

#endif // IRQ_H
