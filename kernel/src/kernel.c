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

// Configuration du scrollback buffer
#define SCROLLBACK_LINES    500     // Nombre de lignes d'historique (500 * 80 * 2 = 80KB)
#define SCROLLBACK_WIDTH    79      // Largeur utilisable (colonne 79 = scrollbar)
#define SCROLLBAR_COLUMN    79      // Colonne de la barre de defilement

volatile uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;

// =============================================================================
// Scrollback Buffer - Stockage de l'historique
// =============================================================================

// Buffer circulaire pour l'historique (stocke caractere + couleur)
static uint16_t scrollback_buffer[SCROLLBACK_LINES][VGA_WIDTH];

// Position d'ecriture dans le buffer (ligne courante absolue)
static size_t scrollback_write_line = 0;

// Nombre total de lignes ecrites (pour calculer la taille de l'historique)
static size_t scrollback_total_lines = 0;

// Offset de vue: 0 = vue actuelle (bas), >0 = remonter dans l'historique
static size_t view_line_offset = 0;

// Flag: sommes-nous en mode consultation d'historique?
static bool viewing_history = false;

// Position du curseur dans le buffer
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
// Fonctions VGA de base
// =============================================================================

// Declarations anticipees
void terminal_update_cursor(size_t x, size_t y);
void terminal_refresh_screen(void);
static void draw_scrollbar(void);

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

// =============================================================================
// Scrollback Buffer - Fonctions internes
// =============================================================================

/**
 * Convertit une ligne relative (0 = plus ancienne visible) en index du buffer circulaire
 */
/*static size_t scrollback_get_buffer_index(size_t relative_line) {
    if (scrollback_total_lines <= SCROLLBACK_LINES) {
        // Buffer pas encore plein
        return relative_line;
    } else {
        // Buffer circulaire plein
        return (scrollback_write_line + relative_line) % SCROLLBACK_LINES;
    }
}*/

/**
 * Retourne le nombre de lignes disponibles dans l'historique
 */
static size_t scrollback_get_history_size(void) {
    if (scrollback_total_lines <= VGA_HEIGHT) {
        return 0;  // Pas assez de lignes pour defiler
    }
    if (scrollback_total_lines <= SCROLLBACK_LINES) {
        return scrollback_total_lines - VGA_HEIGHT;
    }
    return SCROLLBACK_LINES - VGA_HEIGHT;
}

/**
 * Ecrit un caractere dans le scrollback buffer a la position actuelle
 */
static void scrollback_putchar_at(size_t x, size_t y, uint16_t entry) {
    size_t buffer_line = (scrollback_write_line - (VGA_HEIGHT - 1) + y + SCROLLBACK_LINES) % SCROLLBACK_LINES;
    if (x < VGA_WIDTH) {
        scrollback_buffer[buffer_line][x] = entry;
    }
}

/**
 * Lit un caractere du scrollback buffer
 */
static uint16_t scrollback_getchar_at(size_t x, size_t view_line) {
    size_t history_size = scrollback_get_history_size();
    size_t actual_offset = (view_line_offset > history_size) ? history_size : view_line_offset;

    size_t target_line;
    if (scrollback_total_lines <= SCROLLBACK_LINES) {
        target_line = scrollback_total_lines - VGA_HEIGHT - actual_offset + view_line;
        if (target_line >= scrollback_total_lines) {
            return vga_entry(' ', terminal_color);
        }
    } else {
        target_line = (scrollback_write_line - VGA_HEIGHT - actual_offset + view_line + SCROLLBACK_LINES) % SCROLLBACK_LINES;
    }

    if (x < VGA_WIDTH) {
        return scrollback_buffer[target_line][x];
    }
    return vga_entry(' ', terminal_color);
}

// =============================================================================
// Barre de defilement visuelle
// =============================================================================

/**
 * Dessine la barre de defilement sur la colonne 79
 * Utilise des caracteres speciaux pour montrer la position
 */
