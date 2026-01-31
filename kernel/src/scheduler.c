/*
 * scheduler.c - Implementation de l'ordonnanceur
 * Supporte: FCFS, Round Robin, Priority, SJF, SRTF, MLFQ
 */

#include "scheduler.h"
#include "printf.h"
#include "timer.h"

// =============================================================================
// Variables globales
// =============================================================================

// Type d'ordonnancement actuel
static scheduler_type_t current_scheduler = SCHEDULER_ROUND_ROBIN;

// File de processus READY (pour FCFS, RR, Priority, SJF, SRTF)
static process_queue_t ready_queue;

// Files MLFQ (3 niveaux de priorité)
static process_queue_t mlfq_queues[MLFQ_LEVELS];

// Processus actuellement en cours d'exécution
static process_t* current_process = NULL;

// Journal d'exécution
static exec_log_entry_t exec_log[EXEC_LOG_SIZE];
static uint32_t exec_log_index = 0;

// Statistiques
static uint64_t total_context_switches = 0;
static uint64_t last_mlfq_boost = 0;

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
        // File vide
        queue->head = process;
        queue->tail = process;
    } else {
        // Ajouter à la fin
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

    // Cas spécial : premier élément
    if (queue->head == process) {
        queue_dequeue(queue);
        return;
    }

    // Chercher le processus dans la file
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
// Fonctions auxiliaires pour différents algorithmes
// =============================================================================

// Trouve le processus avec la plus haute priorité dans la file
static process_t* find_highest_priority(process_queue_t* queue) {
    if (queue_is_empty(queue)) {
        return NULL;
    }

    process_t* best = queue->head;
    process_t* current = queue->head->next;

    while (current != NULL) {
        // Plus haute priorité = valeur plus grande
        if (current->priority > best->priority) {
            best = current;
        }
        current = current->next;
    }

    return best;
}

// Trouve le processus avec le plus court burst time (SJF)
static process_t* find_shortest_job(process_queue_t* queue) {
    if (queue_is_empty(queue)) {
        return NULL;
    }

    process_t* best = queue->head;
    process_t* current = queue->head->next;

    while (current != NULL) {
        if (current->burst_time < best->burst_time) {
            best = current;
        }
        current = current->next;
    }

    return best;
}

// Trouve le processus avec le plus court temps restant (SRTF)
static process_t* find_shortest_remaining(process_queue_t* queue) {
    if (queue_is_empty(queue)) {
        return NULL;
    }

    process_t* best = queue->head;
    process_t* current = queue->head->next;

    while (current != NULL) {
        if (current->remaining_time < best->remaining_time) {
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
    if (exec_log_index >= EXEC_LOG_SIZE) {
        // Rotation du journal (écrase les anciennes entrées)
        exec_log_index = 0;
    }

    exec_log_entry_t* entry = &exec_log[exec_log_index++];
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
}

// =============================================================================
// Fonctions de sélection selon l'algorithme
// =============================================================================

static process_t* select_next_fcfs(void) {
    return queue_dequeue(&ready_queue);
}

static process_t* select_next_round_robin(void) {
    return queue_dequeue(&ready_queue);
}

static process_t* select_next_priority(void) {
    process_t* best = find_highest_priority(&ready_queue);
    if (best != NULL) {
        queue_remove(&ready_queue, best);
    }
    return best;
}

static process_t* select_next_sjf(void) {
    process_t* best = find_shortest_job(&ready_queue);
    if (best != NULL) {
        queue_remove(&ready_queue, best);
    }
    return best;
}

static process_t* select_next_srtf(void) {
    process_t* best = find_shortest_remaining(&ready_queue);
    if (best != NULL) {
        queue_remove(&ready_queue, best);
    }
    return best;
}

static process_t* select_next_mlfq(void) {
    // Chercher dans les files du plus haut au plus bas niveau
    for (int level = 0; level < MLFQ_LEVELS; level++) {
        if (!queue_is_empty(&mlfq_queues[level])) {
            return queue_dequeue(&mlfq_queues[level]);
        }
    }
    return NULL;
}

static process_t* select_next_process(void) {
    switch (current_scheduler) {
        case SCHEDULER_FCFS:
            return select_next_fcfs();
        case SCHEDULER_ROUND_ROBIN:
            return select_next_round_robin();
        case SCHEDULER_PRIORITY:
            return select_next_priority();
        case SCHEDULER_SJF:
            return select_next_sjf();
        case SCHEDULER_SRTF:
            return select_next_srtf();
        case SCHEDULER_MLFQ:
            return select_next_mlfq();
        default:
            return select_next_round_robin();
    }
}

// Retourne le quantum approprié pour MLFQ
static uint32_t get_mlfq_quantum(uint32_t level) {
    switch (level) {
        case 0: return MLFQ_QUANTUM_0;
        case 1: return MLFQ_QUANTUM_1;
        case 2: return MLFQ_QUANTUM_2;
        default: return MLFQ_QUANTUM_2;
    }
}

// =============================================================================
// Ordonnanceur principal
// =============================================================================

void scheduler_init(scheduler_type_t type) {
    printf("[SCHEDULER] Initialisation de l'ordonnanceur...\n");

    current_scheduler = type;
    queue_init(&ready_queue);

    // Initialiser les files MLFQ
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        queue_init(&mlfq_queues[i]);
    }

    current_process = NULL;
    exec_log_index = 0;
    total_context_switches = 0;
    last_mlfq_boost = 0;

    printf("[SCHEDULER] Type: %s\n", scheduler_type_to_string(type));
    printf("[SCHEDULER] Ordonnanceur initialise\n");
}

void scheduler_add_process(process_t* process) {
    if (process == NULL) {
        return;
    }

    // Mettre le processus en état READY
    process->state = PROCESS_STATE_READY;

    // Initialiser les champs pour SJF/SRTF si non définis
    if (process->burst_time == 0) {
        process->burst_time = 50;  // Valeur par défaut
    }
    if (process->remaining_time == 0) {
        process->remaining_time = process->burst_time;
    }
    process->arrival_time = (uint32_t)timer_get_ticks();

    // Ajouter selon le type d'ordonnanceur
    if (current_scheduler == SCHEDULER_MLFQ) {
        // MLFQ: nouveaux processus au niveau 0 (haute priorité)
        process->mlfq_level = 0;
        process->mlfq_allotment = MLFQ_ALLOTMENT;
        process->time_slice = get_mlfq_quantum(0);
        process->remaining_slice = process->time_slice;
        queue_enqueue(&mlfq_queues[0], process);
    } else {
        queue_enqueue(&ready_queue, process);
    }

    printf("[SCHEDULER] Processus PID=%u ajoute a la file READY\n", process->pid);
}

void scheduler_remove_process(process_t* process) {
    if (process == NULL) {
        return;
    }

    if (current_scheduler == SCHEDULER_MLFQ) {
        for (int i = 0; i < MLFQ_LEVELS; i++) {
            queue_remove(&mlfq_queues[i], process);
        }
    } else {
        queue_remove(&ready_queue, process);
    }

    printf("[SCHEDULER] Processus PID=%u retire de la file READY\n", process->pid);
}

void scheduler_block_process(process_t* process) {
    if (process == NULL) {
        return;
    }

    // Retirer de la file READY
    scheduler_remove_process(process);

    // Changer l'état
    process->state = PROCESS_STATE_BLOCKED;

    printf("[SCHEDULER] Processus PID=%u bloque\n", process->pid);
}

void scheduler_unblock_process(process_t* process) {
    if (process == NULL || process->state != PROCESS_STATE_BLOCKED) {
        return;
    }

    // Réinitialiser la raison de blocage
    process->block_reason = BLOCK_REASON_NONE;
    process->block_resource = NULL;

    // Remettre dans la file READY
    scheduler_add_process(process);

    printf("[SCHEDULER] Processus PID=%u debloque\n", process->pid);
}

void scheduler_mlfq_boost(void) {
    if (current_scheduler != SCHEDULER_MLFQ) {
        return;
    }

    printf("[SCHEDULER] MLFQ Boost - Tous les processus remontent au niveau 0\n");

    // Déplacer tous les processus des niveaux 1 et 2 vers le niveau 0
    for (int level = 1; level < MLFQ_LEVELS; level++) {
        while (!queue_is_empty(&mlfq_queues[level])) {
            process_t* p = queue_dequeue(&mlfq_queues[level]);
            p->mlfq_level = 0;
            p->mlfq_allotment = MLFQ_ALLOTMENT;
            p->time_slice = get_mlfq_quantum(0);
            p->remaining_slice = p->time_slice;
            queue_enqueue(&mlfq_queues[0], p);
        }
    }

    last_mlfq_boost = timer_get_ticks();
}

void scheduler_schedule(void) {
    // Vérifier si un boost MLFQ est nécessaire
    if (current_scheduler == SCHEDULER_MLFQ) {
        if (timer_get_ticks() - last_mlfq_boost >= MLFQ_BOOST_INTERVAL) {
            scheduler_mlfq_boost();
        }
    }

    // Si pas de processus courant, en choisir un
    if (current_process == NULL) {
        current_process = select_next_process();

        if (current_process != NULL) {
            current_process->state = PROCESS_STATE_RUNNING;
            current_process->remaining_slice = current_process->time_slice;
            current_process->start_tick = timer_get_ticks();
            printf("[SCHEDULER] Demarrage processus PID=%u ('%s')\n",
                   current_process->pid, current_process->name);
        }
        return;
    }

    // Incrémenter les ticks du processus courant
    current_process->total_ticks++;
    current_process->remaining_slice--;

    // Décrémenter le temps restant pour SRTF
    if (current_scheduler == SCHEDULER_SRTF && current_process->remaining_time > 0) {
        current_process->remaining_time--;
    }

    // Décrémenter l'allotment pour MLFQ
    if (current_scheduler == SCHEDULER_MLFQ && current_process->mlfq_allotment > 0) {
        current_process->mlfq_allotment--;
    }

    // Vérifier si on doit changer de processus
    bool should_switch = false;
    bool preempt_better = false;  // Un meilleur processus est arrivé

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

        case SCHEDULER_PRIORITY: {
            // Priority : préemption si processus plus prioritaire
            process_t* best = find_highest_priority(&ready_queue);
            if (best != NULL && best->priority > current_process->priority) {
                preempt_better = true;
                should_switch = true;
            }
            break;
        }

        case SCHEDULER_SJF:
            // SJF : pas de préemption (non-préemptif)
            should_switch = false;
            break;

        case SCHEDULER_SRTF: {
            // SRTF : préemption si processus avec temps restant plus court
            process_t* best = find_shortest_remaining(&ready_queue);
            if (best != NULL && best->remaining_time < current_process->remaining_time) {
                preempt_better = true;
                should_switch = true;
            }
            break;
        }

        case SCHEDULER_MLFQ:
            // MLFQ : quantum épuisé = changement + possible demotion
            if (current_process->remaining_slice == 0) {
                should_switch = true;

                // Demotion si allotment épuisé
                if (current_process->mlfq_allotment == 0 &&
                    current_process->mlfq_level < MLFQ_LEVELS - 1) {
                    current_process->mlfq_level++;
                    current_process->mlfq_allotment = MLFQ_ALLOTMENT;
                    printf("[SCHEDULER] MLFQ Demotion: PID=%u -> niveau %u\n",
                           current_process->pid, current_process->mlfq_level);
                }
            }
            break;
    }

    // Effectuer le context switch si nécessaire
    if (should_switch) {
        // Pour SRTF/Priority avec préemption, vérifier qu'il y a un meilleur processus
        if ((current_scheduler == SCHEDULER_SRTF || current_scheduler == SCHEDULER_PRIORITY)
            && !preempt_better) {
            // Pas de meilleur processus, continuer avec le courant
            current_process->remaining_slice = current_process->time_slice;
            return;
        }

        // Vérifier qu'il y a des processus en attente
        bool has_waiting = false;
        if (current_scheduler == SCHEDULER_MLFQ) {
            for (int i = 0; i < MLFQ_LEVELS; i++) {
                if (!queue_is_empty(&mlfq_queues[i])) {
                    has_waiting = true;
                    break;
                }
            }
        } else {
            has_waiting = !queue_is_empty(&ready_queue);
        }

        if (!has_waiting) {
            // Pas d'autre processus, continuer avec le courant
            current_process->remaining_slice = current_process->time_slice;
            return;
        }

        // Sauvegarder le contexte du processus courant
        process_t* old_process = current_process;
        uint64_t old_start = old_process->start_tick;

        // Logger l'exécution
        log_execution(old_process, old_start, timer_get_ticks());

        // Remettre le processus courant dans la file READY appropriée
        old_process->state = PROCESS_STATE_READY;

        if (current_scheduler == SCHEDULER_MLFQ) {
            old_process->time_slice = get_mlfq_quantum(old_process->mlfq_level);
            queue_enqueue(&mlfq_queues[old_process->mlfq_level], old_process);
        } else {
            queue_enqueue(&ready_queue, old_process);
        }

        // Choisir le prochain processus
        current_process = select_next_process();
        current_process->state = PROCESS_STATE_RUNNING;

        if (current_scheduler == SCHEDULER_MLFQ) {
            current_process->time_slice = get_mlfq_quantum(current_process->mlfq_level);
        }

        current_process->remaining_slice = current_process->time_slice;
        current_process->start_tick = timer_get_ticks();

        total_context_switches++;

        printf("[SCHEDULER] Context switch: PID=%u -> PID=%u\n",
               old_process->pid, current_process->pid);
    }
}

scheduler_type_t scheduler_get_type(void) {
    return current_scheduler;
}

void scheduler_set_type(scheduler_type_t type) {
    // Si on change vers/depuis MLFQ, migrer les processus
    if (current_scheduler == SCHEDULER_MLFQ && type != SCHEDULER_MLFQ) {
        // De MLFQ vers autre: fusionner toutes les files MLFQ dans ready_queue
        for (int level = 0; level < MLFQ_LEVELS; level++) {
            while (!queue_is_empty(&mlfq_queues[level])) {
                process_t* p = queue_dequeue(&mlfq_queues[level]);
                queue_enqueue(&ready_queue, p);
            }
        }
    } else if (current_scheduler != SCHEDULER_MLFQ && type == SCHEDULER_MLFQ) {
        // Vers MLFQ: mettre tous les processus au niveau 0
        while (!queue_is_empty(&ready_queue)) {
            process_t* p = queue_dequeue(&ready_queue);
            p->mlfq_level = 0;
            p->mlfq_allotment = MLFQ_ALLOTMENT;
            p->time_slice = get_mlfq_quantum(0);
            p->remaining_slice = p->time_slice;
            queue_enqueue(&mlfq_queues[0], p);
        }
        last_mlfq_boost = timer_get_ticks();
    }

    current_scheduler = type;
    printf("[SCHEDULER] Type change: %s\n", scheduler_type_to_string(type));
}

process_t* scheduler_get_current(void) {
    return current_process;
}

void scheduler_print_queue(void) {
    printf("\n=== File READY ===\n");
    printf("Ordonnanceur: %s\n\n", scheduler_type_to_string(current_scheduler));

    if (current_scheduler == SCHEDULER_MLFQ) {
        // Afficher les files MLFQ
        for (int level = 0; level < MLFQ_LEVELS; level++) {
            printf("Niveau %d (quantum=%u):\n", level, get_mlfq_quantum(level));

            if (queue_is_empty(&mlfq_queues[level])) {
                printf("  (vide)\n");
            } else {
                process_t* p = mlfq_queues[level].head;
                while (p != NULL) {
                    printf("  PID=%u '%s' (allot=%u, slice=%u/%u)\n",
                           p->pid, p->name, p->mlfq_allotment,
                           p->remaining_slice, p->time_slice);
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
                printf("  %d. PID=%u '%s' (prio=%u, burst=%u, remain=%u)\n",
                       pos++, p->pid, p->name, p->priority,
                       p->burst_time, p->remaining_time);
                p = p->next;
            }
        }
    }

    printf("\n");
}

void scheduler_print_log(void) {
    printf("\n=== Journal d'execution (dernieres %u entrees) ===\n", exec_log_index);
    printf("PID  | Nom              | Debut  | Fin    | Duree (ticks)\n");
    printf("-----|------------------|--------|--------|---------------\n");

    for (uint32_t i = 0; i < exec_log_index; i++) {
        exec_log_entry_t* entry = &exec_log[i];
        printf("%-4u | %-16s | %-6u | %-6u | %u\n",
               entry->pid,
               entry->name,
               (uint32_t)entry->start_tick,
               (uint32_t)entry->end_tick,
               entry->duration);
    }

    printf("\nTotal context switches: %u\n\n", (uint32_t)total_context_switches);
}

void scheduler_print_stats(void) {
    printf("\n=== Statistiques ordonnanceur ===\n");
    printf("Type: %s\n", scheduler_type_to_string(current_scheduler));
    printf("Context switches: %u\n", (uint32_t)total_context_switches);

    if (current_scheduler == SCHEDULER_MLFQ) {
        printf("Dernier boost: tick %u\n", (uint32_t)last_mlfq_boost);
        for (int i = 0; i < MLFQ_LEVELS; i++) {
            printf("File niveau %d: %u processus\n", i, mlfq_queues[i].count);
        }
    } else {
        printf("Processus en attente: %u\n", ready_queue.count);
    }

    if (current_process != NULL) {
        printf("Processus courant: PID=%u '%s'\n",
               current_process->pid, current_process->name);
    }
    printf("\n");
}

const char* scheduler_type_to_string(scheduler_type_t type) {
    switch (type) {
        case SCHEDULER_FCFS:        return "FCFS (First Come First Served)";
        case SCHEDULER_ROUND_ROBIN: return "Round Robin";
        case SCHEDULER_PRIORITY:    return "Priority";
        case SCHEDULER_SJF:         return "SJF (Shortest Job First)";
        case SCHEDULER_SRTF:        return "SRTF (Shortest Remaining Time)";
        case SCHEDULER_MLFQ:        return "MLFQ (Multi-Level Feedback Queue)";
        default:                    return "Unknown";
    }
}

// =============================================================================
// Simulation d'ordonnancement
// =============================================================================

void scheduler_simulate(uint32_t ticks) {
    printf("\n=== Simulation d'ordonnancement (%s) ===\n", scheduler_type_to_string(current_scheduler));
    printf("Simulation de %u ticks...\n\n", ticks);

    // Reinitialiser le journal
    exec_log_index = 0;
    total_context_switches = 0;

    // Sauvegarder l'etat actuel
    process_t* saved_current = current_process;
    current_process = NULL;

    // Simuler les ticks
    uint64_t sim_tick = 0;

    for (uint32_t t = 0; t < ticks; t++) {
        sim_tick++;

        // Si pas de processus courant, en choisir un
        if (current_process == NULL) {
            current_process = select_next_process();
            if (current_process != NULL) {
                current_process->state = PROCESS_STATE_RUNNING;
                current_process->remaining_slice = current_process->time_slice;
                current_process->start_tick = sim_tick;
            }
            continue;
        }

        // Incrementer les compteurs
        current_process->total_ticks++;
        current_process->remaining_slice--;

        if (current_scheduler == SCHEDULER_SRTF && current_process->remaining_time > 0) {
            current_process->remaining_time--;
        }

        if (current_scheduler == SCHEDULER_MLFQ && current_process->mlfq_allotment > 0) {
            current_process->mlfq_allotment--;
        }

        // Verifier si on doit changer
        bool should_switch = false;
        bool preempt_better = false;

        switch (current_scheduler) {
            case SCHEDULER_FCFS:
                // Terminer quand remaining_time = 0
                if (current_process->remaining_time == 0) {
                    should_switch = true;
                }
                break;

            case SCHEDULER_ROUND_ROBIN:
                if (current_process->remaining_slice == 0) {
                    should_switch = true;
                }
                break;

            case SCHEDULER_PRIORITY: {
                process_t* best = find_highest_priority(&ready_queue);
                if (best != NULL && best->priority > current_process->priority) {
                    preempt_better = true;
                    should_switch = true;
                }
                // Aussi changer si quantum epuise
                if (current_process->remaining_slice == 0) {
                    should_switch = true;
                }
                break;
            }

            case SCHEDULER_SJF:
                if (current_process->remaining_time == 0) {
                    should_switch = true;
                }
                break;

            case SCHEDULER_SRTF: {
                process_t* best = find_shortest_remaining(&ready_queue);
                if (best != NULL && best->remaining_time < current_process->remaining_time) {
                    preempt_better = true;
                    should_switch = true;
                }
                if (current_process->remaining_time == 0) {
                    should_switch = true;
                }
                break;
            }

            case SCHEDULER_MLFQ:
                if (current_process->remaining_slice == 0) {
                    should_switch = true;
                }
                break;
        }

        // Context switch si necessaire
        if (should_switch) {
            bool has_waiting = false;
            if (current_scheduler == SCHEDULER_MLFQ) {
                for (int i = 0; i < MLFQ_LEVELS; i++) {
                    if (!queue_is_empty(&mlfq_queues[i])) {
                        has_waiting = true;
                        break;
                    }
                }
            } else {
                has_waiting = !queue_is_empty(&ready_queue);
            }

            if (!has_waiting && current_process->remaining_time > 0) {
                current_process->remaining_slice = current_process->time_slice;
                continue;
            }

            // Logger l'execution
            log_execution(current_process, current_process->start_tick, sim_tick);
            total_context_switches++;

            // Remettre en file si pas termine
            process_t* old = current_process;
            if (old->remaining_time > 0) {
                old->state = PROCESS_STATE_READY;
                if (current_scheduler == SCHEDULER_MLFQ) {
                    queue_enqueue(&mlfq_queues[old->mlfq_level], old);
                } else {
                    queue_enqueue(&ready_queue, old);
                }
            } else {
                old->state = PROCESS_STATE_TERMINATED;
                printf("  [Tick %u] Processus '%s' termine\n", (uint32_t)sim_tick, old->name);
            }

            // Choisir le suivant
            current_process = select_next_process();
            if (current_process != NULL) {
                current_process->state = PROCESS_STATE_RUNNING;
                current_process->remaining_slice = current_process->time_slice;
                current_process->start_tick = sim_tick;
            }
        }
    }

    // Log final si un processus etait en cours
    if (current_process != NULL) {
        log_execution(current_process, current_process->start_tick, sim_tick);
    }

    printf("\nSimulation terminee. Utilisez 'log' pour voir le journal.\n");

    // Restaurer l'etat
    current_process = saved_current;
}
