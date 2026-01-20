/*
 * isr.c - Interrupt Service Routines
 */

#include "isr.h"
#include "idt.h"
#include "printf.h"

// =============================================================================
// Déclarations externes (définis dans isr_asm.asm)
// =============================================================================

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// =============================================================================
// Table des handlers personnalisés (optionnel)
// =============================================================================

static isr_handler_t interrupt_handlers[256] = { NULL };

// =============================================================================
// Messages d'exception
// =============================================================================

static const char* exception_messages[] = {
    "Division par zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

// =============================================================================
// Initialisation
// =============================================================================

void isr_init(void) {
    printf("[ISR] Installation des handlers d'exceptions...\n");
    
    // Installer les 32 ISR (exceptions CPU 0-31)
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    
    printf("[ISR] 32 exceptions CPU installees\n");
}

// =============================================================================
// Handler ISR principal (appelé depuis isr_asm.asm)
// =============================================================================

void isr_handler(registers_t* regs) {
    // Appeler le handler personnalisé s'il existe
    if (interrupt_handlers[regs->int_no] != NULL) {
        interrupt_handlers[regs->int_no](regs);
        return;
    }
    
    // Sinon, afficher l'exception
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
    printf("\nSysteme arrete.\n");
    
    // Halt
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

// =============================================================================
// API publique (optionnel)
// =============================================================================

void isr_register_handler(uint8_t n, isr_handler_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_unregister_handler(uint8_t n) {
    interrupt_handlers[n] = NULL;
}