static void draw_scrollbar(void) {
    size_t history_size = scrollback_get_history_size();

    // Couleur de la scrollbar: gris sur noir
    uint8_t scrollbar_bg_color = vga_color(8, 0);   // Gris fonce (fond)
    uint8_t scrollbar_fg_color = vga_color(15, 0);  // Blanc (curseur)
    uint8_t scrollbar_indicator = vga_color(14, 0); // Jaune (indicateur historique)

    if (history_size == 0) {
        // Pas d'historique: barre grise simple
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            vga_buffer[y * VGA_WIDTH + SCROLLBAR_COLUMN] = vga_entry(0xB3, scrollbar_bg_color); // │
        }
        return;
    }

    // Calculer la position et taille du curseur de scrollbar
    size_t scrollbar_height = VGA_HEIGHT;
    size_t thumb_size = (VGA_HEIGHT * VGA_HEIGHT) / (history_size + VGA_HEIGHT);
    if (thumb_size < 1) thumb_size = 1;
    if (thumb_size > VGA_HEIGHT) thumb_size = VGA_HEIGHT;

    // Position du thumb (inversee car offset 0 = bas)
    size_t thumb_pos;
    if (history_size > 0) {
        thumb_pos = ((history_size - view_line_offset) * (scrollbar_height - thumb_size)) / history_size;
    } else {
        thumb_pos = scrollbar_height - thumb_size;
    }

    // Dessiner la scrollbar
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        uint16_t entry;
        if (y >= thumb_pos && y < thumb_pos + thumb_size) {
            // Thumb (curseur de position)
            entry = vga_entry(0xDB, scrollbar_fg_color);  // █ bloc plein
        } else {
            // Fond de la scrollbar
            entry = vga_entry(0xB0, scrollbar_bg_color);  // ░ bloc leger
        }
        vga_buffer[y * VGA_WIDTH + SCROLLBAR_COLUMN] = entry;
    }

    // Indicateur si on consulte l'historique
    if (viewing_history && view_line_offset > 0) {
        // Fleche vers le haut en haut de l'ecran
        vga_buffer[0 * VGA_WIDTH + SCROLLBAR_COLUMN] = vga_entry(0x1E, scrollbar_indicator); // ▲
    }
}

// =============================================================================
// Fonction de rafraichissement de l'ecran
// =============================================================================

/**
 * Rafraichit l'ecran VGA a partir du scrollback buffer
 * Copie les lignes visibles vers la memoire VGA 0xB8000
 */
void terminal_refresh_screen(void) {
    size_t history_size = scrollback_get_history_size();

    // Limiter l'offset de vue
    if (view_line_offset > history_size) {
        view_line_offset = history_size;
    }

    // Copier les lignes du buffer vers l'ecran VGA
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < SCROLLBAR_COLUMN; x++) {
            uint16_t entry = scrollback_getchar_at(x, y);
            vga_buffer[y * VGA_WIDTH + x] = entry;
        }
    }

    // Dessiner la barre de defilement
    draw_scrollbar();

    // Mettre a jour le curseur (seulement si on ne consulte pas l'historique)
    if (!viewing_history || view_line_offset == 0) {
        terminal_update_cursor(terminal_column, terminal_row);
    } else {
        // Cacher le curseur quand on consulte l'historique
        terminal_update_cursor(VGA_WIDTH, VGA_HEIGHT);  // Hors ecran
    }
}

// =============================================================================
// Fonctions de defilement de l'historique
// =============================================================================

/**
 * Fait defiler l'historique vers le haut (voir les anciennes lignes)
 * @param lines Nombre de lignes a remonter
 */
void terminal_scroll_up(size_t lines) {
    size_t history_size = scrollback_get_history_size();

    if (history_size == 0) {
        return;  // Pas d'historique
    }

    viewing_history = true;
    view_line_offset += lines;

    if (view_line_offset > history_size) {
        view_line_offset = history_size;
    }

    terminal_refresh_screen();
}

/**
 * Fait defiler l'historique vers le bas (vers les lignes recentes)
 * @param lines Nombre de lignes a descendre
 */
void terminal_scroll_down(size_t lines) {
    if (view_line_offset == 0) {
        viewing_history = false;
        return;
    }

    if (lines >= view_line_offset) {
        view_line_offset = 0;
        viewing_history = false;
    } else {
        view_line_offset -= lines;
    }

    terminal_refresh_screen();
}

/**
 * Retourne a la vue actuelle (bas de l'historique)
 */
void terminal_scroll_to_bottom(void) {
    view_line_offset = 0;
    viewing_history = false;
    terminal_refresh_screen();
}

/**
 * Va au debut de l'historique (plus anciennes lignes)
 */
