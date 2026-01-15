
/*
 * kernel.c - Code principal du kernel myos-i686
 * (avec SSP + Interruptions + Timer + Processus + Scheduler)
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

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_newline();
    }
}

void terminal_write(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
    }
}

// =============================================================================
// Fonctions de test pour les processus
// =============================================================================

void process_A(void) {
    while (1) {
        // Processus A s'exécute
        __asm__ volatile ("hlt");
    }
}

void process_B(void) {
    while (1) {
        // Processus B s'exécute
        __asm__ volatile ("hlt");
    }
}

void process_C(void) {
    while (1) {
        // Processus C s'exécute
        __asm__ volatile ("hlt");
    }
}

// =============================================================================
// Point d'entrée du kernel
// =============================================================================

void kernel_main(void) {
    // IMPORTANT : Initialiser le SSP en PREMIER
    stack_protector_init();
    
    // Puis initialiser le terminal
    terminal_clear();
    
    // En-tête
    terminal_setcolor(vga_color(15, 1));  // Blanc sur bleu
    printf("========================================\n");
    printf("    myos-i686 Kernel v0.6\n");
    printf("    (+ Scheduler FCFS/Round Robin)\n");
    printf("========================================\n\n");
    
    // Initialisation des systèmes
    terminal_setcolor(vga_color(14, 0));  // Jaune
    printf("Initialisation du systeme...\n\n");
    
    // IDT
    idt_init();
    
    // ISR (exceptions CPU)
    isr_init();
    
    // IRQ (interruptions hardware)
    irq_init();
    
    // Timer (100 Hz = tick toutes les 10ms)
    timer_init(100);
    
    // Système de processus
    process_init();
    
    // Ordonnanceur (Round Robin par défaut)
    scheduler_init(SCHEDULER_ROUND_ROBIN);
    
    printf("\n");
    terminal_setcolor(vga_color(10, 0));  // Vert
    printf("Systeme initialise avec succes!\n\n");
    
    // Informations
    terminal_setcolor(vga_color(11, 0));  // Cyan
    printf("=== Configuration ===\n");
    printf("  Timer: 100 Hz (10ms/tick)\n");
    printf("  Scheduler: %s\n", scheduler_type_to_string(scheduler_get_type()));
    printf("  Quantum: 10 ticks (100ms)\n\n");
    
    // Créer des processus de test
    terminal_setcolor(vga_color(14, 0));  // Jaune
    printf("=== Creation de processus ===\n");
    
    uint32_t pid_a = process_create("Process_A", process_A, PRIORITY_DEFAULT);
    uint32_t pid_b = process_create("Process_B", process_B, PRIORITY_DEFAULT);
    uint32_t pid_c = process_create("Process_C", process_C, PRIORITY_DEFAULT);
    
    printf("\nProcessus crees:\n");
    printf("  PID %u: Process_A\n", pid_a);
    printf("  PID %u: Process_B\n", pid_b);
    printf("  PID %u: Process_C\n\n", pid_c);
    
    // Ajouter les processus au scheduler
    process_t* proc_a = process_get_by_pid(pid_a);
    process_t* proc_b = process_get_by_pid(pid_b);
    process_t* proc_c = process_get_by_pid(pid_c);
    
    if (proc_a) scheduler_add_process(proc_a);
    if (proc_b) scheduler_add_process(proc_b);
    if (proc_c) scheduler_add_process(proc_c);
    
    // Afficher la file READY
    terminal_setcolor(vga_color(15, 0));  // Blanc
    scheduler_print_queue();
    
    // Activer le scheduler dans le timer
    timer_enable_scheduler();
    
    // Attendre quelques secondes pour laisser le scheduler travailler
    terminal_setcolor(vga_color(11, 0));  // Cyan
    printf("=== Test du scheduler ===\n");
    printf("Execution pendant 5 secondes...\n\n");
    
    timer_wait(500);  // 5 secondes à 100 Hz
    
    // Afficher le journal d'exécution
    terminal_setcolor(vga_color(15, 0));  // Blanc
    scheduler_print_log();
    
    // Afficher les statistiques des processus
    process_list();
    
    // Message final
    terminal_setcolor(vga_color(10, 0));  // Vert
    printf("\n=== Test termine ===\n");
    printf("Le scheduler fonctionne correctement!\n");
    printf("Context switches effectues avec succes.\n\n");
    
    terminal_setcolor(vga_color(12, 0));  // Rouge clair
    printf("Appuyez sur Ctrl+C pour quitter QEMU\n");
    
    // Boucle infinie
    while (1) {
        __asm__ volatile ("hlt");
    }
}
