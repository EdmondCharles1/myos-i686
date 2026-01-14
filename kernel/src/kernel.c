/*
 * kernel.c - Code principal du kernel MYOS-I686
 * 
 * Mémoire VGA mode texte 80x25 :
 * - Adresse : 0xB8000
 * - Format : 16 bits par caractère (8 bits couleur + 8 bits ASCII)
 * - Couleur : 4 bits background + 4 bits foreground
 */

#include <stdint.h>
#include <stddef.h>

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

// Couleur actuelle (blanc sur noir par défaut)
static uint8_t terminal_color = 0x0F;

// =============================================================================
// Fonctions utilitaires
// =============================================================================

/**
 * Crée une entrée VGA (caractère + couleur)
 */
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Crée un code couleur VGA (4 bits background + 4 bits foreground)
 */
static inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

/**
 * Efface l'écran (remplit avec des espaces)
 */
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

/**
 * Change la couleur du terminal
 */
void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

/**
 * Affiche un caractère à une position donnée
 */
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

/**
 * Passe à la ligne suivante
 */
void terminal_newline(void) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
        terminal_row = 0;  // Retour en haut (pas de scroll)
    }
}

/**
 * Affiche un caractère
 */
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

/**
 * Affiche une chaîne de caractères
 */
void terminal_write(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
    }
}

/**
 * Affiche une chaîne avec une longueur spécifique
 */
void terminal_writestring(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

// =============================================================================
// Point d'entrée du kernel
// =============================================================================

void kernel_main(void) {
    // Efface l'écran
    terminal_clear();
    
    // Titre en blanc sur bleu
    terminal_setcolor(vga_color(15, 1));  // 15=blanc, 1=bleu
    terminal_write("========================================");
    terminal_newline();
    terminal_write("         MYOS-I686 Kernel v0.1          ");
    terminal_newline();
    terminal_write("========================================");
    terminal_newline();
    terminal_newline();
    
    // Texte en vert clair sur noir
    terminal_setcolor(vga_color(10, 0));  // 10=vert clair, 0=noir
    terminal_write("Bonjour depuis le kernel i686 !");
    terminal_newline();
    terminal_newline();
    
    // Informations en jaune sur noir
    terminal_setcolor(vga_color(14, 0));  // 14=jaune, 0=noir
    terminal_write("Compilateur : i686-elf-gcc");
    terminal_newline();
    terminal_write("Architecture : x86 (32-bit)");
    terminal_newline();
    terminal_write("Mode : Bare-metal (freestanding)");
    terminal_newline();
    terminal_newline();
    
    // Message final en cyan sur noir
    terminal_setcolor(vga_color(11, 0));  // 11=cyan clair, 0=noir
    terminal_write("Le kernel tourne correctement !");
    terminal_newline();
    terminal_write("Appuyez sur Ctrl+C pour quitter QEMU");
    
    // Boucle infinie (kernel ne doit jamais se terminer)
    while (1) {
        __asm__ volatile ("hlt");  // Halte le CPU jusqu'à la prochaine interruption
    }
}
