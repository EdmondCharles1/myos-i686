/*
 * shell.c - Mini-shell complet avec toutes les commandes
 * Commandes: help, clear, ps, kill, sched, log, queue, uptime, info, reboot
 *            spawn, bench, demo, states, block, unblock
 *            mem, mbox, mutex, sem
 */

#include "shell.h"
#include "keyboard.h"
#include "printf.h"
#include "process.h"
#include "scheduler.h"
#include "timer.h"
#include "memory.h"
#include "ipc.h"
#include "sync.h"

// =============================================================================
// Variables globales
// =============================================================================

static char input_buffer[SHELL_BUFFER_SIZE];
static uint32_t input_pos = 0;

static char* argv_buffer[SHELL_MAX_ARGS];

// =============================================================================
// Historique des commandes
// =============================================================================

#define HISTORY_SIZE 16

static char history[HISTORY_SIZE][SHELL_BUFFER_SIZE];
static uint32_t history_count = 0;      // Nombre de commandes dans l'historique
static uint32_t history_index = 0;      // Position actuelle dans l'historique
static uint32_t history_write_pos = 0;  // Prochaine position d'ecriture (circulaire)

// =============================================================================
// Fonctions utilitaires
// =============================================================================

static int string_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void string_copy_n(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static uint32_t string_to_uint(const char* str) {
    uint32_t result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

static uint32_t string_length(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// =============================================================================
// Fonctions d'historique
// =============================================================================

static void history_add(const char* cmd) {
    // Ne pas ajouter les commandes vides
    if (cmd[0] == '\0') {
        return;
    }

    // Copier la commande dans l'historique
    string_copy_n(history[history_write_pos], cmd, SHELL_BUFFER_SIZE);

    // Avancer la position d'ecriture (circulaire)
    history_write_pos = (history_write_pos + 1) % HISTORY_SIZE;

    // Mettre a jour le compteur
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }

    // Reinitialiser l'index de navigation
    history_index = history_count;
}

static const char* history_get_prev(void) {
    if (history_count == 0) {
        return NULL;
    }

    if (history_index > 0) {
        history_index--;
    }

    // Calculer la position reelle dans le buffer circulaire
    uint32_t pos;
    if (history_count < HISTORY_SIZE) {
        pos = history_index;
    } else {
        pos = (history_write_pos + history_index) % HISTORY_SIZE;
    }

    return history[pos];
}

static const char* history_get_next(void) {
    if (history_count == 0 || history_index >= history_count) {
        return NULL;
    }

    history_index++;

    if (history_index >= history_count) {
        // Retourner une chaine vide pour effacer la ligne
        return "";
    }

    // Calculer la position reelle dans le buffer circulaire
    uint32_t pos;
    if (history_count < HISTORY_SIZE) {
        pos = history_index;
    } else {
        pos = (history_write_pos + history_index) % HISTORY_SIZE;
    }

    return history[pos];
}

static void history_reset_navigation(void) {
    history_index = history_count;
}

static int parse_command(char* input, char** argv, int max_args) {
    int argc = 0;
    char* p = input;

    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        argv[argc++] = p;

        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }

        if (*p) {
            *p = '\0';
            p++;
        }
    }

    return argc;
}

// =============================================================================
// Processus de demo
// =============================================================================

// Processus simple pour benchmark
static void demo_process_short(void) {
    volatile uint32_t count = 0;
    for (uint32_t i = 0; i < 100000; i++) {
        count++;
    }
}

static void demo_process_medium(void) {
    volatile uint32_t count = 0;
    for (uint32_t i = 0; i < 500000; i++) {
        count++;
    }
}

static void demo_process_long(void) {
    volatile uint32_t count = 0;
    for (uint32_t i = 0; i < 1000000; i++) {
        count++;
    }
}