void terminal_scroll_to_top(void) {
    size_t history_size = scrollback_get_history_size();
    view_line_offset = history_size;
    viewing_history = (history_size > 0);
    terminal_refresh_screen();
}

// =============================================================================
// Scrolling interne (quand le texte depasse l'ecran)
// =============================================================================

/**
 * Scrolling: Fait remonter tout l'ecran d'une ligne dans le buffer
 */
void terminal_scroll(void) {
    // Avancer la ligne d'ecriture dans le buffer circulaire
    scrollback_write_line = (scrollback_write_line + 1) % SCROLLBACK_LINES;
    scrollback_total_lines++;

    // Initialiser la nouvelle ligne avec des espaces
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        scrollback_buffer[scrollback_write_line][x] = vga_entry(' ', terminal_color);
    }

    // Si on consulte l'historique, ne pas changer la vue
    // Sinon, rafraichir l'ecran
    if (!viewing_history) {
        terminal_refresh_screen();
    }
}

// =============================================================================
// Fonctions Terminal principales
// =============================================================================

void terminal_clear(void) {
    // Reinitialiser le buffer
    for (size_t y = 0; y < SCROLLBACK_LINES; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            scrollback_buffer[y][x] = vga_entry(' ', terminal_color);
        }
    }

    scrollback_write_line = VGA_HEIGHT - 1;
    scrollback_total_lines = VGA_HEIGHT;
    view_line_offset = 0;
    viewing_history = false;
    terminal_row = 0;
    terminal_column = 0;

    terminal_refresh_screen();
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    uint16_t entry = vga_entry(c, color);

    // Ecrire dans le scrollback buffer
    scrollback_putchar_at(x, y, entry);

    // Si on ne consulte pas l'historique, mettre a jour l'ecran directement
    if (!viewing_history && x < SCROLLBAR_COLUMN) {
        vga_buffer[y * VGA_WIDTH + x] = entry;
    }
}

void terminal_newline(void) {
    terminal_column = 0;
    terminal_row++;

    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
}

/**
 * Met a jour la position du curseur materiel clignotant
 * Utilise les ports I/O 0x3D4 (index) et 0x3D5 (data) du controleur VGA
 */
void terminal_update_cursor(size_t x, size_t y) {
    uint16_t pos = y * VGA_WIDTH + x;

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
    outb(0x3D5, (cursor_start & 0x1F));
    outb(0x3D4, 0x0B);
    outb(0x3D5, (cursor_end & 0x1F));
}

void terminal_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void terminal_backspace(void) {
    // Revenir a la vue actuelle si on consulte l'historique
    if (viewing_history) {
        terminal_scroll_to_bottom();
    }

    if (terminal_column > 0) {
        terminal_column--;
    } else if (terminal_row > 0) {
        terminal_row--;
        terminal_column = SCROLLBAR_COLUMN - 1;  // Utiliser la largeur utilisable
    } else {
        return;
    }

    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    terminal_update_cursor(terminal_column, terminal_row);
}

void terminal_putchar(char c) {
    // Revenir a la vue actuelle lors de l'ecriture
    if (viewing_history && c != '\0') {
        terminal_scroll_to_bottom();
    }

    if (c == '\n') {
        // Nouvelle ligne
        terminal_newline();
    } else if (c == '\b') {
        terminal_backspace();
        return;
    } else if (c == '\r') {
        terminal_column = 0;
    } else if (c == '\t') {
        size_t next_tab = (terminal_column + 8) & ~7;
        if (next_tab >= SCROLLBAR_COLUMN) {
            terminal_newline();
        } else {
            while (terminal_column < next_tab) {
                terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
                terminal_column++;
            }
        }
    } else if (c >= 32 && c < 127) {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

        if (++terminal_column >= SCROLLBAR_COLUMN) {
            terminal_newline();
        }
    }

    if (!viewing_history) {
        terminal_update_cursor(terminal_column, terminal_row);
        draw_scrollbar();  // Mettre a jour la scrollbar
    }
}

void terminal_write(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
    }
}

/**
 * Retourne true si on est en mode consultation d'historique
 */
bool terminal_is_viewing_history(void) {
    return viewing_history;
}

/**
 * Retourne l'offset de vue actuel
 */
size_t terminal_get_view_offset(void) {
    return view_line_offset;
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
