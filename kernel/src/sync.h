/*
 * sync.h - Primitives de synchronisation (Mutex / Semaphore)
 */

#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include "types.h"

// =============================================================================
// Configuration
// =============================================================================

#define MAX_MUTEXES     16
#define MAX_SEMAPHORES  16
#define MUTEX_NAME_LEN  16

// =============================================================================
// Mutex (exclusion mutuelle)
// =============================================================================

typedef struct {
    uint32_t id;                    // ID du mutex
    char name[MUTEX_NAME_LEN];      // Nom du mutex
    bool in_use;                    // Mutex actif?
    bool locked;                    // Mutex verrouillé?
    uint32_t owner_pid;             // PID du propriétaire (si verrouillé)
    uint32_t lock_count;            // Compteur pour mutex récursif
    uint32_t wait_count;            // Processus en attente
    uint32_t total_locks;           // Statistique: verrouillages
    uint32_t total_contentions;     // Statistique: contentions
} mutex_t;

// =============================================================================
// Semaphore
// =============================================================================

typedef struct {
    uint32_t id;                    // ID du sémaphore
    char name[MUTEX_NAME_LEN];      // Nom du sémaphore
    bool in_use;                    // Sémaphore actif?
    int32_t value;                  // Valeur courante
    int32_t max_value;              // Valeur maximale
    uint32_t wait_count;            // Processus en attente
    uint32_t total_waits;           // Statistique: attentes
    uint32_t total_signals;         // Statistique: signaux
} semaphore_t;

// =============================================================================
// API Mutex
// =============================================================================

/**
 * Initialise le système de synchronisation
 */
void sync_init(void);

/**
 * Crée un nouveau mutex
 *
 * @param name Nom du mutex
 * @return ID du mutex, ou 0 en cas d'erreur
 */
uint32_t mutex_create(const char* name);

/**
 * Détruit un mutex
 *
 * @param id ID du mutex
 * @return true si succès
 */
bool mutex_destroy(uint32_t id);

/**
 * Verrouille un mutex (bloquant)
 *
 * @param id ID du mutex
 * @return true si succès
 */
bool mutex_lock(uint32_t id);

/**
 * Tente de verrouiller un mutex (non bloquant)
 *
 * @param id ID du mutex
 * @return true si verrouillé, false si déjà pris
 */
bool mutex_trylock(uint32_t id);

/**
 * Déverrouille un mutex
 *
 * @param id ID du mutex
 * @return true si succès
 */
bool mutex_unlock(uint32_t id);

/**
 * Vérifie si un mutex est verrouillé
 *
 * @param id ID du mutex
 * @return true si verrouillé
 */
bool mutex_is_locked(uint32_t id);

// =============================================================================
// API Semaphore
// =============================================================================

/**
 * Crée un nouveau sémaphore
 *
 * @param name Nom du sémaphore
 * @param initial_value Valeur initiale
 * @param max_value Valeur maximale (0 = illimitée)
 * @return ID du sémaphore, ou 0 en cas d'erreur
 */
uint32_t semaphore_create(const char* name, int32_t initial_value, int32_t max_value);

/**
 * Détruit un sémaphore
 *
 * @param id ID du sémaphore
 * @return true si succès
 */
bool semaphore_destroy(uint32_t id);

/**
 * Wait (P) - décrémente le sémaphore
 *
 * @param id ID du sémaphore
 * @return true si succès (non bloquant dans cette implémentation)
 */
bool semaphore_wait(uint32_t id);

/**
 * Try Wait - décrémente si possible (non bloquant)
 *
 * @param id ID du sémaphore
 * @return true si décrementé, false sinon
 */
bool semaphore_trywait(uint32_t id);

/**
 * Signal (V) - incrémente le sémaphore
 *
 * @param id ID du sémaphore
 * @return true si succès
 */
bool semaphore_signal(uint32_t id);

/**
 * Retourne la valeur courante du sémaphore
 *
 * @param id ID du sémaphore
 * @return Valeur courante
 */
int32_t semaphore_get_value(uint32_t id);

// =============================================================================
// Affichage
// =============================================================================

/**
 * Affiche l'état de tous les mutex
 */
void sync_print_mutexes(void);

/**
 * Affiche l'état de tous les sémaphores
 */
void sync_print_semaphores(void);

/**
 * Affiche les statistiques de synchronisation
 */
void sync_print_stats(void);

#endif // SYNC_H
