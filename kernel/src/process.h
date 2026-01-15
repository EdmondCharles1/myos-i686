/*
 * process.h - Gestion de processus (PCB)
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

// Définition de bool si nécessaire (pas de stdbool.h en bare-metal)
#ifndef __cplusplus
typedef _Bool bool;
#define true 1
#define false 0
#endif

// =============================================================================
// Constantes
// =============================================================================

#define MAX_PROCESSES       32      // Nombre max de processus
#define PROCESS_NAME_LEN    32      // Longueur max du nom
#define PROCESS_STACK_SIZE  4096    // Taille de la pile (4 KB)

#define PRIORITY_MIN        0       // Priorité minimale
#define PRIORITY_MAX        31      // Priorité maximale
#define PRIORITY_DEFAULT    15      // Priorité par défaut

// =============================================================================
// États des processus
// =============================================================================

typedef enum {
    PROCESS_STATE_NEW,              // Processus créé, pas encore initialisé
    PROCESS_STATE_READY,            // Prêt à s'exécuter
    PROCESS_STATE_RUNNING,          // En cours d'exécution
    PROCESS_STATE_BLOCKED,          // Bloqué (attente I/O, sleep, etc.)
    PROCESS_STATE_TERMINATED        // Terminé
} process_state_t;

// =============================================================================
// Contexte CPU (registres sauvegardés lors d'un context switch)
// =============================================================================

typedef struct {
    uint32_t eax, ebx, ecx, edx;    // Registres généraux
    uint32_t esi, edi;              // Index
    uint32_t esp, ebp;              // Stack et base pointer
    uint32_t eip;                   // Instruction pointer
    uint32_t eflags;                // Flags
    uint32_t cr3;                   // Page directory (pour la pagination)
} cpu_context_t;

// =============================================================================
// PCB (Process Control Block)
// =============================================================================

typedef struct process {
    // Identification
    uint32_t pid;                   // Process ID (unique)
    char name[PROCESS_NAME_LEN];    // Nom du processus
    
    // État
    process_state_t state;          // État actuel
    uint32_t priority;              // Priorité (0-31, plus haut = plus prioritaire)
    
    // Contexte CPU
    cpu_context_t context;          // Registres sauvegardés
    
    // Mémoire
    uint32_t stack_base;            // Base de la pile
    uint32_t stack_size;            // Taille de la pile
    
    // Statistiques
    uint64_t total_ticks;           // Nombre total de ticks CPU
    uint64_t start_tick;            // Tick de démarrage
    uint32_t time_slice;            // Quantum de temps (pour Round Robin)
    uint32_t remaining_slice;       // Temps restant dans le quantum
    
    // Relations
    uint32_t parent_pid;            // PID du processus parent (0 si aucun)
    
    // Liste chaînée (pour les files de processus)
    struct process* next;           // Processus suivant dans la file
    
} process_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le système de gestion de processus
 */
void process_init(void);

/**
 * Crée un nouveau processus
 * 
 * @param name      Nom du processus
 * @param entry     Adresse de la fonction d'entrée
 * @param priority  Priorité (0-31)
 * @return          PID du processus créé, ou 0 en cas d'erreur
 */
uint32_t process_create(const char* name, void (*entry)(void), uint32_t priority);

/**
 * Termine le processus courant
 */
void process_exit(void);

/**
 * Termine un processus spécifique
 * 
 * @param pid PID du processus à terminer
 * @return    true si succès, false sinon
 */
bool process_kill(uint32_t pid);

/**
 * Retourne le processus actuellement en cours d'exécution
 * 
 * @return Pointeur vers le PCB du processus courant
 */
process_t* process_get_current(void);

/**
 * Retourne un processus par son PID
 * 
 * @param pid PID du processus
 * @return    Pointeur vers le PCB, ou NULL si non trouvé
 */
process_t* process_get_by_pid(uint32_t pid);

/**
 * Change l'état d'un processus
 * 
 * @param process Pointeur vers le PCB
 * @param state   Nouvel état
 */
void process_set_state(process_t* process, process_state_t state);

/**
 * Affiche la liste de tous les processus (pour debug)
 */
void process_list(void);

/**
 * Retourne le nombre de processus actifs
 * 
 * @return Nombre de processus (tous états confondus sauf TERMINATED)
 */
uint32_t process_count(void);

/**
 * Convertit un état en chaîne de caractères
 */
const char* process_state_to_string(process_state_t state);

#endif // PROCESS_H