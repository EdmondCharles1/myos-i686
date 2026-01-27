/*
 * memory.h - Gestion memoire (pool allocator statique)
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "process.h"

// =============================================================================
// Configuration du pool memoire
// =============================================================================

#define MEMORY_POOL_SIZE    (64 * 1024)     // 64 KB de pool
#define MEMORY_BLOCK_SIZE   64              // Taille d'un bloc (64 octets)
#define MEMORY_NUM_BLOCKS   (MEMORY_POOL_SIZE / MEMORY_BLOCK_SIZE)  // 1024 blocs

// =============================================================================
// Structures
// =============================================================================

// Statistiques memoire
typedef struct {
    uint32_t total_size;        // Taille totale du pool
    uint32_t block_size;        // Taille d'un bloc
    uint32_t total_blocks;      // Nombre total de blocs
    uint32_t used_blocks;       // Blocs utilises
    uint32_t free_blocks;       // Blocs libres
    uint32_t alloc_count;       // Nombre d'allocations
    uint32_t free_count;        // Nombre de liberations
    uint32_t peak_usage;        // Pic d'utilisation (blocs)
} memory_stats_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le gestionnaire de memoire
 */
void memory_init(void);

/**
 * Alloue de la memoire
 *
 * @param size Taille demandee en octets
 * @return Pointeur vers la zone allouee, ou NULL si echec
 */
void* kmalloc(size_t size);

/**
 * Alloue et initialise a zero
 *
 * @param num   Nombre d'elements
 * @param size  Taille de chaque element
 * @return Pointeur vers la zone allouee, ou NULL si echec
 */
void* kcalloc(size_t num, size_t size);

/**
 * Libere de la memoire
 *
 * @param ptr Pointeur vers la zone a liberer
 */
void kfree(void* ptr);

/**
 * Retourne les statistiques memoire
 *
 * @return Structure contenant les stats
 */
memory_stats_t memory_get_stats(void);

/**
 * Affiche les statistiques memoire
 */
void memory_print_stats(void);

/**
 * Test du gestionnaire de memoire
 * Effectue des allocations/liberations et affiche les resultats
 */
void memory_test(void);

/**
 * Affiche l'etat du bitmap (pour debug)
 */
void memory_print_bitmap(void);

#endif // MEMORY_H
