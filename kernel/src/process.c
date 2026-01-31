/*
 * process.c - Implementation de la gestion de processus
 */

#include "process.h"
#include "printf.h"
#include "timer.h"

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
    // Vérifier les paramètres
    if (name == NULL || entry == NULL) {
        printf("[PROCESS] Erreur: parametres invalides\n");
        return 0;
    }
    
    // Limiter la priorité
    if (priority > PRIORITY_MAX) {
        priority = PRIORITY_MAX;
    }
    
    // Trouver un slot libre
    process_t* process = find_free_slot();
    if (process == NULL) {
        printf("[PROCESS] Erreur: table de processus pleine\n");
        return 0;
    }
    
    // Initialiser le PCB
    process->pid = next_pid++;
    string_copy(process->name, name, PROCESS_NAME_LEN);
    process->state = PROCESS_STATE_NEW;
    process->priority = priority;
    
    // Allouer une pile (simplifié : on utilise un buffer statique pour l'instant)
    // TODO: implémenter un allocateur de mémoire
    static uint8_t stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];
    
    // Calculer l'index du stack à utiliser
    uint32_t stack_index = (process->pid - 1) % MAX_PROCESSES;
    
    // Mettre à zéro le stack
    memory_zero(&stacks[stack_index][0], PROCESS_STACK_SIZE);
    
    // Le stack grandit vers le bas, donc on pointe vers la fin
    process->stack_base = (uint32_t)&stacks[stack_index][PROCESS_STACK_SIZE];
    process->stack_size = PROCESS_STACK_SIZE;
    
    // Initialiser le contexte CPU
    process->context.eip = (uint32_t)entry;
    process->context.esp = process->stack_base;
    process->context.ebp = process->stack_base;
    process->context.eflags = 0x202;  // Interruptions activées (IF=1)
    
    // Initialiser tous les registres à 0
    process->context.eax = 0;
    process->context.ebx = 0;
    process->context.ecx = 0;
    process->context.edx = 0;
    process->context.esi = 0;
    process->context.edi = 0;
    process->context.cr3 = 0;  // Pas de pagination pour l'instant
    
    // Statistiques
    process->total_ticks = 0;
    process->start_tick = timer_get_ticks();
    process->time_slice = 10;  // 10 ticks par défaut (100ms à 100Hz)
    process->remaining_slice = process->time_slice;

    // Champs pour SJF/SRTF (valeurs par défaut)
    process->burst_time = 50;       // Estimation par défaut
    process->remaining_time = 50;
    process->arrival_time = (uint32_t)timer_get_ticks();

    // Champs pour MLFQ
    process->mlfq_level = 0;
    process->mlfq_allotment = 30;

    // Champs de blocage
    process->block_reason = 0;
    process->block_resource = NULL;

    // Relations
    if (current_process != NULL) {
        process->parent_pid = current_process->pid;
    } else {
        process->parent_pid = 0;
    }

    process->next = NULL;
    
    // Passer à l'état READY
    process->state = PROCESS_STATE_READY;
    active_processes++;
    
    printf("[PROCESS] Processus cree: PID=%u, nom='%s', priorite=%u\n",
           process->pid, process->name, process->priority);
    
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