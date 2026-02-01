/*
 * process.c - Implementation de la gestion de processus
 */

#include "process.h"
#include "printf.h"
#include "timer.h"
#include <stdint.h>

// =============================================================================
// Variables globales
// =============================================================================

// Table de processus (statique pour simplifier)
static process_t process_table[MAX_PROCESSES];

// Compteur de PID (auto-incrémenté)
static uint32_t next_pid = 1;

// Processus actuellement en cours d'exécution
static process_t* current_process = NULL;

// Nombre de processus actifs
static uint32_t active_processes = 0;

// =============================================================================
// Fonctions utilitaires internes
// =============================================================================

/**
 * Trouve un slot libre dans la table de processus
 */
static process_t* find_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_TERMINATED || 
            process_table[i].pid == 0) {
            return &process_table[i];
        }
    }
    return NULL;
}

/**
 * Copie une chaîne de caractères (comme strncpy)
 */
static void string_copy(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/**
 * Met à zéro une zone mémoire (comme memset)
 */
static void memory_zero(void* ptr, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

// =============================================================================
// Implémentation de l'API
// =============================================================================

void process_init(void) {
    printf("[PROCESS] Initialisation du systeme de processus...\n");
    
    // Initialiser la table de processus
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_STATE_TERMINATED;
        process_table[i].next = NULL;
        
        // Mettre à zéro le nom
        for (int j = 0; j < PROCESS_NAME_LEN; j++) {
            process_table[i].name[j] = '\0';
        }
    }
    
    next_pid = 1;
    current_process = NULL;
    active_processes = 0;
    
    printf("[PROCESS] Table de processus initialisee (%d slots)\n", MAX_PROCESSES);
}

uint32_t process_create(const char* name, void (*entry)(void), uint32_t priority) {
    if (name == NULL || entry == NULL) return 0;
    
    if (priority > PRIORITY_MAX) priority = PRIORITY_MAX;
    
    process_t* process = find_free_slot();
    if (process == NULL) return 0;
    
    // --- 1. NETTOYAGE CRITIQUE ---
    // On remet TOUT le PCB à zéro pour éviter les résidus (surtout le pointeur 'next')
    memory_zero(process, sizeof(process_t));

    process->pid = next_pid++;
    string_copy(process->name, name, PROCESS_NAME_LEN);
    
    // --- 2. ÉTAT INITIAL : NEW ---
    // On le laisse en NEW. C'est scheduler_add_process qui le passera en READY.
    process->state = PROCESS_STATE_NEW;
    process->priority = priority;
    
    // Gestion de la pile
    static uint8_t stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];
    uint32_t stack_index = (process->pid - 1) % MAX_PROCESSES;
    memory_zero(&stacks[stack_index][0], PROCESS_STACK_SIZE);
    
    // Initialisation du contexte (Stack Frame pour x86)
    process->stack_base = (uint32_t)(uintptr_t)&stacks[stack_index][PROCESS_STACK_SIZE];
    
    // IMPORTANT : On simule un empilement initial pour le context_switch
    // On réserve de l'espace pour les registres que 'pop' va récupérer
    process->context.esp = process->stack_base - sizeof(cpu_context_t); 
    process->context.ebp = process->stack_base;
    process->context.eip = (uint32_t)(uintptr_t)entry;
    process->context.eflags = 0x202; // IF=1
    
    // Stats et Burst
    process->start_tick = timer_get_ticks();
    process->burst_time = 50;
    process->remaining_time = 50;
    process->time_slice = 10;
    process->remaining_slice = 10;

    // MLFQ
    process->mlfq_level = 0;
    process->mlfq_allotment = 30;

    // Relations
    process->parent_pid = (current_process != NULL) ? current_process->pid : 0;

    // --- 3. GARANTIE DE CHAÎNAGE ---
    process->next = NULL; 
    
    active_processes++;
    
    printf("[PROCESS] Cree: PID=%u, nom='%s'\n", process->pid, process->name);
    
    return process->pid;
}
void process_exit(void) {
    if (current_process == NULL) {
        return;
    }
    
    printf("[PROCESS] Processus PID=%u ('%s') termine\n",
           current_process->pid, current_process->name);
    
    current_process->state = PROCESS_STATE_TERMINATED;
    active_processes--;
    
    // TODO: Déclencher un context switch vers le prochain processus
}

bool process_kill(uint32_t pid) {
    process_t* process = process_get_by_pid(pid);
    if (process == NULL) {
        return false;
    }
    
    printf("[PROCESS] Terminaison forcee du processus PID=%u ('%s')\n",
           process->pid, process->name);
    
    process->state = PROCESS_STATE_TERMINATED;
    active_processes--;
    
    return true;
}

process_t* process_get_current(void) {
    return current_process;
}

void process_set_current(process_t* process) {
    current_process = process;
}

process_t* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && 
            process_table[i].state != PROCESS_STATE_TERMINATED) {
            return &process_table[i];
        }
    }
    return NULL;
}

void process_set_state(process_t* process, process_state_t state) {
    if (process == NULL) {
        return;
    }
    
    process_state_t old_state = process->state;
    process->state = state;
    
    // Ne logger que si l'état change réellement
    if (old_state != state) {
        printf("[PROCESS] PID=%u: %s -> %s\n",
               process->pid,
               process_state_to_string(old_state),
               process_state_to_string(state));
    }
}

void process_list(void) {
    printf("\n=== Liste des processus ===\n");
    printf("PID  | Nom              | Etat       | Priorite | Ticks CPU\n");
    printf("-----|------------------|------------|----------|----------\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0 && 
            process_table[i].state != PROCESS_STATE_TERMINATED) {
            
            process_t* p = &process_table[i];
            
            //printf("    %u |                %s|          %s | %u | %u\n",
            printf("%-4u | %-16s | %-10s | %-8u | %u\n",
                   p->pid,
                   p->name,
                   process_state_to_string(p->state),
                   p->priority,
                   (uint32_t)p->total_ticks);
        }
    }
    
    printf("\nProcessus actifs: %u / %u\n\n", active_processes, MAX_PROCESSES);
}

uint32_t process_count(void) {
    return active_processes;
}

const char* process_state_to_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_NEW:        return "NEW";
        case PROCESS_STATE_READY:      return "READY";
        case PROCESS_STATE_RUNNING:    return "RUNNING";
        case PROCESS_STATE_BLOCKED:    return "BLOCKED";
        case PROCESS_STATE_TERMINATED: return "TERMINATED";
        default:                       return "UNKNOWN";
    }
}

process_t* process_get_table(uint32_t* out_size) {
    if (out_size != NULL) {
        *out_size = MAX_PROCESSES;
    }
    return process_table;
}