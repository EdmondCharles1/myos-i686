/*
 * kernel.c - Code principal du kernel myos-i686 (avec printf)
 */

#include <stdint.h>
#include <stddef.h>
#include "printf.h"

// =============================================================================
// Configuration VGA
// =============================================================================

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Pointeur vers la mémoire VGA
volatile uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;

// Position actuelle du curseur
static size_t terminal_row = 0;
static size_t terminal_column = 0;

// Couleur actuelle
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
    terminal_clear();
    
    // Test de printf
    terminal_setcolor(vga_color(15, 1));  // Blanc sur bleu
    printf("========================================\n");
    printf("       myos-i686 Kernel v0.2 \n");
    printf("       (avec printf!) \n");
    printf("========================================\n\n");
    
    terminal_setcolor(vga_color(10, 0));  // Vert clair
    printf("Hello from printf!\n\n");
    
    terminal_setcolor(vga_color(14, 0));  // Jaune
    printf("Tests de formatage:\n");
    printf("  Caractere: %c\n", 'A');
    printf("  Chaine: %s\n", "myos-i686");
    printf("  Decimal: %d\n", 42);
    printf("  Negatif: %d\n", -123);
    printf("  Unsigned: %u\n", 4294967295U);
    printf("  Hexadecimal: 0x%x\n", 0xDEADBEEF);
    printf("  Hexadecimal MAJ: 0x%X\n", 0xCAFEBABE);
    printf("  Pointeur: %p\n", (void*)0xB8000);
    printf("  Pourcent: 100%%\n\n");
    
    terminal_setcolor(vga_color(11, 0));  // Cyan
    printf("Kernel compile avec i686-elf-gcc\n");
    printf("Architecture: x86 (32-bit)\n");
    printf("Mode: Bare-metal\n\n");
    
    terminal_setcolor(vga_color(12, 0));  // Rouge clair
    printf("Appuyez sur Ctrl+C pour quitter QEMU\n");
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}
