/*
 * pic.c - Implementation du PIC 8259
 */

#include "pic.h"
#include "printf.h"

// =============================================================================
// Fonctions I/O (assembleur inline)
// =============================================================================

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);  // Port 0x80 est utilisé pour l'attente I/O
}

// =============================================================================
// Implémentation
// =============================================================================

void pic_init(void) {
    printf("[PIC] Remappage des IRQs (0-15 -> 32-47)...\n");
    
    // Sauvegarder les masques
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Initialisation en cascade (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // Vecteurs d'interruption (ICW2)
    outb(PIC1_DATA, 32);    // PIC maître : IRQ 0-7  -> INT 32-39
    io_wait();
    outb(PIC2_DATA, 40);    // PIC esclave : IRQ 8-15 -> INT 40-47
    io_wait();
    
    // Configuration de la cascade (ICW3)
    outb(PIC1_DATA, 0x04);  // PIC maître : IRQ2 = cascade
    io_wait();
    outb(PIC2_DATA, 0x02);  // PIC esclave : cascade identity = 2
    io_wait();
    
    // Mode 8086 (ICW4)
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    // Restaurer les masques
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
    
    printf("[PIC] PIC initialise\n");
}

void pic_send_eoi(uint8_t irq) {
    // Si l'IRQ vient du PIC esclave, envoyer EOI aux deux PICs
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    
    // Toujours envoyer EOI au PIC maître
    outb(PIC1_COMMAND, PIC_EOI);
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
