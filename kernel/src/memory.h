/*
 * memory.h - Gestionnaire de mémoire simple (pool allocator)
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "types.h"

// =============================================================================
// Configuration
// =============================================================================

#define MEMORY_POOL_SIZE    (64 * 1024)     // 64 KB de pool
#define MEMORY_BLOCK_SIZE   64              // Taille d'un bloc (64 bytes)
#define MAX_BLOCKS          (MEMORY_POOL_SIZE / MEMORY_BLOCK_SIZE)

// =============================================================================
// Statistiques
// =============================================================================

typedef struct {
    uint32_t total_bytes;           // Taille totale du pool
    uint32_t used_bytes;            // Octets utilisés
    uint32_t free_bytes;            // Octets libres
    uint32_t block_size;            // Taille d'un bloc
    uint32_t total_blocks;          // Nombre total de blocs
    uint32_t used_blocks;           // Blocs utilisés
    uint32_t free_blocks;           // Blocs libres
    uint32_t alloc_count;           // Nombre d'allocations
    uint32_t free_count;            // Nombre de libérations
    uint32_t alloc_failed;          // Allocations échouées
} memory_stats_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le gestionnaire de mémoire
 */
void memory_init(void);

/**
 * Alloue un bloc de mémoire
 *
 * @param size Taille demandée en octets
 * @return Pointeur vers la mémoire allouée, ou NULL si échec
 */
void* kmalloc(uint32_t size);

/**
 * Libère un bloc de mémoire
 *
 * @param ptr Pointeur vers la mémoire à libérer
 */
void kfree(void* ptr);

/**
 * Alloue et initialise à zéro
 *
 * @param count Nombre d'éléments
 * @param size Taille de chaque élément
 * @return Pointeur vers la mémoire allouée, ou NULL si échec
 */
void* kcalloc(uint32_t count, uint32_t size);

/**
 * Retourne les statistiques de mémoire
 *
 * @param stats Pointeur vers la structure à remplir
 */
void memory_get_stats(memory_stats_t* stats);

/**
 * Affiche les statistiques de mémoire
 */
void memory_print_stats(void);

/**
 * Affiche la carte de la mémoire (debug)
 */
void memory_print_map(void);

/**
 * Teste le gestionnaire de mémoire
 */
void memory_test(void);

#endif // MEMORY_H