// =============================================================================
// Commandes de base
// =============================================================================

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Mini-Shell myos-i686 v0.8 ===\n\n");
    printf("COMMANDES DE BASE:\n");
    printf("  help              - Affiche cette aide\n");
    printf("  clear             - Efface l'ecran\n");
    printf("  info              - Informations systeme\n");
    printf("  uptime            - Temps d'execution\n");
    printf("  reboot            - Redemarrage (simulation)\n");
    printf("\n");
    printf("GESTION DES PROCESSUS:\n");
    printf("  ps                - Liste les processus\n");
    printf("  kill <pid>        - Termine un processus\n");
    printf("  spawn [n] [burst] - Cree n processus (burst=estimation CPU)\n");
    printf("  bench             - Lance un benchmark\n");
    printf("  demo              - Demo d'ordonnancement\n");
    printf("  states            - Montre les transitions d'etats\n");
    printf("  block <pid>       - Bloque un processus\n");
    printf("  unblock <pid>     - Debloque un processus\n");
    printf("\n");
    printf("ORDONNANCEMENT:\n");
    printf("  sched [type]      - Change/affiche ordonnanceur\n");
    printf("    Types: fcfs, rr, priority, sjf, srtf, mlfq\n");
    printf("  log               - Journal d'execution\n");
    printf("  queue             - File READY\n");
    printf("\n");
    printf("MEMOIRE:\n");
    printf("  mem [test|stats]  - Gestion memoire\n");
    printf("\n");
    printf("IPC (MAILBOXES):\n");
    printf("  mbox list         - Liste les mailboxes\n");
    printf("  mbox create <nom> - Cree une mailbox\n");
    printf("  mbox send <id> <msg>  - Envoie un message\n");
    printf("  mbox recv <id>    - Recoit un message\n");
    printf("  mbox test         - Test IPC\n");
    printf("\n");
    printf("SYNCHRONISATION:\n");
    printf("  mutex list        - Liste les mutex\n");
    printf("  mutex create <nom>- Cree un mutex\n");
    printf("  mutex lock <id>   - Verrouille\n");
    printf("  mutex unlock <id> - Deverrouille\n");
    printf("  mutex test        - Test mutex\n");
    printf("  sem list          - Liste les semaphores\n");
    printf("  sem create <nom> <val> - Cree un semaphore\n");
    printf("  sem wait <id>     - Decremente (P)\n");
    printf("  sem post <id>     - Incremente (V)\n");
    printf("  sem test          - Test semaphores\n");
    printf("\n");
}

static void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;

    extern void terminal_clear(void);
    terminal_clear();
    // Ne pas afficher le prompt ici - il sera affiche par shell_run()
}

static void cmd_ps(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    process_list();
}

static void cmd_kill(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return;
    }

    uint32_t pid = string_to_uint(argv[1]);

    if (pid == 0) {
        printf("PID invalide\n");
        return;
    }

    printf("Terminaison du processus PID=%u...\n", pid);

    if (process_kill(pid)) {
        printf("Processus termine avec succes\n");
    } else {
        printf("Echec: processus introuvable\n");
    }
}

static void cmd_sched(int argc, char** argv) {
    if (argc < 2) {
        printf("Ordonnanceur actuel: %s\n", scheduler_type_to_string(scheduler_get_type()));
        printf("Types disponibles: fcfs, rr, priority, sjf, srtf, mlfq\n");
        return;
    }

    if (string_compare(argv[1], "fcfs") == 0) {
        scheduler_set_type(SCHEDULER_FCFS);
        printf("Ordonnanceur: FCFS (non preemptif)\n");
    } else if (string_compare(argv[1], "rr") == 0) {
        scheduler_set_type(SCHEDULER_ROUND_ROBIN);
        printf("Ordonnanceur: Round Robin (quantum=10 ticks)\n");
    } else if (string_compare(argv[1], "priority") == 0) {
        scheduler_set_type(SCHEDULER_PRIORITY);
        printf("Ordonnanceur: Priority (preemptif)\n");
    } else if (string_compare(argv[1], "sjf") == 0) {
        scheduler_set_type(SCHEDULER_SJF);
        printf("Ordonnanceur: SJF (non preemptif)\n");
    } else if (string_compare(argv[1], "srtf") == 0) {
        scheduler_set_type(SCHEDULER_SRTF);
        printf("Ordonnanceur: SRTF (preemptif)\n");
    } else if (string_compare(argv[1], "mlfq") == 0) {
        scheduler_set_type(SCHEDULER_MLFQ);
        printf("Ordonnanceur: MLFQ (3 niveaux)\n");
    } else {
        printf("Type inconnu: %s\n", argv[1]);
        printf("Types disponibles: fcfs, rr, priority, sjf, srtf, mlfq\n");
    }
}

