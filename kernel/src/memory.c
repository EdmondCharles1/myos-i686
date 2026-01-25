/*
 * memory.c - Gestionnaire de mémoire simple (pool allocator)
 *
 * Implémentation simple avec bitmap pour un mini-OS éducatif.
 * Chaque bloc fait 64 octets, permettant des allocations de 1 à N blocs.
 */

#include "memory.h"
#include "printf.h"

// =============================================================================
// Pool de mémoire
// =============================================================================

// Pool de mémoire statique (alloué dans .bss)
static uint8_t memory_pool[MEMORY_POOL_SIZE] __attribute__((aligned(16)));

// Bitmap des blocs utilisés (1 bit par bloc)
static uint8_t block_bitmap[MAX_BLOCKS / 8 + 1];

// Taille de chaque allocation (pour kfree)
static uint16_t block_sizes[MAX_BLOCKS];

// Statistiques
static uint32_t stat_alloc_count = 0;
static uint32_t stat_free_count = 0;
static uint32_t stat_alloc_failed = 0;
static uint32_t stat_used_blocks = 0;

// =============================================================================
// Fonctions bitmap
// =============================================================================

static inline bool bitmap_get(uint32_t block_idx) {
    return (block_bitmap[block_idx / 8] >> (block_idx % 8)) & 1;
}

static inline void bitmap_set(uint32_t block_idx) {
    block_bitmap[block_idx / 8] |= (1 << (block_idx % 8));
}

static inline void bitmap_clear(uint32_t block_idx) {
    block_bitmap[block_idx / 8] &= ~(1 << (block_idx % 8));
}

// Trouve N blocs consécutifs libres
static int32_t find_free_blocks(uint32_t count) {
    uint32_t consecutive = 0;
    uint32_t start = 0;

    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        if (!bitmap_get(i)) {
            if (consecutive == 0) {
                start = i;
            }
            consecutive++;
            if (consecutive >= count) {
                return (int32_t)start;
            }
        } else {
            consecutive = 0;
        }
    }

    return -1;  // Pas assez de blocs consécutifs
}

// =============================================================================
// Fonctions utilitaires
// =============================================================================

static void memory_zero(void* ptr, uint32_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

// =============================================================================
// Implémentation
// =============================================================================

void memory_init(void) {
    printf("[MEMORY] Initialisation du gestionnaire de memoire...\n");

    // Effacer le bitmap
    for (uint32_t i = 0; i < sizeof(block_bitmap); i++) {
        block_bitmap[i] = 0;
    }

    // Effacer les tailles
    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        block_sizes[i] = 0;
    }

    // Effacer le pool (optionnel mais utile pour debug)
    memory_zero(memory_pool, MEMORY_POOL_SIZE);

    stat_alloc_count = 0;
    stat_free_count = 0;
    stat_alloc_failed = 0;
    stat_used_blocks = 0;

    printf("[MEMORY] Pool: %u KB, Blocs: %u x %u bytes\n",
           MEMORY_POOL_SIZE / 1024, MAX_BLOCKS, MEMORY_BLOCK_SIZE);
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }

    // Calculer le nombre de blocs nécessaires
    uint32_t blocks_needed = (size + MEMORY_BLOCK_SIZE - 1) / MEMORY_BLOCK_SIZE;

    if (blocks_needed > MAX_BLOCKS) {
        stat_alloc_failed++;
        return NULL;
    }

    // Trouver des blocs libres consécutifs
    int32_t start_block = find_free_blocks(blocks_needed);

    if (start_block < 0) {
        stat_alloc_failed++;
        return NULL;
    }

    // Marquer les blocs comme utilisés
    for (uint32_t i = 0; i < blocks_needed; i++) {
        bitmap_set(start_block + i);
    }

    // Sauvegarder le nombre de blocs pour kfree
    block_sizes[start_block] = blocks_needed;

    stat_alloc_count++;
    stat_used_blocks += blocks_needed;

    // Calculer l'adresse
    void* ptr = &memory_pool[start_block * MEMORY_BLOCK_SIZE];

    return ptr;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Vérifier que le pointeur est dans le pool
    if ((uint8_t*)ptr < memory_pool ||
        (uint8_t*)ptr >= memory_pool + MEMORY_POOL_SIZE) {
        printf("[MEMORY] Erreur: kfree sur pointeur invalide!\n");
        return;
    }

    // Calculer l'index du bloc
    uint32_t offset = (uint8_t*)ptr - memory_pool;
    uint32_t block_idx = offset / MEMORY_BLOCK_SIZE;

    // Vérifier l'alignement
    if (offset % MEMORY_BLOCK_SIZE != 0) {
        printf("[MEMORY] Erreur: kfree sur pointeur non-aligne!\n");
        return;
    }

    // Récupérer le nombre de blocs
    uint16_t blocks = block_sizes[block_idx];

    if (blocks == 0) {
        printf("[MEMORY] Erreur: double kfree detecte!\n");
        return;
    }

    // Libérer les blocs
    for (uint32_t i = 0; i < blocks; i++) {
        bitmap_clear(block_idx + i);
    }

    block_sizes[block_idx] = 0;

    stat_free_count++;
    stat_used_blocks -= blocks;
}

