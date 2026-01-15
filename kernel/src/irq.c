/*
 * irq.c - Implementation des IRQ handlers
 */

#include "irq.h"
#include "idt.h"
#include "pic.h"
#include "printf.h"

// =============================================================================
// Table des handlers IRQ personnalisés
// =============================================================================

static irq_handler_t irq_handlers[16];

// =============================================================================
// Implémentation
// =============================================================================

void irq_init(void) {
    printf("[IRQ] Installation des handlers IRQ...\n");
    
    // Installer les 16 IRQs dans l'IDT (interruptions 32-47)
    idt_set_gate(32, (uint32_t)irq0,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(33, (uint32_t)irq1,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(34, (uint32_t)irq2,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(35, (uint32_t)irq3,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(36, (uint32_t)irq4,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(37, (uint32_t)irq5,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(38, (uint32_t)irq6,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(39, (uint32_t)irq7,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(40, (uint32_t)irq8,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(41, (uint32_t)irq9,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    
    // Initialiser le PIC
    pic_init();
    
    // Activer les interruptions CPU
    __asm__ volatile ("sti");
    
    printf("[IRQ] IRQs installees et activees\n");
}

void irq_handler(registers_t* regs) {
    // Calculer le numéro d'IRQ (0-15)
    uint8_t irq_num = regs->int_no - 32;
    
    // Appeler le handler personnalisé si présent
    if (irq_handlers[irq_num] != 0) {
        irq_handler_t handler = irq_handlers[irq_num];
        handler(regs);
    }
    
    // Envoyer EOI au PIC
    pic_send_eoi(irq_num);
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
        pic_enable_irq(irq);
    }
}

void irq_unregister_handler(uint8_t irq) {
    if (irq < 16) {
        irq_handlers[irq] = 0;
        pic_disable_irq(irq);
    }
}
