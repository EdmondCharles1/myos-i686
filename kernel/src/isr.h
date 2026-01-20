/*
 * isr.h - Interrupt Service Routines
 */

#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Structure des registres sauvegardés lors d'une interruption
typedef struct {
    uint32_t ds;                                    // Data segment
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Registres poussés par pusha
    uint32_t int_no, err_code;                      // Numéro d'interruption et code d'erreur
    uint32_t eip, cs, eflags, useresp, ss;          // Poussés automatiquement par le CPU
} registers_t;

// Type pour les handlers
typedef void (*isr_handler_t)(registers_t* regs);

/**
 * Initialise les ISR
 */
void isr_init(void);

/**
 * Handler ISR principal (appelé depuis isr_asm.asm)
 */
void isr_handler(registers_t* regs);

#endif // ISR_H
