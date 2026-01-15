/*
 * scheduler.c - Implementation de l'ordonnanceur
 */

#include "scheduler.h"
#include "printf.h"
#include "timer.h"

// =============================================================================
// Variables globales
// =============================================================================

// Type d'ordonnancement actuel
static scheduler_type_t current_scheduler = SCHEDULER_ROUND_ROBIN;

// File de processus READY
static process_queue_t ready_queue;

// Processus actuellement en cours d'exécution
static process_t* current_process = NULL;

// Journal d'exécution
static exec_log_entry_t exec_log[EXEC_LOG_SIZE];
static uint32_t exec_log_index = 0;

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
// Ordonnanceur
// =============================================================================

void scheduler_init(scheduler_type_t type) {
    printf("[SCHEDULER] Initialisation de l'ordonnanceur...\n");
    
    current_scheduler = type;
    queue_init(&ready_queue);
    current_process = NULL;
    exec_log_index = 0;
    total_context_switches = 0;
    
    printf("[SCHEDULER] Type: %s\n", scheduler_type_to_string(type));
    printf("[SCHEDULER] Ordonnanceur initialise\n");
}

void scheduler_add_process(process_t* process) {
    if (process == NULL) {
        return;
    }
    
    // Mettre le processus en état READY
    process->state = PROCESS_STATE_READY;
    
    // Ajouter à la file READY
    queue_enqueue(&ready_queue, process);
    
    printf("[SCHEDULER] Processus PID=%u ajoute a la file READY\n", process->pid);
}

void scheduler_remove_process(process_t* process) {
    if (process == NULL) {
        return;
    }
    
    queue_remove(&ready_queue, process);
    
    printf("[SCHEDULER] Processus PID=%u retire de la file READY\n", process->pid);
}

void scheduler_schedule(void) {
    // Si pas de processus courant, en choisir un
    if (current_process == NULL) {
        current_process = queue_dequeue(&ready_queue);
        
        if (current_process != NULL) {
            current_process->state = PROCESS_STATE_RUNNING;
            current_process->remaining_slice = current_process->time_slice;
            printf("[SCHEDULER] Demarrage processus PID=%u ('%s')\n",
                   current_process->pid, current_process->name);
        }
        return;
    }
    
    // Incrémenter les ticks du processus courant
    current_process->total_ticks++;
    current_process->remaining_slice--;
    
    // Vérifier si le processus a terminé son quantum (Round Robin)
    bool should_switch = false;
    
    switch (current_scheduler) {
        case SCHEDULER_FCFS:
            // FCFS : pas de préemption, on continue jusqu'à la fin
            should_switch = false;
            break;
            
        case SCHEDULER_ROUND_ROBIN:
            // Round Robin : changer si quantum épuisé
            if (current_process->remaining_slice == 0) {
                should_switch = true;
            }
            break;
            
        case SCHEDULER_PRIORITY:
            // TODO: implémenter ordonnancement par priorité
            should_switch = false;
            break;
    }
    
    // Effectuer le context switch si nécessaire
    if (should_switch && !queue_is_empty(&ready_queue)) {
        // Sauvegarder le contexte du processus courant
        process_t* old_process = current_process;
        uint64_t old_start = old_process->start_tick;
        
        // Logger l'exécution
        log_execution(old_process, old_start, timer_get_ticks());
        
        // Remettre le processus courant dans la file READY
        old_process->state = PROCESS_STATE_READY;
        queue_enqueue(&ready_queue, old_process);
        
        // Choisir le prochain processus
        current_process = queue_dequeue(&ready_queue);
        current_process->state = PROCESS_STATE_RUNNING;
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
    current_scheduler = type;
    printf("[SCHEDULER] Type change: %s\n", scheduler_type_to_string(type));
}

void scheduler_print_queue(void) {
    printf("\n=== File READY ===\n");
    
    if (queue_is_empty(&ready_queue)) {
        printf("(vide)\n\n");
        return;
    }
    
    printf("Processus dans la file (%u) :\n", ready_queue.count);
    
    process_t* p = ready_queue.head;
    int pos = 1;
    while (p != NULL) {
        printf("  %d. PID=%u '%s' (priorite=%u, quantum=%u/%u)\n",
               pos++, p->pid, p->name, p->priority,
               p->remaining_slice, p->time_slice);
        p = p->next;
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

const char* scheduler_type_to_string(scheduler_type_t type) {
    switch (type) {
        case SCHEDULER_FCFS:        return "FCFS (First Come First Served)";
        case SCHEDULER_ROUND_ROBIN: return "Round Robin";
        case SCHEDULER_PRIORITY:    return "Priority";
        default:                    return "Unknown";
    }
}