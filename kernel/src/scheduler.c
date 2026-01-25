/*
 * scheduler.c - Ordonnanceur multi-algorithmes
 *
 * Algorithmes supportés :
 * - FCFS (First Come First Served)
 * - Round Robin (avec quantum configurable)
 * - Priority (préemptif par priorité)
 * - SJF (Shortest Job First) - estimé
 * - SRTF (Shortest Remaining Time First) - estimé
 * - MLFQ (Multi-Level Feedback Queue)
 */

#include "scheduler.h"
#include "printf.h"
#include "timer.h"
#include "types.h"

// =============================================================================
// Variables globales
// =============================================================================

// Type d'ordonnancement actuel
static scheduler_type_t current_scheduler = SCHEDULER_ROUND_ROBIN;

// File de processus READY
static process_queue_t ready_queue;

// Files pour MLFQ (3 niveaux)
#define MLFQ_LEVELS 3
static process_queue_t mlfq_queues[MLFQ_LEVELS];
static uint32_t mlfq_quantums[MLFQ_LEVELS] = {2, 4, 8};  // Quantum croissant

// Processus actuellement en cours d'exécution
static process_t* current_process = NULL;

// Journal d'exécution
static exec_log_entry_t exec_log[EXEC_LOG_SIZE];
static uint32_t exec_log_index = 0;
static uint32_t exec_log_count = 0;  // Nombre total d'entrées (pour wrap)

// Statistiques
static uint64_t total_context_switches = 0;

// =============================================================================
// Fonctions de file (queue)
// =============================================================================

void queue_init(process_queue_t* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
}

void queue_enqueue(process_queue_t* queue, process_t* process) {
    if (process == NULL) {
        return;
    }

    process->next = NULL;

    if (queue->tail == NULL) {
        queue->head = process;
        queue->tail = process;
    } else {
        queue->tail->next = process;
        queue->tail = process;
    }

    queue->count++;
}

process_t* queue_dequeue(process_queue_t* queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    process_t* process = queue->head;
    queue->head = process->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    process->next = NULL;
    queue->count--;

    return process;
}

void queue_remove(process_queue_t* queue, process_t* process) {
    if (queue->head == NULL || process == NULL) {
        return;
    }

    if (queue->head == process) {
        queue_dequeue(queue);
        return;
    }

    process_t* current = queue->head;
    while (current->next != NULL && current->next != process) {
        current = current->next;
    }

    if (current->next == process) {
        current->next = process->next;

        if (queue->tail == process) {
            queue->tail = current;
        }

        process->next = NULL;
        queue->count--;
    }
}

process_t* queue_peek(process_queue_t* queue) {
    return queue->head;
}

bool queue_is_empty(process_queue_t* queue) {
    return queue->head == NULL;
}

// =============================================================================
// Fonctions d'ordonnancement spécialisées
// =============================================================================

// Trouve le processus avec la plus haute priorité dans la queue
static process_t* find_highest_priority(process_queue_t* queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    process_t* best = queue->head;
    process_t* current = queue->head->next;

    while (current != NULL) {
        if (current->priority > best->priority) {
            best = current;
        }
        current = current->next;
    }

    return best;
}

// Retire le processus avec la plus haute priorité
static process_t* dequeue_highest_priority(process_queue_t* queue) {
    process_t* best = find_highest_priority(queue);
    if (best != NULL) {
        queue_remove(queue, best);
    }
    return best;
}

// Trouve le processus avec le plus court temps estimé (pour SJF)
static process_t* find_shortest_job(process_queue_t* queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    process_t* best = queue->head;
    process_t* current = queue->head->next;

    while (current != NULL) {
        // Utilise time_slice comme estimation du temps restant
        if (current->time_slice < best->time_slice) {
            best = current;
        }
        current = current->next;
    }

    return best;
}

