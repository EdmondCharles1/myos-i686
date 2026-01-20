/*
 * pic.c - Contrôleur d'interruptions programmable (PIC 8259)
 */

#include "pic.h"
#include "printf.h"

// =============================================================================
// Ports I/O
// =============================================================================

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Délai pour laisser le PIC se stabiliser
static inline void io_wait(void) {
    outb(0x80, 0);
}

// =============================================================================
// Fonctions PIC
// =============================================================================

void pic_remap(uint8_t offset1, uint8_t offset2) {
    printf("[PIC] Remappage des IRQs (%u-15 -> %u-47)...\n", 0, offset1);
    
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Initialisation ICW1
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();
    
    // ICW2 : Vecteurs d'interruption
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    
    // ICW3 : Configuration en cascade
    outb(PIC1_DATA, 0x04);  // PIC1 : esclave sur IRQ2
    io_wait();
    outb(PIC2_DATA, 0x02);  // PIC2 : identifiant cascade 2
    io_wait();
    
    // ICW4 : Mode 8086
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();
    
    // Restaurer les masques
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
    
    printf("[PIC] PIC initialise\n");
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}