void* kcalloc(uint32_t count, uint32_t size) {
    uint32_t total = count * size;
    void* ptr = kmalloc(total);

    if (ptr != NULL) {
        memory_zero(ptr, total);
    }

    return ptr;
}

void memory_get_stats(memory_stats_t* stats) {
    if (stats == NULL) {
        return;
    }

    stats->total_bytes = MEMORY_POOL_SIZE;
    stats->block_size = MEMORY_BLOCK_SIZE;
    stats->total_blocks = MAX_BLOCKS;
    stats->used_blocks = stat_used_blocks;
    stats->free_blocks = MAX_BLOCKS - stat_used_blocks;
    stats->used_bytes = stat_used_blocks * MEMORY_BLOCK_SIZE;
    stats->free_bytes = MEMORY_POOL_SIZE - stats->used_bytes;
    stats->alloc_count = stat_alloc_count;
    stats->free_count = stat_free_count;
    stats->alloc_failed = stat_alloc_failed;
}

void memory_print_stats(void) {
    memory_stats_t stats;
    memory_get_stats(&stats);

    printf("\n=== Statistiques Memoire ===\n\n");
    printf("  Pool total:     %u KB\n", stats.total_bytes / 1024);
    printf("  Utilise:        %u bytes (%u%%)\n",
           stats.used_bytes,
           (stats.used_bytes * 100) / stats.total_bytes);
    printf("  Libre:          %u bytes\n", stats.free_bytes);
    printf("\n");
    printf("  Blocs total:    %u (x%u bytes)\n", stats.total_blocks, stats.block_size);
    printf("  Blocs utilises: %u\n", stats.used_blocks);
    printf("  Blocs libres:   %u\n", stats.free_blocks);
    printf("\n");
    printf("  Allocations:    %u\n", stats.alloc_count);
    printf("  Liberations:    %u\n", stats.free_count);
    printf("  Echecs:         %u\n", stats.alloc_failed);
    printf("\n");
}

void memory_print_map(void) {
    printf("\n=== Carte Memoire (. = libre, # = utilise) ===\n\n");

    // Afficher 64 blocs par ligne
    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        if (i % 64 == 0) {
            printf("%04X: ", i * MEMORY_BLOCK_SIZE);
        }

        printf("%c", bitmap_get(i) ? '#' : '.');

        if ((i + 1) % 64 == 0) {
            printf("\n");
        }
    }

    if (MAX_BLOCKS % 64 != 0) {
        printf("\n");
    }
    printf("\n");
}

void memory_test(void) {
    printf("\n=== Test du gestionnaire de memoire ===\n\n");

    printf("1. Allocation de 100 bytes...\n");
    void* p1 = kmalloc(100);
    printf("   Resultat: %p\n", p1);

    printf("2. Allocation de 500 bytes...\n");
    void* p2 = kmalloc(500);
    printf("   Resultat: %p\n", p2);

    printf("3. Allocation de 1000 bytes...\n");
    void* p3 = kmalloc(1000);
    printf("   Resultat: %p\n", p3);

    printf("\nEtat apres allocations:\n");
    memory_print_stats();

    printf("4. Liberation du 2eme bloc...\n");
    kfree(p2);

    printf("5. Nouvelle allocation de 200 bytes...\n");
    void* p4 = kmalloc(200);
    printf("   Resultat: %p (devrait reutiliser l'espace libere)\n", p4);

    printf("\nEtat final:\n");
    memory_print_stats();

    // Nettoyage
    kfree(p1);
    kfree(p3);
    kfree(p4);

    printf("Tous les blocs liberes.\n\n");
}
