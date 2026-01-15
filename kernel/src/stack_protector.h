/*
 * stack_protector.h - Stack Smashing Protector
 * 
 * Protège contre les buffer overflows en détectant
 * la corruption de la pile avant le retour d'une fonction.
 * 
 * Inspiré de : https://wiki.osdev.org/Stack_Smashing_Protector
 */

#ifndef STACK_PROTECTOR_H
#define STACK_PROTECTOR_H

#include <stdint.h>

// =============================================================================
// Configuration
// =============================================================================

/**
 * Valeur du canary (garde)
 * 
 * En production, cette valeur devrait être :
 * - Générée aléatoirement au boot
 * - Différente à chaque exécution
 * - Provenant d'une source d'entropie (RDRAND, timer, etc.)
 * 
 * Pour un kernel simple, on utilise une valeur statique.
 */
extern uintptr_t __stack_chk_guard;

// =============================================================================
// API
// =============================================================================

/**
 * Initialise le stack protector
 * 
 * Doit être appelé très tôt dans kernel_main()
 * AVANT toute fonction qui pourrait utiliser des buffers.
 */
void stack_protector_init(void);

/**
 * Handler appelé par GCC quand un stack smashing est détecté
 * 
 * Cette fonction ne doit JAMAIS retourner.
 * Elle affiche une erreur et arrête le kernel.
 */
void __stack_chk_fail(void) __attribute__((noreturn));

/**
 * Alias pour __stack_chk_fail (requis sur certaines versions de GCC)
 */
void __stack_chk_fail_local(void) __attribute__((noreturn));

#endif // STACK_PROTECTOR_H