// Retire le processus avec le plus court temps estimé
static process_t* dequeue_shortest_job(process_queue_t* queue) {
    process_t* best = find_shortest_job(queue);
    if (best != NULL) {
        queue_remove(queue, best);
    }
    return best;
}

// Trouve le processus avec le plus court temps restant (pour SRTF)
static process_t* find_shortest_remaining(process_queue_t* queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    process_t* best = queue->head;
    process_t* current = queue->head->next;

    while (current != NULL) {
        if (current->remaining_slice < best->remaining_slice) {
            best = current;
        }
        current = current->next;
    }

    return best;
}

// =============================================================================
// Journal d'exécution
// =============================================================================

static void log_execution(process_t* process, uint64_t start, uint64_t end) {
    exec_log_entry_t* entry = &exec_log[exec_log_index];

    entry->pid = process->pid;

    // Copier le nom
    int i;
    for (i = 0; i < 15 && process->name[i] != '\0'; i++) {
        entry->name[i] = process->name[i];
    }
    entry->name[i] = '\0';

    entry->start_tick = start;
    entry->end_tick = end;
    entry->duration = (uint32_t)(end - start);

    exec_log_index = (exec_log_index + 1) % EXEC_LOG_SIZE;
    if (exec_log_count < EXEC_LOG_SIZE) {
        exec_log_count++;
    }
}

// =============================================================================
// Sélection du prochain processus
// =============================================================================

static process_t* select_next_process(void) {
    switch (current_scheduler) {
        case SCHEDULER_FCFS:
        case SCHEDULER_ROUND_ROBIN:
            // FIFO simple
            return queue_dequeue(&ready_queue);

        case SCHEDULER_PRIORITY:
            // Plus haute priorité d'abord
            return dequeue_highest_priority(&ready_queue);

        case SCHEDULER_SJF:
            // Plus court job d'abord
            return dequeue_shortest_job(&ready_queue);

        case SCHEDULER_SRTF:
            // Plus court temps restant d'abord
            {
                process_t* best = find_shortest_remaining(&ready_queue);
                if (best != NULL) {
                    queue_remove(&ready_queue, best);
                }
                return best;
            }

        case SCHEDULER_MLFQ:
            // Multi-Level Feedback Queue
            for (int level = 0; level < MLFQ_LEVELS; level++) {
                if (!queue_is_empty(&mlfq_queues[level])) {
                    return queue_dequeue(&mlfq_queues[level]);
                }
            }
            return NULL;

        default:
            return queue_dequeue(&ready_queue);
    }
}

// =============================================================================
// Ordonnanceur principal
// =============================================================================

void scheduler_init(scheduler_type_t type) {
    printf("[SCHEDULER] Initialisation de l'ordonnanceur...\n");

    current_scheduler = type;
    queue_init(&ready_queue);
    current_process = NULL;
    exec_log_index = 0;
    exec_log_count = 0;
    total_context_switches = 0;

    // Initialiser les files MLFQ
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        queue_init(&mlfq_queues[i]);
    }

    printf("[SCHEDULER] Type: %s\n", scheduler_type_to_string(type));
    printf("[SCHEDULER] Ordonnanceur initialise\n");
}

void scheduler_add_process(process_t* process) {
    if (process == NULL) {
        return;
    }

    // Mettre le processus en état READY
    process->state = PROCESS_STATE_READY;

    // Pour MLFQ, les nouveaux processus vont dans la file haute priorité
    if (current_scheduler == SCHEDULER_MLFQ) {
        queue_enqueue(&mlfq_queues[0], process);
    } else {
        queue_enqueue(&ready_queue, process);
    }

    // Log silencieux (trop verbeux sinon)
    // printf("[SCHEDULER] Processus PID=%u ajoute a la file READY\n", process->pid);
}

void scheduler_remove_process(process_t* process) {
    if (process == NULL) {
        return;
    }

    // Retirer de toutes les files possibles
    queue_remove(&ready_queue, process);
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        queue_remove(&mlfq_queues[i], process);
    }
}

