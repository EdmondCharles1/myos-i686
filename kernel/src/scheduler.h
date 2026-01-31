/*
 * scheduler.h - Ordonnanceur de processus
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"

// =============================================================================
// Types d'ordonnancement
// =============================================================================

typedef enum {
    SCHEDULER_FCFS,         // First Come First Served (non préemptif)
    SCHEDULER_ROUND_ROBIN,  // Round Robin (préemptif avec quantum)
    SCHEDULER_PRIORITY,     // Par priorité (préemptif)
    SCHEDULER_SJF,          // Shortest Job First (non préemptif)
    SCHEDULER_SRTF,         // Shortest Remaining Time First (préemptif)
    SCHEDULER_MLFQ,         // Multi-Level Feedback Queue
} scheduler_type_t;

// =============================================================================
// File de processus (queue)
// =============================================================================

typedef struct {
    process_t* head;        // Premier processus de la file
    process_t* tail;        // Dernier processus de la file
    uint32_t count;         // Nombre de processus dans la file
} process_queue_t;

// =============================================================================
// Journal d'exécution
// =============================================================================

#define EXEC_LOG_SIZE 100

typedef struct {
    uint32_t pid;           // PID du processus
    char name[16];          // Nom du processus
    uint64_t start_tick;    // Tick de début d'exécution
    uint64_t end_tick;      // Tick de fin d'exécution
    uint32_t duration;      // Durée d'exécution (ticks)
} exec_log_entry_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise l'ordonnanceur
 * 
 * @param type Type d'ordonnancement (FCFS, Round Robin, etc.)
 */
void scheduler_init(scheduler_type_t type);

/**
 * Ajoute un processus à la file READY
 * 
 * @param process Pointeur vers le PCB
 */
void scheduler_add_process(process_t* process);

/**
 * Retire un processus de la file READY
 * 
 * @param process Pointeur vers le PCB
 */
void scheduler_remove_process(process_t* process);

/**
 * Ordonnance le prochain processus (appelé par le timer)
 * Effectue un context switch si nécessaire
 */
void scheduler_schedule(void);

/**
 * Retourne le type d'ordonnancement actuel
 */
scheduler_type_t scheduler_get_type(void);

/**
 * Change le type d'ordonnancement
 */
void scheduler_set_type(scheduler_type_t type);

/**
 * Affiche la file READY
 */
void scheduler_print_queue(void);

/**
 * Affiche le journal d'exécution
 */
void scheduler_print_log(void);

/**
 * Retourne le nom du type d'ordonnancement
 */
const char* scheduler_type_to_string(scheduler_type_t type);

// =============================================================================
// Fonctions de file (queue)
// =============================================================================

/**
 * Initialise une file
 */
void queue_init(process_queue_t* queue);

/**
 * Ajoute un processus à la fin de la file
 */
void queue_enqueue(process_queue_t* queue, process_t* process);

/**
 * Retire et retourne le premier processus de la file
 */
process_t* queue_dequeue(process_queue_t* queue);

/**
 * Retire un processus spécifique de la file
 */
void queue_remove(process_queue_t* queue, process_t* process);

/**
 * Retourne le premier processus sans le retirer
 */
process_t* queue_peek(process_queue_t* queue);

/**
 * Vérifie si la file est vide
 */
bool queue_is_empty(process_queue_t* queue);

// =============================================================================
// MLFQ (Multi-Level Feedback Queue) - Constantes
// =============================================================================

#define MLFQ_LEVELS         3       // Nombre de niveaux dans MLFQ
#define MLFQ_QUANTUM_0      5       // Quantum niveau 0 (haute priorité)
#define MLFQ_QUANTUM_1      10      // Quantum niveau 1 (moyenne priorité)
#define MLFQ_QUANTUM_2      20      // Quantum niveau 2 (basse priorité)
#define MLFQ_BOOST_INTERVAL 500     // Intervalle de boost (ticks)
#define MLFQ_ALLOTMENT      30      // Temps alloué avant demotion

// =============================================================================
// Fonctions avancées de l'ordonnanceur
// =============================================================================

/**
 * Bloque un processus (l'enlève de la file READY)
 */
void scheduler_block_process(process_t* process);

/**
 * Débloque un processus (le remet dans la file READY)
 */
void scheduler_unblock_process(process_t* process);

/**
 * Retourne le processus actuellement en exécution
 */
process_t* scheduler_get_current(void);

/**
 * Affiche les statistiques de l'ordonnanceur
 */
void scheduler_print_stats(void);

/**
 * Effectue un boost MLFQ (remonte tous les processus au niveau 0)
 */
void scheduler_mlfq_boost(void);

/**
 * Simule l'execution des processus et remplit le journal
 * @param ticks Nombre de ticks a simuler
 */
void scheduler_simulate(uint32_t ticks);

#endif // SCHEDULER_H
