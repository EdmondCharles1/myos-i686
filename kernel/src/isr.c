/*
 * isr.c - Implementation des ISR (Interrupt Service Routines)
 */

#include "isr.h"
#include "idt.h"
#include "printf.h"

// =============================================================================
// Noms des exceptions CPU
// =============================================================================

static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// =============================================================================
// Table des handlers personnalisés
// =============================================================================

static isr_t interrupt_handlers[256];

// =============================================================================
// Implémentation
// =============================================================================

void isr_init(void) {
    printf("[ISR] Installation des handlers d'exceptions...\n");
    
    // Installer les 32 exceptions CPU dans l'IDT
    idt_set_gate(0,  (uint32_t)isr0,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_FLAG_GATE_INT | IDT_FLAG_RING0);
    
    printf("[ISR] 32 exceptions CPU installees\n");
}

void isr_handler(registers_t* regs) {
    // Vérifier s'il y a un handler personnalisé
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        // Afficher l'exception
        printf("\n!!! EXCEPTION CPU !!!\n");
        printf("Exception: %s (0x%x)\n", 
               exception_messages[regs->int_no], 
               regs->int_no);
        printf("Error code: 0x%x\n", regs->err_code);
        printf("EIP: 0x%x  CS: 0x%x  EFLAGS: 0x%x\n", 
               regs->eip, regs->cs, regs->eflags);
        printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx);
        printf("ESP: 0x%x  EBP: 0x%x  ESI: 0x%x  EDI: 0x%x\n",
               regs->esp, regs->ebp, regs->esi, regs->edi);
        
        // Halt le système
        printf("\nSysteme arrete.\n");
        for(;;);
    }
}

void isr_register_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}