/*
 * kernel.c - Code principal du kernel myos-i686
 * Version FINALE avec Shell + Curseur
 */

#include <stdint.h>
#include <stddef.h>
#include "printf.h"
#include "stack_protector.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"
#include "process.h"
#include "scheduler.h"
#include "keyboard.h"
#include "shell.h"

// =============================================================================
// Configuration VGA
// =============================================================================

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

volatile uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;

static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color = 0x0F;

// =============================================================================
// Ports I/O
// =============================================================================

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// =============================================================================
// Fonctions VGA
// =============================================================================

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

void terminal_newline(void) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
        terminal_row = 0;
    }
}

void terminal_update_cursor(size_t x, size_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (cursor_start & 0x1F) | 0x20);
    
    outb(0x3D4, 0x0B);
    outb(0x3D5, cursor_end & 0x1F);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        
        if (++terminal_column == VGA_WIDTH) {
            terminal_newline();
        }
    }
    
    terminal_update_cursor(terminal_column, terminal_row);
}

void terminal_write(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
    }
}

// =============================================================================
// Fonctions de processus de test
// =============================================================================

void process_idle(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

// =============================================================================
// Point d'entrée du kernel
// =============================================================================

void kernel_main(void) {
    // Désactiver les interruptions
    __asm__ volatile ("cli");
    
    // Initialiser le SSP
    stack_protector_init();
    
    // Terminal
    terminal_clear();
    terminal_enable_cursor(14, 15);
    
    // En-tête
    terminal_setcolor(vga_color(15, 1));
    printf("========================================\n");
    printf("    myos-i686 Kernel v0.7\n");
    printf("    (Mini-Shell interactif)\n");
    printf("========================================\n\n");
    
    terminal_setcolor(vga_color(14, 0));
    printf("Initialisation du systeme...\n\n");
    
    // IDT
    idt_init();
    
    // ISR
    isr_init();
    
    // IRQ
    irq_init();
    
    // Timer
    timer_init(100);
    
    // Process
    process_init();
    
    // Scheduler
    scheduler_init(SCHEDULER_ROUND_ROBIN);
    
    // Keyboard
    keyboard_init();
    
    // Shell
    shell_init();
    
    printf("\n");
    terminal_setcolor(vga_color(10, 0));
    printf("Systeme initialise avec succes!\n\n");
    
    // Créer processus idle
    terminal_setcolor(vga_color(11, 0));
    printf("Creation du processus idle...\n");
    uint32_t pid_idle = process_create("idle", process_idle, PRIORITY_MIN);
    printf("Processus idle cree (PID=%u)\n\n", pid_idle);
    
    // Ajouter au scheduler
    process_t* proc_idle = process_get_by_pid(pid_idle);
    if (proc_idle) {
        scheduler_add_process(proc_idle);
    }
    
    // Activer le scheduler
    timer_enable_scheduler();
    
    printf("Activation des interruptions...\n");
    
    // RÉACTIVER les interruptions
    __asm__ volatile ("sti");
    
    printf("Interruptions activees!\n\n");
    
    // Attendre stabilisation
    for (volatile int i = 0; i < 10000000; i++);
    
    // NETTOYER L'ÉCRAN AVANT LE SHELL
    terminal_clear();
    terminal_setcolor(vga_color(15, 0));
    
    // Lancer le shell
    shell_run();
    
    // Ne devrait jamais arriver ici
    while (1) {
        __asm__ volatile ("hlt");
    }
}