static void cmd_log(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    scheduler_print_log();
}

static void cmd_queue(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    scheduler_print_queue();
}

static void cmd_uptime(int argc, char** argv) {
    (void)argc;
    (void)argv;

    uint64_t ms = timer_get_ms();
    uint32_t seconds = (uint32_t)(ms / 1000);
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    seconds %= 60;
    minutes %= 60;

    printf("Uptime: %u heures, %u minutes, %u secondes\n", hours, minutes, seconds);
    printf("Ticks totaux: %u\n", (uint32_t)timer_get_ticks());
}

static void cmd_info(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Informations systeme ===\n");
    printf("OS: myos-i686 v0.8 (complet)\n");
    printf("Architecture: x86 (32-bit, i686)\n");
    printf("Compilateur: i686-elf-gcc\n");
    printf("Timer: PIT 100 Hz (10ms/tick)\n");
    printf("Ordonnanceur: %s\n", scheduler_type_to_string(scheduler_get_type()));
    printf("Processus: %u / %u\n", process_count(), 32);
    printf("Memoire pool: 64 KB\n");
    printf("Max mailboxes: %d\n", MAX_MAILBOXES);
    printf("Max mutex: %d\n", MAX_MUTEXES);
    printf("Max semaphores: %d\n", MAX_SEMAPHORES);
    printf("Uptime: %u ms\n", (uint32_t)timer_get_ms());
    printf("\n");
}

static void cmd_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("Redemarrage du systeme...\n");
    printf("(Simulation - appuyez sur Ctrl+C dans QEMU)\n");
}

// =============================================================================
// Commandes de gestion des processus
// =============================================================================

static void cmd_spawn(int argc, char** argv) {
    uint32_t count = 1;
    uint32_t burst = 50;

    if (argc >= 2) {
        count = string_to_uint(argv[1]);
        if (count == 0) count = 1;
        if (count > 10) count = 10;
    }

    if (argc >= 3) {
        burst = string_to_uint(argv[2]);
        if (burst == 0) burst = 50;
    }

    printf("Creation de %u processus (burst=%u)...\n", count, burst);

    for (uint32_t i = 0; i < count; i++) {
        char name[16];
        // Construire le nom manuellement
        name[0] = 'P';
        name[1] = 'r';
        name[2] = 'o';
        name[3] = 'c';
        name[4] = '_';
        name[5] = '0' + (i % 10);
        name[6] = '\0';

        uint32_t priority = (i * 5) % 32;  // Priorites variees

        uint32_t pid = process_create(name, demo_process_medium, priority);
        if (pid > 0) {
            process_t* p = process_get_by_pid(pid);
            if (p) {
                p->burst_time = burst + (i * 10);  // Burst varie
                p->remaining_time = p->burst_time;
            }
            scheduler_add_process(p);
        }
    }

    printf("%u processus crees\n", count);
}

static void cmd_bench(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Benchmark ordonnancement ===\n\n");

    // Creer des processus avec differents burst times
    printf("Creation de processus de test...\n");

    uint32_t pid1 = process_create("Short", demo_process_short, 10);
    uint32_t pid2 = process_create("Medium", demo_process_medium, 15);
    uint32_t pid3 = process_create("Long", demo_process_long, 20);

    if (pid1 && pid2 && pid3) {
        process_t* p1 = process_get_by_pid(pid1);
        process_t* p2 = process_get_by_pid(pid2);
        process_t* p3 = process_get_by_pid(pid3);

        // Definir des burst times differents pour SJF/SRTF
        if (p1) { p1->burst_time = 20; p1->remaining_time = 20; scheduler_add_process(p1); }
        if (p2) { p2->burst_time = 50; p2->remaining_time = 50; scheduler_add_process(p2); }
        if (p3) { p3->burst_time = 100; p3->remaining_time = 100; scheduler_add_process(p3); }

        printf("Processus crees:\n");
        printf("  - Short  (PID=%u, burst=20)\n", pid1);
        printf("  - Medium (PID=%u, burst=50)\n", pid2);
        printf("  - Long   (PID=%u, burst=100)\n", pid3);
        printf("\nUtilisez 'log' pour voir le journal d'execution\n");
        printf("Utilisez 'sched <type>' pour changer l'ordonnanceur\n");
    } else {
        printf("Erreur lors de la creation des processus\n");
    }
}

