/*
 * pic.h - Contrôleur d'interruptions programmable (PIC 8259)
 */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// Ports I/O du PIC
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// Commandes PIC
#define PIC_EOI         0x20    // End Of Interrupt

/**
 * Remap le PIC (changer les vecteurs d'interruption)
 * 
 * @param offset1 Offset pour PIC1 (généralement 32)
 * @param offset2 Offset pour PIC2 (généralement 40)
 */
void pic_remap(uint8_t offset1, uint8_t offset2);

/**
 * Envoie End Of Interrupt au PIC
 * 
 * @param irq Numéro de l'IRQ (0-15)
 */
void pic_send_eoi(uint8_t irq);

/**
 * Active/désactive une IRQ spécifique
 * 
 * @param irq Numéro de l'IRQ (0-15)
 */
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);

#endif // PIC_H