void scheduler_schedule(void) {
    // Si pas de processus courant, en choisir un
    if (current_process == NULL) {
        current_process = select_next_process();

        if (current_process != NULL) {
            current_process->state = PROCESS_STATE_RUNNING;
            current_process->remaining_slice = current_process->time_slice;
            current_process->start_tick = timer_get_ticks();
        }
        return;
    }

    // Incrémenter les ticks du processus courant
    current_process->total_ticks++;
    current_process->remaining_slice--;

    // Déterminer si un context switch est nécessaire
    bool should_switch = false;
    int mlfq_level = 0;  // Niveau MLFQ pour réenqueue

    switch (current_scheduler) {
        case SCHEDULER_FCFS:
            // FCFS : pas de préemption
            should_switch = false;
            break;

        case SCHEDULER_ROUND_ROBIN:
            // Round Robin : changer si quantum épuisé
            if (current_process->remaining_slice == 0) {
                should_switch = true;
            }
            break;

        case SCHEDULER_PRIORITY:
            // Priority : changer si quantum épuisé OU si un processus plus prioritaire arrive
            if (current_process->remaining_slice == 0) {
                should_switch = true;
            } else {
                // Vérifier si un processus plus prioritaire est prêt
                process_t* highest = find_highest_priority(&ready_queue);
                if (highest != NULL && highest->priority > current_process->priority) {
                    should_switch = true;
                }
            }
            break;

        case SCHEDULER_SJF:
            // SJF : pas de préemption (non-préemptif)
            should_switch = false;
            break;

        case SCHEDULER_SRTF:
            // SRTF : préemption si un processus avec temps restant plus court arrive
            {
                process_t* shortest = find_shortest_remaining(&ready_queue);
                if (shortest != NULL &&
                    shortest->remaining_slice < current_process->remaining_slice) {
                    should_switch = true;
                }
            }
            break;

        case SCHEDULER_MLFQ:
            // MLFQ : quantum épuisé = descendre d'un niveau
            if (current_process->remaining_slice == 0) {
                should_switch = true;
                // Calculer le niveau pour réenqueue (descendre d'un cran)
                mlfq_level = 1;  // Par défaut niveau 1
                // On pourrait tracker le niveau actuel dans le PCB
            }
            break;
    }

    // Effectuer le context switch si nécessaire
    bool has_waiting = !queue_is_empty(&ready_queue);

    // Pour MLFQ, vérifier toutes les files
    if (current_scheduler == SCHEDULER_MLFQ) {
        has_waiting = false;
        for (int i = 0; i < MLFQ_LEVELS; i++) {
            if (!queue_is_empty(&mlfq_queues[i])) {
                has_waiting = true;
                break;
            }
        }
    }

    if (should_switch && has_waiting) {
        // Sauvegarder le contexte du processus courant
        process_t* old_process = current_process;
        uint64_t old_start = old_process->start_tick;

        // Logger l'exécution
        log_execution(old_process, old_start, timer_get_ticks());

        // Remettre le processus courant dans la file READY
        old_process->state = PROCESS_STATE_READY;

        if (current_scheduler == SCHEDULER_MLFQ) {
            // MLFQ : descendre d'un niveau (max = dernier niveau)
            if (mlfq_level >= MLFQ_LEVELS) {
                mlfq_level = MLFQ_LEVELS - 1;
            }
            queue_enqueue(&mlfq_queues[mlfq_level], old_process);
            // Mettre à jour le quantum pour le nouveau niveau
            old_process->time_slice = mlfq_quantums[mlfq_level];
        } else {
            queue_enqueue(&ready_queue, old_process);
        }

        // Choisir le prochain processus
        current_process = select_next_process();
        if (current_process != NULL) {
            current_process->state = PROCESS_STATE_RUNNING;
            current_process->remaining_slice = current_process->time_slice;
            current_process->start_tick = timer_get_ticks();
        }

        total_context_switches++;
    }
}

