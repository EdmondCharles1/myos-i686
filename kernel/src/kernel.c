
/*
 * kernel.c - Code principal du kernel myos-i686
 * (avec SSP + Interruptions + Timer)
 */

#include <stdint.h>
#include <stddef.h>
#include "printf.h"
#include "stack_protector.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"

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
    printf("    myos-i686 Kernel v0.4\n");
    printf("    (SSP + Interruptions + Timer)\n");
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
    
    printf("\n");
    terminal_setcolor(vga_color(10, 0));  // Vert
    printf("Systeme initialise avec succes!\n\n");
    
    // Informations de sécurité
    terminal_setcolor(vga_color(14, 0));  // Jaune
    printf("Securite:\n");
    printf("  Stack Smashing Protector: ACTIF\n");
    printf("  Canary: 0x%X\n\n", __stack_chk_guard);
    
    // Test du timer
    terminal_setcolor(vga_color(11, 0));  // Cyan
    printf("Test du timer...\n");
    printf("Attente de 3 secondes...\n");
    
    uint64_t start_ticks = timer_get_ticks();
    timer_wait(300);  // 300 ticks = 3 secondes à 100 Hz
    uint64_t end_ticks = timer_get_ticks();
    
    printf("Ticks ecoules: %u\n", (uint32_t)(end_ticks - start_ticks));
    printf("Temps ecoule: %u ms\n\n", (uint32_t)timer_get_ms());
    
    // Informations système
    terminal_setcolor(vga_color(15, 0));  // Blanc
    printf("Informations systeme:\n");
    printf("  Compilateur: i686-elf-gcc\n");
    printf("  Architecture: x86 (32-bit)\n");
    printf("  Mode: Bare-metal\n");
    printf("  Frequence timer: 100 Hz (10ms/tick)\n\n");
    
    // Affichage d'un compteur en temps réel
    terminal_setcolor(vga_color(14, 0));  // Jaune
    printf("Compteur de ticks (en temps reel):\n\n");
    
    // Position fixe pour le compteur
    size_t counter_row = terminal_row;
    size_t counter_col = terminal_column;
    
    uint64_t last_display_tick = 0;
    
    terminal_setcolor(vga_color(12, 0));  // Rouge clair
    terminal_newline();
    printf("Appuyez sur Ctrl+C pour quitter QEMU\n");
    
    // Boucle principale
    while (1) {
        uint64_t current_tick = timer_get_ticks();
        
        // Mettre à jour l'affichage toutes les 50 ticks (500ms)
        if (current_tick - last_display_tick >= 50) {
            last_display_tick = current_tick;
            
            // Sauvegarder la position actuelle
            size_t saved_row = terminal_row;
            size_t saved_col = terminal_column;
            uint8_t saved_color = terminal_color;
            
            // Aller à la position du compteur
            terminal_row = counter_row;
            terminal_column = counter_col;
            
            // Effacer la ligne
            for (size_t i = 0; i < 60; i++) {
                terminal_putchar(' ');
            }
            
            // Repositionner
            terminal_row = counter_row;
            terminal_column = counter_col;
            
            // Afficher le nouveau compteur
            terminal_setcolor(vga_color(10, 0));  // Vert clair
            printf("Ticks: %u | Temps: %u ms | Secondes: %u",
                   (uint32_t)current_tick,
                   (uint32_t)timer_get_ms(),
                   (uint32_t)(timer_get_ms() / 1000));
            
            // Restaurer la position
            terminal_row = saved_row;
            terminal_column = saved_col;
            terminal_color = saved_color;
        }
        
        // Halte le CPU jusqu'à la prochaine interruption
        __asm__ volatile ("hlt");
    }
}
