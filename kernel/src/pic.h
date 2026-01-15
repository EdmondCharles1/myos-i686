/*
 * pic.h - Programmable Interrupt Controller (8259 PIC)
 */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// =============================================================================
// Ports I/O du PIC 8259
// =============================================================================

#define PIC1_COMMAND    0x20        // PIC maître - Command
#define PIC1_DATA       0x21        // PIC maître - Data
#define PIC2_COMMAND    0xA0        // PIC esclave - Command
#define PIC2_DATA       0xA1        // PIC esclave - Data

// Commandes PIC
#define PIC_EOI         0x20        // End Of Interrupt

// Commandes d'initialisation
#define ICW1_ICW4       0x01        // ICW4 nécessaire
#define ICW1_SINGLE     0x02        // Mode single (pas de cascade)
#define ICW1_INTERVAL4  0x04        // Call address interval 4 (8)
#define ICW1_LEVEL      0x08        // Level triggered (edge) mode
#define ICW1_INIT       0x10        // Initialisation

#define ICW4_8086       0x01        // Mode 8086/88 (MCS-80/85)
#define ICW4_AUTO       0x02        // Auto (normal) EOI
#define ICW4_BUF_SLAVE  0x08        // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C        // Buffered mode/master
#define ICW4_SFNM       0x10        // Special fully nested (not)

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise les PICs (maître et esclave)
 * Remappe les IRQs de 0-15 vers 32-47
 */
void pic_init(void);

/**
 * Envoie un End Of Interrupt (EOI) au PIC
 * 
 * @param irq Numéro de l'IRQ (0-15)
 */
void pic_send_eoi(uint8_t irq);

/**
 * Désactive une IRQ spécifique
 * 
 * @param irq Numéro de l'IRQ (0-15)
 */
void pic_disable_irq(uint8_t irq);

/**
 * Active une IRQ spécifique
 * 
 * @param irq Numéro de l'IRQ (0-15)
 */
void pic_enable_irq(uint8_t irq);

#endif // PIC_H
