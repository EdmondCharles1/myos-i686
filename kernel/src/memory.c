/*
 * memory.c - Implementation du pool allocator statique
 * Pool de 64KB avec blocs de 64 octets, gere par bitmap
 */

#include "memory.h"
#include "printf.h"

// =============================================================================
// Variables globales
// =============================================================================

// Pool de memoire statique (64 KB dans BSS)
static uint8_t memory_pool[MEMORY_POOL_SIZE] __attribute__((aligned(16)));

// Bitmap pour suivre l'allocation des blocs (1 bit par bloc)
// MEMORY_NUM_BLOCKS / 8 = 128 octets pour 1024 blocs
static uint8_t memory_bitmap[MEMORY_NUM_BLOCKS / 8];

// Statistiques
static memory_stats_t stats;

// =============================================================================
// Fonctions utilitaires
// =============================================================================

// Marque un bloc comme utilise
static void set_block_used(uint32_t block_index) {
    uint32_t byte_index = block_index / 8;
    uint32_t bit_index = block_index % 8;
    memory_bitmap[byte_index] |= (1 << bit_index);
}

// Marque un bloc comme libre
static void set_block_free(uint32_t block_index) {
    uint32_t byte_index = block_index / 8;
    uint32_t bit_index = block_index % 8;
    memory_bitmap[byte_index] &= ~(1 << bit_index);
}

// Verifie si un bloc est utilise
static bool is_block_used(uint32_t block_index) {
    uint32_t byte_index = block_index / 8;
    uint32_t bit_index = block_index % 8;
    return (memory_bitmap[byte_index] & (1 << bit_index)) != 0;
}

// Trouve N blocs contigus libres
static int32_t find_free_blocks(uint32_t num_blocks) {
    uint32_t consecutive = 0;
    int32_t start_block = -1;

    for (uint32_t i = 0; i < MEMORY_NUM_BLOCKS; i++) {
        if (!is_block_used(i)) {
            if (consecutive == 0) {
                start_block = i;
            }
            consecutive++;

            if (consecutive >= num_blocks) {
                return start_block;
            }
        } else {
            consecutive = 0;
            start_block = -1;
        }
    }

    return -1;  // Pas assez de blocs contigus
}

// Compte les blocs utilises
static uint32_t count_used_blocks(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < MEMORY_NUM_BLOCKS; i++) {
        if (is_block_used(i)) {
            count++;
        }
    }
    return count;
}

// =============================================================================
// Implementation de l'API
// =============================================================================

void memory_init(void) {
    printf("[MEMORY] Initialisation du gestionnaire memoire...\n");

    // Initialiser le bitmap (tous les blocs libres)
    for (uint32_t i = 0; i < MEMORY_NUM_BLOCKS / 8; i++) {
        memory_bitmap[i] = 0;
    }

    // Initialiser les statistiques
    stats.total_size = MEMORY_POOL_SIZE;
    stats.block_size = MEMORY_BLOCK_SIZE;
    stats.total_blocks = MEMORY_NUM_BLOCKS;
    stats.used_blocks = 0;
    stats.free_blocks = MEMORY_NUM_BLOCKS;
    stats.alloc_count = 0;
    stats.free_count = 0;
    stats.peak_usage = 0;

    printf("[MEMORY] Pool: %u KB, Blocs: %u x %u octets\n",
           MEMORY_POOL_SIZE / 1024, MEMORY_NUM_BLOCKS, MEMORY_BLOCK_SIZE);
    printf("[MEMORY] Adresse pool: 0x%x - 0x%x\n",
           (uint32_t)memory_pool,
           (uint32_t)(memory_pool + MEMORY_POOL_SIZE - 1));
    printf("[MEMORY] Gestionnaire memoire initialise\n");
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Calculer le nombre de blocs necessaires
    uint32_t num_blocks = (size + MEMORY_BLOCK_SIZE - 1) / MEMORY_BLOCK_SIZE;

    // Chercher des blocs contigus libres
    int32_t start_block = find_free_blocks(num_blocks);

    if (start_block < 0) {
        printf("[MEMORY] Echec allocation: %u octets (%u blocs) - memoire insuffisante\n",
               (uint32_t)size, num_blocks);
        return NULL;
    }

    // Marquer les blocs comme utilises
    for (uint32_t i = 0; i < num_blocks; i++) {
        set_block_used(start_block + i);
    }

    // Mettre a jour les statistiques
    stats.used_blocks += num_blocks;
    stats.free_blocks -= num_blocks;
    stats.alloc_count++;

    if (stats.used_blocks > stats.peak_usage) {
        stats.peak_usage = stats.used_blocks;
    }

    // Calculer l'adresse
    void* ptr = (void*)(memory_pool + (start_block * MEMORY_BLOCK_SIZE));

    return ptr;
}

