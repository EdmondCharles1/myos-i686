/*
 * sync.c - Primitives de synchronisation (Mutex / Semaphore)
 */

#include "sync.h"
#include "printf.h"
#include "process.h"

// =============================================================================
// Variables globales
// =============================================================================

static mutex_t mutexes[MAX_MUTEXES];
static semaphore_t semaphores[MAX_SEMAPHORES];
static uint32_t next_mutex_id = 1;
static uint32_t next_semaphore_id = 1;

// =============================================================================
// Fonctions utilitaires
// =============================================================================

static void string_copy(char* dest, const char* src, uint32_t max_len) {
    uint32_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Désactive les interruptions (section critique)
static inline void cli(void) {
    __asm__ volatile ("cli");
}

// Réactive les interruptions
static inline void sti(void) {
    __asm__ volatile ("sti");
}

// =============================================================================
// Mutex
// =============================================================================

void sync_init(void) {
    printf("[SYNC] Initialisation des primitives de synchronisation...\n");

    for (int i = 0; i < MAX_MUTEXES; i++) {
        mutexes[i].id = 0;
        mutexes[i].in_use = false;
        mutexes[i].locked = false;
        mutexes[i].owner_pid = 0;
        mutexes[i].lock_count = 0;
        mutexes[i].wait_count = 0;
        mutexes[i].total_locks = 0;
        mutexes[i].total_contentions = 0;
    }

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        semaphores[i].id = 0;
        semaphores[i].in_use = false;
        semaphores[i].value = 0;
        semaphores[i].max_value = 0;
        semaphores[i].wait_count = 0;
        semaphores[i].total_waits = 0;
        semaphores[i].total_signals = 0;
    }

    next_mutex_id = 1;
    next_semaphore_id = 1;

    printf("[SYNC] %d mutex, %d semaphores disponibles\n",
           MAX_MUTEXES, MAX_SEMAPHORES);
}

uint32_t mutex_create(const char* name) {
    if (name == NULL) {
        return 0;
    }

    // Chercher un slot libre
    mutex_t* mtx = NULL;
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (!mutexes[i].in_use) {
            mtx = &mutexes[i];
            break;
        }
    }

    if (mtx == NULL) {
        printf("[SYNC] Erreur: plus de mutex disponible\n");
        return 0;
    }

    mtx->id = next_mutex_id++;
    string_copy(mtx->name, name, MUTEX_NAME_LEN);
    mtx->in_use = true;
    mtx->locked = false;
    mtx->owner_pid = 0;
    mtx->lock_count = 0;
    mtx->wait_count = 0;
    mtx->total_locks = 0;
    mtx->total_contentions = 0;

    printf("[SYNC] Mutex cree: id=%u, nom='%s'\n", mtx->id, mtx->name);

    return mtx->id;
}

bool mutex_destroy(uint32_t id) {
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].in_use && mutexes[i].id == id) {
            if (mutexes[i].locked) {
                printf("[SYNC] Attention: destruction d'un mutex verrouille!\n");
            }
            printf("[SYNC] Mutex detruit: id=%u\n", id);
            mutexes[i].in_use = false;
            mutexes[i].id = 0;
            return true;
        }
    }
    return false;
}

static mutex_t* get_mutex(uint32_t id) {
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].in_use && mutexes[i].id == id) {
            return &mutexes[i];
        }
    }
    return NULL;
}

bool mutex_lock(uint32_t id) {
    mutex_t* mtx = get_mutex(id);
    if (mtx == NULL) {
        return false;
    }

    process_t* current = process_get_current();
    uint32_t current_pid = current ? current->pid : 0;

    cli();  // Section critique

    // Vérifier si c'est un lock récursif
    if (mtx->locked && mtx->owner_pid == current_pid) {
        mtx->lock_count++;
        sti();
        return true;
    }

    // Attendre si verrouillé (busy-wait simplifié)
    while (mtx->locked) {
        mtx->total_contentions++;
        mtx->wait_count++;
        sti();
        // Pause CPU
        __asm__ volatile ("pause");
        cli();
        mtx->wait_count--;
    }

    // Verrouiller
    mtx->locked = true;
    mtx->owner_pid = current_pid;
    mtx->lock_count = 1;
    mtx->total_locks++;

    sti();
    return true;
}

bool mutex_trylock(uint32_t id) {
    mutex_t* mtx = get_mutex(id);
    if (mtx == NULL) {
        return false;
    }

    process_t* current = process_get_current();
    uint32_t current_pid = current ? current->pid : 0;

    cli();

    // Vérifier si c'est un lock récursif
    if (mtx->locked && mtx->owner_pid == current_pid) {
        mtx->lock_count++;
        sti();
        return true;
    }

    // Si verrouillé, échec
    if (mtx->locked) {
        sti();
        return false;
    }

    // Verrouiller
    mtx->locked = true;
    mtx->owner_pid = current_pid;
    mtx->lock_count = 1;
    mtx->total_locks++;

    sti();
    return true;
}

bool mutex_unlock(uint32_t id) {
    mutex_t* mtx = get_mutex(id);
    if (mtx == NULL) {
        return false;
    }

    process_t* current = process_get_current();
    uint32_t current_pid = current ? current->pid : 0;

    cli();

    // Vérifier que c'est bien le propriétaire
    if (!mtx->locked) {
        sti();
        return false;
    }

    if (mtx->owner_pid != current_pid && current_pid != 0) {
        sti();
        printf("[SYNC] Erreur: tentative de unlock par non-proprietaire\n");
        return false;
    }

    // Décrémenter le compteur récursif
    mtx->lock_count--;

    if (mtx->lock_count == 0) {
        mtx->locked = false;
        mtx->owner_pid = 0;
    }

    sti();
    return true;
}

