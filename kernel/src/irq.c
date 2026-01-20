/*
 * irq.c - Gestion des IRQ (Interruptions matérielles)
 */

#include "irq.h"
#include "idt.h"
#include "pic.h"
#include "printf.h"

// =============================================================================
// Handlers par défaut
// =============================================================================

static irq_handler_t irq_handlers[16] = { NULL };

// =============================================================================
// Stubs IRQ (définis dans isr_asm.asm)
// =============================================================================

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

// Table des stubs
static void (*irq_stub_table[16])(void) = {
    irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
    irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
};

// =============================================================================
// Installation d'un handler IRQ dans l'IDT
// =============================================================================

static void irq_install_handler(uint8_t num, void (*handler)(void)) {
    idt_set_gate(num, (uint32_t)handler, 0x08, 0x8E);
}

// =============================================================================
// Handler IRQ principal (appelé depuis isr_asm.asm)
// =============================================================================

void irq_handler(registers_t* regs) {
    // Numéro de l'IRQ (0-15)
    uint32_t irq_num = regs->int_no - 32;
    
    // Appeler le handler enregistré (si existant)
    if (irq_num < 16 && irq_handlers[irq_num] != NULL) {
        irq_handlers[irq_num](regs);
    }
    
    // Envoyer EOI (End Of Interrupt) au PIC
    pic_send_eoi(irq_num);
}

// =============================================================================
// API publique
// =============================================================================

void irq_init(void) {
    printf("[IRQ] Installation des handlers IRQ...\n");
    
    // Remap PIC (IRQ 0-15 → INT 32-47)
    pic_remap(32, 40);
    
    // Installer les handlers dans l'IDT
    for (int i = 0; i < 16; i++) {
        irq_install_handler(i + 32, irq_stub_table[i]);
    }
    
    printf("[IRQ] IRQs installees et activees\n");
    
    // PAS DE sti ICI !
    // Les interruptions seront activées plus tard dans kernel_main()
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void irq_unregister_handler(uint8_t irq) {
    if (irq < 16) {
        irq_handlers[irq] = NULL;
    }
}