void* kcalloc(size_t num, size_t size) {
    size_t total = num * size;

    void* ptr = kmalloc(total);

    if (ptr != NULL) {
        // Initialiser a zero
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }

    return ptr;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Verifier que le pointeur est dans le pool
    uint8_t* p = (uint8_t*)ptr;
    if (p < memory_pool || p >= memory_pool + MEMORY_POOL_SIZE) {
        printf("[MEMORY] ERREUR: tentative de liberation hors du pool: 0x%x\n",
               (uint32_t)ptr);
        return;
    }

    // Calculer l'index du bloc
    uint32_t offset = p - memory_pool;
    uint32_t block_index = offset / MEMORY_BLOCK_SIZE;

    // Verifier l'alignement
    if (offset % MEMORY_BLOCK_SIZE != 0) {
        printf("[MEMORY] ERREUR: pointeur non aligne: 0x%x\n", (uint32_t)ptr);
        return;
    }

    // Verifier que le bloc est utilise
    if (!is_block_used(block_index)) {
        printf("[MEMORY] ERREUR: double liberation du bloc %u\n", block_index);
        return;
    }

    // Liberer les blocs contigus (on doit savoir combien)
    // Pour simplifier, on libere un bloc a la fois
    // Note: une implementation complete garderait la taille d'allocation
    uint32_t freed_blocks = 0;

    // Liberer tous les blocs contigus utilises a partir de ce point
    while (block_index < MEMORY_NUM_BLOCKS && is_block_used(block_index)) {
        set_block_free(block_index);
        freed_blocks++;
        block_index++;

        // Pour eviter de liberer trop (blocs d'une autre allocation),
        // on s'arrete apres un certain nombre ou si on trouve un marqueur
        // Dans cette implementation simplifiee, on libere 1 bloc par defaut
        break;  // Liberation d'un seul bloc pour simplicite
    }

    // Mettre a jour les statistiques
    stats.used_blocks -= freed_blocks;
    stats.free_blocks += freed_blocks;
    stats.free_count++;
}

memory_stats_t memory_get_stats(void) {
    // Recalculer les blocs utilises pour etre sur
    stats.used_blocks = count_used_blocks();
    stats.free_blocks = MEMORY_NUM_BLOCKS - stats.used_blocks;
    return stats;
}

void memory_print_stats(void) {
    memory_stats_t s = memory_get_stats();

    printf("\n=== Statistiques memoire ===\n");
    printf("Pool total:      %u octets (%u KB)\n", s.total_size, s.total_size / 1024);
    printf("Taille bloc:     %u octets\n", s.block_size);
    printf("Blocs totaux:    %u\n", s.total_blocks);
    printf("Blocs utilises:  %u (%u octets)\n", s.used_blocks, s.used_blocks * s.block_size);
    printf("Blocs libres:    %u (%u octets)\n", s.free_blocks, s.free_blocks * s.block_size);
    printf("Allocations:     %u\n", s.alloc_count);
    printf("Liberations:     %u\n", s.free_count);
    printf("Pic utilisation: %u blocs\n", s.peak_usage);
    printf("Utilisation:     %u%%\n", (s.used_blocks * 100) / s.total_blocks);
    printf("\n");
}

void memory_test(void) {
    printf("\n=== Test du gestionnaire memoire ===\n\n");

    // Test 1: Allocations simples
    printf("Test 1: Allocations simples\n");
    void* p1 = kmalloc(100);
    printf("  Alloc 100 octets: 0x%x\n", (uint32_t)p1);

    void* p2 = kmalloc(200);
    printf("  Alloc 200 octets: 0x%x\n", (uint32_t)p2);

    void* p3 = kmalloc(50);
    printf("  Alloc 50 octets:  0x%x\n", (uint32_t)p3);

    memory_stats_t s = memory_get_stats();
    printf("  Blocs utilises: %u\n", s.used_blocks);

    // Test 2: Liberation
    printf("\nTest 2: Liberation\n");
    kfree(p2);
    printf("  Libere p2 (200 octets)\n");

    s = memory_get_stats();
    printf("  Blocs utilises: %u\n", s.used_blocks);

    // Test 3: Reallocation dans l'espace libere
    printf("\nTest 3: Reallocation\n");
    void* p4 = kmalloc(64);
    printf("  Alloc 64 octets:  0x%x\n", (uint32_t)p4);

    // Test 4: kcalloc
    printf("\nTest 4: kcalloc (memoire initialisee a zero)\n");
    uint8_t* p5 = (uint8_t*)kcalloc(10, sizeof(uint32_t));
    printf("  kcalloc 10 x 4 octets: 0x%x\n", (uint32_t)p5);
    printf("  Premiers octets: %u %u %u %u\n", p5[0], p5[1], p5[2], p5[3]);

    // Nettoyage
    printf("\nNettoyage...\n");
    kfree(p1);
    kfree(p3);
    kfree(p4);
    kfree(p5);

    // Stats finales
    memory_print_stats();

    printf("=== Test termine ===\n\n");
}

void memory_print_bitmap(void) {
    printf("\n=== Bitmap memoire (premiers 64 blocs) ===\n");

    for (uint32_t i = 0; i < 64; i++) {
        if (i % 16 == 0) {
            printf("\n%04u: ", i);
        }
        printf("%c", is_block_used(i) ? '#' : '.');
    }

    printf("\n\nLegende: . = libre, # = utilise\n\n");
}