scheduler_type_t scheduler_get_type(void) {
    return current_scheduler;
}

void scheduler_set_type(scheduler_type_t type) {
    // Réinitialiser les files lors du changement de type
    if (current_scheduler == SCHEDULER_MLFQ || type == SCHEDULER_MLFQ) {
        // Déplacer tous les processus vers la file standard ou MLFQ
        // Pour simplifier, on ne fait rien de spécial
    }

    current_scheduler = type;
    printf("[SCHEDULER] Type change: %s\n", scheduler_type_to_string(type));
}

void scheduler_print_queue(void) {
    printf("\n=== File READY ===\n");
    printf("Ordonnanceur: %s\n\n", scheduler_type_to_string(current_scheduler));

    if (current_scheduler == SCHEDULER_MLFQ) {
        // Afficher les files MLFQ
        for (int level = 0; level < MLFQ_LEVELS; level++) {
            printf("Niveau %d (quantum=%u ticks):\n", level, mlfq_quantums[level]);
            if (queue_is_empty(&mlfq_queues[level])) {
                printf("  (vide)\n");
            } else {
                process_t* p = mlfq_queues[level].head;
                int pos = 1;
                while (p != NULL) {
                    printf("  %d. PID=%u '%s' prio=%u\n",
                           pos++, p->pid, p->name, p->priority);
                    p = p->next;
                }
            }
        }
    } else {
        if (queue_is_empty(&ready_queue)) {
            printf("(vide)\n");
        } else {
            printf("Processus dans la file (%u):\n", ready_queue.count);
            process_t* p = ready_queue.head;
            int pos = 1;
            while (p != NULL) {
                printf("  %d. PID=%u '%s' prio=%u slice=%u/%u\n",
                       pos++, p->pid, p->name, p->priority,
                       p->remaining_slice, p->time_slice);
                p = p->next;
            }
        }
    }
    printf("\n");
}

void scheduler_print_log(void) {
    printf("\n=== Journal d'execution ===\n");
    printf("Ordonnanceur: %s\n", scheduler_type_to_string(current_scheduler));
    printf("Entries: %u (max %u)\n\n", exec_log_count, EXEC_LOG_SIZE);

    if (exec_log_count == 0) {
        printf("(aucune entree)\n\n");
        return;
    }

    printf("PID  | Nom              | Debut  | Fin    | Duree\n");
    printf("-----|------------------|--------|--------|------\n");

    // Afficher les entrées dans l'ordre chronologique
    uint32_t start_idx = 0;
    uint32_t count = exec_log_count;

    if (exec_log_count >= EXEC_LOG_SIZE) {
        // Le journal a wrap, commencer à exec_log_index
        start_idx = exec_log_index;
        count = EXEC_LOG_SIZE;
    }

    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (start_idx + i) % EXEC_LOG_SIZE;
        exec_log_entry_t* entry = &exec_log[idx];
        printf("%-4u | %-16s | %-6u | %-6u | %u\n",
               entry->pid,
               entry->name,
               (uint32_t)entry->start_tick,
               (uint32_t)entry->end_tick,
               entry->duration);
    }

    printf("\nContext switches: %u\n\n", (uint32_t)total_context_switches);
}

const char* scheduler_type_to_string(scheduler_type_t type) {
    switch (type) {
        case SCHEDULER_FCFS:        return "FCFS";
        case SCHEDULER_ROUND_ROBIN: return "Round Robin";
        case SCHEDULER_PRIORITY:    return "Priority";
        case SCHEDULER_SJF:         return "SJF (Shortest Job First)";
        case SCHEDULER_SRTF:        return "SRTF (Shortest Remaining Time)";
        case SCHEDULER_MLFQ:        return "MLFQ (Multi-Level Feedback)";
        default:                    return "Unknown";
    }
}
