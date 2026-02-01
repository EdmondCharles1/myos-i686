/*
 * sync.h - Primitives de synchronisation (Mutex et Semaphores)
 */

#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include <stddef.h>
#include "process.h"

// =============================================================================
// Configuration
// =============================================================================

#define MAX_MUTEXES         16          // Nombre max de mutex
#define MAX_SEMAPHORES      16          // Nombre max de semaphores
#define SYNC_NAME_LEN       16          // Longueur max du nom
#define SYNC_MAX_WAITERS    8           // Processus max en attente

// =============================================================================
// Codes de retour
// =============================================================================

#define SYNC_SUCCESS        0
#define SYNC_ERROR_BUSY    -1           // Mutex deja verrouille (trylock)
#define SYNC_ERROR_NOTFOUND -2          // Mutex/Semaphore inexistant
#define SYNC_ERROR_NOTOWNER -3          // Pas proprietaire du mutex
#define SYNC_ERROR_NOSLOT  -4           // Plus de slot disponible
#define SYNC_ERROR_PARAM   -5           // Parametre invalide
#define SYNC_ERROR_EXISTS  -6           // Existe deja
#define SYNC_ERROR_WOULDBLOCK -7        // Bloquerait (sem_trywait)

// =============================================================================
// Structures - Mutex
// =============================================================================

typedef struct {
    uint32_t id;                        // ID unique
    char name[SYNC_NAME_LEN];           // Nom du mutex
    bool active;                        // Mutex actif/valide

    bool locked;                        // Etat verouille
    uint32_t owner_pid;                 // PID du proprietaire

    // File d'attente des processus bloques
    process_t* waiters[SYNC_MAX_WAITERS];
    uint32_t waiter_count;

    // Statistiques
    uint32_t lock_count;                // Nombre de verrouillages
    uint32_t contention_count;          // Nombre de contentions
} mutex_t;

// =============================================================================
// Structures - Semaphore
// =============================================================================

typedef struct {
    uint32_t id;                        // ID unique
    char name[SYNC_NAME_LEN];           // Nom du semaphore
    bool active;                        // Semaphore actif/valide

    int32_t value;                      // Valeur courante
    int32_t initial_value;              // Valeur initiale

    // File d'attente des processus bloques
    process_t* waiters[SYNC_MAX_WAITERS];
    uint32_t waiter_count;

    // Statistiques
    uint32_t wait_count;                // Nombre de wait
    uint32_t post_count;                // Nombre de post
} semaphore_t;

// =============================================================================
// API Mutex
// =============================================================================

/**
 * Initialise le systeme de synchronisation
 */
void sync_init(void);

/**
 * Cree un nouveau mutex
 *
 * @param name Nom du mutex
 * @return ID du mutex (>= 0) ou code erreur (< 0)
 */
int32_t mutex_create(const char* name);

/**
 * Detruit un mutex
 *
 * @param id ID du mutex
 * @return SYNC_SUCCESS ou code erreur
 */
int32_t mutex_destroy(uint32_t id);

/**
 * Trouve un mutex par son nom
 *
 * @param name Nom du mutex
 * @return ID du mutex ou SYNC_ERROR_NOTFOUND
 */
int32_t mutex_find(const char* name);

/**
 * Verrouille un mutex (bloquant)
 * Bloque le processus si le mutex est deja pris
 *
 * @param id ID du mutex
 * @return SYNC_SUCCESS ou code erreur
 */
int32_t mutex_lock(uint32_t id);

/**
 * Tente de verrouiller un mutex (non bloquant)
 *
 * @param id ID du mutex
 * @return SYNC_SUCCESS, SYNC_ERROR_BUSY, ou autre erreur
 */
int32_t mutex_trylock(uint32_t id);

/**
 * Deverrouille un mutex
 *
 * @param id ID du mutex
 * @return SYNC_SUCCESS ou code erreur
 */
int32_t mutex_unlock(uint32_t id);

/**
 * Affiche l'etat de tous les mutex
 */
void mutex_print_all(void);

// =============================================================================
// API Semaphore
// =============================================================================

/**
 * Cree un nouveau semaphore
 *
 * @param name  Nom du semaphore
 * @param value Valeur initiale
 * @return ID du semaphore (>= 0) ou code erreur (< 0)
 */
int32_t sem_create(const char* name, int32_t value);

/**
 * Detruit un semaphore
 *
 * @param id ID du semaphore
 * @return SYNC_SUCCESS ou code erreur
 */
int32_t sem_destroy(uint32_t id);

/**
 * Trouve un semaphore par son nom
 *
 * @param name Nom du semaphore
 * @return ID du semaphore ou SYNC_ERROR_NOTFOUND
 */
int32_t sem_find(const char* name);

/**
 * Decremente le semaphore (P / wait)
 * Bloque si la valeur est <= 0
 *
 * @param id ID du semaphore
 * @return SYNC_SUCCESS ou code erreur
 */
int32_t sem_wait(uint32_t id);

/**
 * Tente de decrementer le semaphore (non bloquant)
 *
 * @param id ID du semaphore
 * @return SYNC_SUCCESS, SYNC_ERROR_WOULDBLOCK, ou autre erreur
 */
int32_t sem_trywait(uint32_t id);

/**
 * Incremente le semaphore (V / post)
 *
 * @param id ID du semaphore
 * @return SYNC_SUCCESS ou code erreur
 */
int32_t sem_post(uint32_t id);

/**
 * Retourne la valeur courante du semaphore
 *
 * @param id ID du semaphore
 * @return Valeur ou code erreur si < 0
 */
int32_t sem_getvalue(uint32_t id);

/**
 * Affiche l'etat de tous les semaphores
 */
void sem_print_all(void);

// =============================================================================
// Tests
// =============================================================================

/**
 * Test des mutex
 */
void mutex_test(void);

/**
 * Demonstration du blocage mutex avec processus
 */
void mutex_demo(void);

/**
 * Test des semaphores
 */
void sem_test(void);

/**
 * Test complet de la synchronisation
 */
void sync_test(void);

#endif // SYNC_H
