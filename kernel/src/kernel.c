/*
 * kernel.c - Code principal du kernel myos-i686
 * Version COMPLETE avec tous les modules
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
#include "memory.h"
#include "ipc.h"
#include "sync.h"

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

/**
 * Scrolling: Fait remonter tout l'ecran d'une ligne
 * - La premiere ligne disparait
 * - Les autres lignes remontent
 * - La derniere ligne est videe
 */
void terminal_scroll(void) {
    // Copier chaque ligne vers la ligne precedente (lignes 1 a 24 -> lignes 0 a 23)
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src_index = y * VGA_WIDTH + x;
            const size_t dst_index = (y - 1) * VGA_WIDTH + x;
            vga_buffer[dst_index] = vga_buffer[src_index];
        }
    }

    // Vider la derniere ligne (ligne 24)
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = vga_entry(' ', terminal_color);
    }
}

/**
 * Passe a la ligne suivante avec gestion du scrolling
 * - Line Wrapping: retour a la colonne 0
 * - Scrolling: si on depasse la ligne 25, l'ecran remonte
 */
void terminal_newline(void) {
    terminal_column = 0;
    terminal_row++;

    // Si on depasse la derniere ligne, faire defiler l'ecran
    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;  // Rester sur la derniere ligne
    }
}

/**
 * Met a jour la position du curseur materiel clignotant
 * Utilise les ports I/O 0x3D4 (index) et 0x3D5 (data) du controleur VGA
 */
void terminal_update_cursor(size_t x, size_t y) {
    uint16_t pos = y * VGA_WIDTH + x;

    // Envoyer la partie basse de la position (registre 0x0F)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    // Envoyer la partie haute de la position (registre 0x0E)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * Active et configure le curseur materiel clignotant
 * cursor_start: ligne de debut du curseur (0-15)
 * cursor_end: ligne de fin du curseur (0-15)
 * Pour un curseur en forme de bloc: start=0, end=15
 * Pour un curseur en forme de ligne: start=14, end=15
 */
void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    // Registre 0x0A: Cursor Start Register
    // Bit 5 = 0 pour activer le curseur (1 = desactive)
    outb(0x3D4, 0x0A);
    outb(0x3D5, (cursor_start & 0x1F));  // Bits 0-4: ligne de debut

    // Registre 0x0B: Cursor End Register
    outb(0x3D4, 0x0B);
    outb(0x3D5, (cursor_end & 0x1F));    // Bits 0-4: ligne de fin
}

/**
 * Desactive le curseur materiel
 */
void terminal_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);  // Bit 5 = 1 pour desactiver
}

/**
 * Gestion du backspace: efface le caractere precedent
 * - Recule le curseur d'une position
 * - Efface le caractere a cette position
 * - Gere le retour a la ligne precedente si necessaire
 */
void terminal_backspace(void) {
    if (terminal_column > 0) {
        // Cas simple: reculer dans la ligne actuelle
        terminal_column--;
    } else if (terminal_row > 0) {
        // Retour a la fin de la ligne precedente
        terminal_row--;
        terminal_column = VGA_WIDTH - 1;
    } else {
        // Deja au debut de l'ecran, ne rien faire
        return;
    }

    // Effacer le caractere a la position actuelle
    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);

    // Mettre a jour le curseur materiel
    terminal_update_cursor(terminal_column, terminal_row);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        // Nouvelle ligne
        terminal_newline();
    } else if (c == '\b') {
        // Backspace
        terminal_backspace();
        return;  // Ne pas mettre a jour le curseur une deuxieme fois
    } else if (c == '\r') {
        // Retour chariot: retour au debut de la ligne
        terminal_column = 0;
    } else if (c == '\t') {
        // Tabulation: avancer jusqu'a la prochaine position multiple de 8
        size_t next_tab = (terminal_column + 8) & ~7;
        if (next_tab >= VGA_WIDTH) {
            terminal_newline();
        } else {
            while (terminal_column < next_tab) {
                terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
                terminal_column++;
            }
        }
    } else if (c >= 32 && c < 127) {
        // Caractere imprimable
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

        // Line Wrapping: si on depasse 80 colonnes, passer a la ligne suivante
        if (++terminal_column >= VGA_WIDTH) {
            terminal_newline();
        }
    }
    // Ignorer les autres caracteres de controle

    // Mettre a jour le curseur materiel
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
    printf("    myos-i686 Kernel v0.8\n");
    printf("    OS Complet (Ordonnancement, IPC, Sync)\n");
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

    // Memory
    memory_init();

    // IPC
    ipc_init();

    // Sync
    sync_init();

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