static void cmd_demo(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Demo d'ordonnancement ===\n\n");
    printf("Ordonnanceur actuel: %s\n\n", scheduler_type_to_string(scheduler_get_type()));

    // Creer plusieurs processus avec priorites differentes
    printf("Creation de 4 processus avec priorites variees...\n\n");

    struct {
        const char* name;
        uint32_t priority;
        uint32_t burst;
    } procs[] = {
        {"HighPrio", 30, 30},
        {"MedPrio", 20, 50},
        {"LowPrio", 10, 70},
        {"VeryLow", 5, 100}
    };

    for (int i = 0; i < 4; i++) {
        uint32_t pid = process_create(procs[i].name, demo_process_medium, procs[i].priority);
        if (pid > 0) {
            process_t* p = process_get_by_pid(pid);
            if (p) {
                p->burst_time = procs[i].burst;
                p->remaining_time = procs[i].burst;
                scheduler_add_process(p);
                printf("  %s: PID=%u, prio=%u, burst=%u\n",
                       procs[i].name, pid, procs[i].priority, procs[i].burst);
            }
        }
    }

    printf("\nCommandes utiles:\n");
    printf("  ps     - voir les processus\n");
    printf("  queue  - voir la file READY\n");
    printf("  log    - voir le journal\n");
    printf("  sched <type> - changer d'ordonnanceur\n");
    printf("  simulate <ticks> - simuler l'ordonnancement\n");
    printf("\n");
}

// Declaration externe
extern void scheduler_simulate(uint32_t ticks);

static void cmd_simulate(int argc, char** argv) {
    uint32_t ticks = 100;  // Par defaut: 100 ticks

    if (argc >= 2) {
        ticks = string_to_uint(argv[1]);
        if (ticks == 0) ticks = 100;
        if (ticks > 1000) ticks = 1000;  // Limite max
    }

    scheduler_simulate(ticks);
}

static void cmd_states(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Demonstration des etats ===\n\n");
    printf("Etats possibles: NEW -> READY -> RUNNING -> BLOCKED -> TERMINATED\n\n");

    // Creer un processus
    uint32_t pid = process_create("StateDemo", demo_process_short, 15);
    if (pid == 0) {
        printf("Erreur creation processus\n");
        return;
    }

    process_t* p = process_get_by_pid(pid);
    if (p == NULL) {
        printf("Processus introuvable\n");
        return;
    }

    printf("1. Processus cree (PID=%u): etat=%s\n", pid, process_state_to_string(p->state));

    // Ajouter a la file READY
    //scheduler_add_process(p);
    printf("2. Ajoute a la file:        etat=%s\n", process_state_to_string(p->state));

    printf("\nUtilisez 'block %u' puis 'unblock %u' pour tester BLOCKED\n", pid, pid);
    printf("Utilisez 'kill %u' pour terminer le processus\n\n", pid);
}

static void cmd_block(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: block <pid>\n");
        return;
    }

    uint32_t pid = string_to_uint(argv[1]);
    process_t* p = process_get_by_pid(pid);

    if (p == NULL) {
        printf("Processus PID=%u introuvable\n", pid);
        return;
    }

    if (p->state == PROCESS_STATE_BLOCKED) {
        printf("Processus deja bloque\n");
        return;
    }

    p->block_reason = BLOCK_REASON_SLEEP;
    scheduler_block_process(p);
    printf("Processus PID=%u bloque\n", pid);
}