bool mutex_is_locked(uint32_t id) {
    mutex_t* mtx = get_mutex(id);
    if (mtx == NULL) {
        return false;
    }
    return mtx->locked;
}

// =============================================================================
// Semaphore
// =============================================================================

uint32_t semaphore_create(const char* name, int32_t initial_value, int32_t max_value) {
    if (name == NULL) {
        return 0;
    }

    // Chercher un slot libre
    semaphore_t* sem = NULL;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphores[i].in_use) {
            sem = &semaphores[i];
            break;
        }
    }

    if (sem == NULL) {
        printf("[SYNC] Erreur: plus de semaphore disponible\n");
        return 0;
    }

    sem->id = next_semaphore_id++;
    string_copy(sem->name, name, MUTEX_NAME_LEN);
    sem->in_use = true;
    sem->value = initial_value;
    sem->max_value = max_value;
    sem->wait_count = 0;
    sem->total_waits = 0;
    sem->total_signals = 0;

    printf("[SYNC] Semaphore cree: id=%u, nom='%s', valeur=%d\n",
           sem->id, sem->name, sem->value);

    return sem->id;
}

bool semaphore_destroy(uint32_t id) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].in_use && semaphores[i].id == id) {
            printf("[SYNC] Semaphore detruit: id=%u\n", id);
            semaphores[i].in_use = false;
            semaphores[i].id = 0;
            return true;
        }
    }
    return false;
}

static semaphore_t* get_semaphore(uint32_t id) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].in_use && semaphores[i].id == id) {
            return &semaphores[i];
        }
    }
    return NULL;
}

bool semaphore_wait(uint32_t id) {
    semaphore_t* sem = get_semaphore(id);
    if (sem == NULL) {
        return false;
    }

    cli();

    // Attendre si valeur <= 0 (busy-wait simplifié)
    while (sem->value <= 0) {
        sem->wait_count++;
        sti();
        __asm__ volatile ("pause");
        cli();
        sem->wait_count--;
    }

    sem->value--;
    sem->total_waits++;

    sti();
    return true;
}

bool semaphore_trywait(uint32_t id) {
    semaphore_t* sem = get_semaphore(id);
    if (sem == NULL) {
        return false;
    }

    cli();

    if (sem->value <= 0) {
        sti();
        return false;
    }

    sem->value--;
    sem->total_waits++;

    sti();
    return true;
}

bool semaphore_signal(uint32_t id) {
    semaphore_t* sem = get_semaphore(id);
    if (sem == NULL) {
        return false;
    }

    cli();

    // Vérifier la limite max
    if (sem->max_value > 0 && sem->value >= sem->max_value) {
        sti();
        return false;
    }

    sem->value++;
    sem->total_signals++;

    sti();
    return true;
}

int32_t semaphore_get_value(uint32_t id) {
    semaphore_t* sem = get_semaphore(id);
    if (sem == NULL) {
        return -1;
    }
    return sem->value;
}

// =============================================================================
// Affichage
// =============================================================================

void sync_print_mutexes(void) {
    printf("\n=== Mutex ===\n\n");

    bool found = false;
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].in_use) {
            found = true;
            mutex_t* mtx = &mutexes[i];
            printf("  [%u] '%s'\n", mtx->id, mtx->name);
            printf("      Etat: %s", mtx->locked ? "VERROUILLE" : "LIBRE");
            if (mtx->locked) {
                printf(" (owner PID=%u, count=%u)\n",
                       mtx->owner_pid, mtx->lock_count);
            } else {
                printf("\n");
            }
            printf("      Locks: %u, Contentions: %u\n",
                   mtx->total_locks, mtx->total_contentions);
        }
    }

    if (!found) {
        printf("  (aucun mutex)\n");
    }
    printf("\n");
}

void sync_print_semaphores(void) {
    printf("\n=== Semaphores ===\n\n");

    bool found = false;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].in_use) {
            found = true;
            semaphore_t* sem = &semaphores[i];
            printf("  [%u] '%s'\n", sem->id, sem->name);
            printf("      Valeur: %d", sem->value);
            if (sem->max_value > 0) {
                printf(" / %d\n", sem->max_value);
            } else {
                printf(" (illimite)\n");
            }
            printf("      Waits: %u, Signals: %u\n",
                   sem->total_waits, sem->total_signals);
        }
    }

    if (!found) {
        printf("  (aucun semaphore)\n");
    }
    printf("\n");
}

void sync_print_stats(void) {
    printf("\n=== Statistiques Synchronisation ===\n\n");

    uint32_t active_mutex = 0;
    uint32_t total_locks = 0;
    uint32_t total_contentions = 0;

    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].in_use) {
            active_mutex++;
            total_locks += mutexes[i].total_locks;
            total_contentions += mutexes[i].total_contentions;
        }
    }

    uint32_t active_sem = 0;
    uint32_t total_waits = 0;
    uint32_t total_signals = 0;

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].in_use) {
            active_sem++;
            total_waits += semaphores[i].total_waits;
            total_signals += semaphores[i].total_signals;
        }
    }

    printf("  Mutex actifs:     %u/%u\n", active_mutex, MAX_MUTEXES);
    printf("  Total locks:      %u\n", total_locks);
    printf("  Contentions:      %u\n", total_contentions);
    printf("\n");
    printf("  Semaphores actifs: %u/%u\n", active_sem, MAX_SEMAPHORES);
    printf("  Total waits:       %u\n", total_waits);
    printf("  Total signals:     %u\n", total_signals);
    printf("\n");
}