static void cmd_unblock(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: unblock <pid>\n");
        return;
    }

    uint32_t pid = string_to_uint(argv[1]);
    process_t* p = process_get_by_pid(pid);

    if (p == NULL) {
        printf("Processus PID=%u introuvable\n", pid);
        return;
    }

    if (p->state != PROCESS_STATE_BLOCKED) {
        printf("Processus non bloque (etat=%s)\n", process_state_to_string(p->state));
        return;
    }

    scheduler_unblock_process(p);
    printf("Processus PID=%u debloque\n", pid);
}

// =============================================================================
// Commandes memoire
// =============================================================================

static void cmd_mem(int argc, char** argv) {
    if (argc < 2) {
        memory_print_stats();
        return;
    }

    if (string_compare(argv[1], "test") == 0) {
        memory_test();
    } else if (string_compare(argv[1], "stats") == 0) {
        memory_print_stats();
    } else if (string_compare(argv[1], "bitmap") == 0) {
        memory_print_bitmap();
    } else {
        printf("Usage: mem [test|stats|bitmap]\n");
    }
}

// =============================================================================
// Commandes IPC (mailboxes)
// =============================================================================

static void cmd_mbox(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mbox <list|create|send|recv|test>\n");
        return;
    }

    if (string_compare(argv[1], "list") == 0) {
        ipc_print_mailboxes();

    } else if (string_compare(argv[1], "create") == 0) {
        if (argc < 3) {
            printf("Usage: mbox create <nom>\n");
            return;
        }
        int32_t id = mbox_create(argv[2]);
        if (id >= 0) {
            printf("Mailbox '%s' creee (ID=%d)\n", argv[2], id);
        } else {
            printf("Erreur creation: %d\n", id);
        }

    } else if (string_compare(argv[1], "send") == 0) {
        if (argc < 4) {
            printf("Usage: mbox send <id> <message>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        int32_t r = mbox_send(id, argv[3], 32);
        if (r == IPC_SUCCESS) {
            printf("Message envoye\n");
        } else if (r == IPC_ERROR_FULL) {
            printf("Erreur: mailbox pleine\n");
        } else {
            printf("Erreur: %d\n", r);
        }

    } else if (string_compare(argv[1], "recv") == 0) {
        if (argc < 3) {
            printf("Usage: mbox recv <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        char buffer[64];
        uint32_t size, sender;
        int32_t r = mbox_recv(id, buffer, sizeof(buffer) - 1, &size, &sender);
        if (r == IPC_SUCCESS) {
            buffer[size] = '\0';
            printf("Message recu: '%s' (de PID=%u)\n", buffer, sender);
        } else if (r == IPC_ERROR_EMPTY) {
            printf("Mailbox vide\n");
        } else {
            printf("Erreur: %d\n", r);
        }

    } else if (string_compare(argv[1], "test") == 0) {
        ipc_test();

    } else {
        printf("Commande mbox inconnue: %s\n", argv[1]);
    }
}

// =============================================================================
// Commandes synchronisation (mutex)
// =============================================================================

static void cmd_mutex(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mutex <list|create|lock|unlock|test>\n");
        return;
    }

    if (string_compare(argv[1], "list") == 0) {
        mutex_print_all();

    } else if (string_compare(argv[1], "create") == 0) {
        if (argc < 3) {
            printf("Usage: mutex create <nom>\n");
            return;
        }
        int32_t id = mutex_create(argv[2]);
        if (id >= 0) {
            printf("Mutex '%s' cree (ID=%d)\n", argv[2], id);
        } else {
            printf("Erreur creation: %d\n", id);
        }

    } else if (string_compare(argv[1], "lock") == 0) {
        if (argc < 3) {
            printf("Usage: mutex lock <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        int32_t r = mutex_trylock(id);
        if (r == SYNC_SUCCESS) {
            printf("Mutex verrouille\n");
        } else if (r == SYNC_ERROR_BUSY) {
            printf("Mutex deja pris\n");
        } else {
            printf("Erreur: %d\n", r);
        }

    } else if (string_compare(argv[1], "unlock") == 0) {
        if (argc < 3) {
            printf("Usage: mutex unlock <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        int32_t r = mutex_unlock(id);
        if (r == SYNC_SUCCESS) {
            printf("Mutex deverrouille\n");
        } else {
            printf("Erreur: %d\n", r);
        }

    } else if (string_compare(argv[1], "test") == 0) {
        mutex_test();

    } else {
        printf("Commande mutex inconnue: %s\n", argv[1]);
    }
}

// =============================================================================
// Commandes synchronisation (semaphore)
// =============================================================================

static void cmd_sem(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: sem <list|create|wait|post|test>\n");
        return;
    }

    if (string_compare(argv[1], "list") == 0) {
        sem_print_all();

    } else if (string_compare(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: sem create <nom> <valeur_initiale>\n");
            return;
        }
        int32_t value = (int32_t)string_to_uint(argv[3]);
        int32_t id = sem_create(argv[2], value);
        if (id >= 0) {
            printf("Semaphore '%s' cree (ID=%d, value=%d)\n", argv[2], id, value);
        } else {
            printf("Erreur creation: %d\n", id);
        }

    } else if (string_compare(argv[1], "wait") == 0) {
        if (argc < 3) {
            printf("Usage: sem wait <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        int32_t r = sem_trywait(id);
        if (r == SYNC_SUCCESS) {
            printf("Wait OK, nouvelle valeur: %d\n", sem_getvalue(id));
        } else if (r == SYNC_ERROR_WOULDBLOCK) {
            printf("Semaphore a 0 (bloquerait)\n");
        } else {
            printf("Erreur: %d\n", r);
        }

    } else if (string_compare(argv[1], "post") == 0) {
        if (argc < 3) {
            printf("Usage: sem post <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        int32_t r = sem_post(id);
        if (r == SYNC_SUCCESS) {
            printf("Post OK, nouvelle valeur: %d\n", sem_getvalue(id));
        } else {
            printf("Erreur: %d\n", r);
        }

    } else if (string_compare(argv[1], "test") == 0) {
        sem_test();

    } else {
        printf("Commande sem inconnue: %s\n", argv[1]);
    }
}

// =============================================================================
// Table des commandes
// =============================================================================

typedef void (*shell_command_fn)(int argc, char** argv);

typedef struct {
    const char* name;
    const char* description;
    shell_command_fn handler;
} shell_command_t;

static const shell_command_t builtin_commands[] = {
    // Commandes de base
    { "help",    "Affiche l'aide",                   cmd_help },
    { "clear",   "Efface l'ecran",                   cmd_clear },
    { "info",    "Informations systeme",             cmd_info },
    { "uptime",  "Temps d'execution",                cmd_uptime },
    { "reboot",  "Redemarre le systeme",             cmd_reboot },

    // Gestion processus
    { "ps",      "Liste les processus",              cmd_ps },
    { "kill",    "Termine un processus",             cmd_kill },
    { "spawn",   "Cree des processus",               cmd_spawn },
    { "bench",   "Lance un benchmark",               cmd_bench },
    { "demo",    "Demo ordonnancement",              cmd_demo },
    { "states",  "Demo des etats",                   cmd_states },
    { "block",   "Bloque un processus",              cmd_block },
    { "unblock", "Debloque un processus",            cmd_unblock },

    // Ordonnancement
    { "sched",   "Change l'ordonnanceur",            cmd_sched },
    { "log",     "Journal d'execution",              cmd_log },
    { "queue",   "Affiche la file READY",            cmd_queue },
    { "simulate","Simule l'ordonnancement",          cmd_simulate },

    // Memoire
    { "mem",     "Gestion memoire",                  cmd_mem },

    // IPC
    { "mbox",    "Gestion mailboxes",                cmd_mbox },

    // Synchronisation
    { "mutex",   "Gestion mutex",                    cmd_mutex },
    { "sem",     "Gestion semaphores",               cmd_sem },

    { NULL, NULL, NULL }
};

// =============================================================================
// Implementation du shell
// =============================================================================

void shell_init(void) {
    printf("[SHELL] Initialisation du mini-shell...\n");

    input_pos = 0;

    for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
        input_buffer[i] = 0;
    }

    // Initialiser l'historique
    history_count = 0;
    history_index = 0;
    history_write_pos = 0;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i][0] = '\0';
    }

    printf("[SHELL] Shell initialise\n");
}

void shell_execute(const char* command) {
    static char cmd_buffer[SHELL_BUFFER_SIZE];
    string_copy_n(cmd_buffer, command, SHELL_BUFFER_SIZE);

    int argc = parse_command(cmd_buffer, argv_buffer, SHELL_MAX_ARGS);

    if (argc == 0) {
        return;
    }

    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        if (string_compare(argv_buffer[0], builtin_commands[i].name) == 0) {
            builtin_commands[i].handler(argc, argv_buffer);
            return;
        }
    }

    printf("Commande inconnue: %s\n", argv_buffer[0]);
    printf("Tapez 'help' pour la liste des commandes\n");
}

// Efface la ligne de commande actuelle et affiche une nouvelle commande
static void shell_replace_line(const char* new_cmd) {
    // Effacer les caracteres affiches
    while (input_pos > 0) {
        printf("\b \b");
        input_pos--;
    }

    // Copier la nouvelle commande
    string_copy_n(input_buffer, new_cmd, SHELL_BUFFER_SIZE);
    input_pos = string_length(input_buffer);

    // Afficher la nouvelle commande
    printf("%s", input_buffer);
}

// Declaration des fonctions de scrollback du terminal (definies dans kernel.c)
extern void terminal_scroll_up(size_t lines);
extern void terminal_scroll_down(size_t lines);
extern void terminal_scroll_to_top(void);
extern void terminal_scroll_to_bottom(void);

void shell_run(void) {
    printf("\n");
    printf("=========================================================\n");
    printf("       Bienvenue dans myos-i686 Mini-Shell v0.9        \n");
    printf("=========================================================\n");
    printf("       OS complet avec ordonnancement avance,           \n");
    printf("       gestion memoire, IPC et synchronisation          \n");
    printf("=========================================================\n");
    printf("\n");
    printf("Tapez 'help' pour la liste des commandes\n");
    printf("Fleches haut/bas: historique des commandes\n");
    printf("Page Up/Down: defiler l'ecran | Home/End: debut/fin\n\n");

    printf("myos-i686 shell > ");

    while (1) {
         unsigned char c = keyboard_getchar();

        if (c == '\n') {
            printf("\n");

            input_buffer[input_pos] = '\0';

            if (input_pos > 0) {
                // Ajouter a l'historique avant d'executer
                history_add(input_buffer);
                shell_execute(input_buffer);
            }

            // Reinitialiser pour la prochaine commande
            input_pos = 0;
            for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
                input_buffer[i] = 0;
            }
            history_reset_navigation();

            printf("myos-i686 shell > ");

        } else if (c == '\b') {
            if (input_pos > 0) {
                input_pos--;
                input_buffer[input_pos] = '\0';
                printf("\b \b");
            }

        } else if (c == CHAR_ARROW_UP) {
            // Fleche haut: commande precedente dans l'historique
            const char* prev = history_get_prev();
            if (prev != NULL) {
                shell_replace_line(prev);
            }

        } else if (c == CHAR_ARROW_DOWN) {
            // Fleche bas: commande suivante dans l'historique
            const char* next = history_get_next();
            if (next != NULL) {
                shell_replace_line(next);
            }

        } else if (c == CHAR_PAGE_UP) {
            // Page Up: remonter dans l'historique de l'ecran (10 lignes)
            terminal_scroll_up(10);

        } else if (c == CHAR_PAGE_DOWN) {
            // Page Down: descendre dans l'historique de l'ecran (10 lignes)
            terminal_scroll_down(10);

        } else if (c == CHAR_HOME) {
            // Home: aller au debut de l'historique
            terminal_scroll_to_top();

        } else if (c == CHAR_END) {
            // End: aller a la fin (vue actuelle)
            terminal_scroll_to_bottom();

        } else if (c >= 32 && c < 127) {
            if (input_pos < SHELL_BUFFER_SIZE - 1) {
                input_buffer[input_pos++] = c;
                printf("%c", c);
            }
        }
    }
}
