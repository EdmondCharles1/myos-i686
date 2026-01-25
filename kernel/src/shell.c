/*
 * shell.c - Mini-shell interactif complet
 *
 * Commandes pour démontrer :
 * - Gestion de processus (spawn, block, unblock, kill)
 * - Ordonnancement (sched, log, queue)
 * - Tests et benchmarks (bench, demo)
 */

#include "shell.h"
#include "keyboard.h"
#include "printf.h"
#include "process.h"
#include "scheduler.h"
#include "timer.h"
#include "types.h"
#include "ipc.h"
#include "sync.h"
#include "memory.h"

// =============================================================================
// Variables globales
// =============================================================================

static char input_buffer[SHELL_BUFFER_SIZE];
static uint32_t input_pos = 0;

static char* argv_buffer[SHELL_MAX_ARGS];

// Compteur pour nommer les processus de test
static uint32_t test_process_counter = 0;

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
// Fonctions de processus de test
// =============================================================================

// Processus de test qui "calcule" pendant quelques ticks
static void test_process_func(void) {
    volatile uint32_t counter = 0;
    // Simuler du travail
    for (uint32_t i = 0; i < 1000000; i++) {
        counter++;
    }
    // Le processus se termine naturellement
    process_exit();
}

// Processus CPU-bound (calcul intensif)
static void cpu_bound_process(void) {
    volatile uint32_t counter = 0;
    while (1) {
        counter++;
        // Simuler un travail CPU-bound
        if (counter % 100000 == 0) {
            // Point de préemption
            __asm__ volatile ("nop");
        }
    }
}

// Processus I/O-bound (beaucoup d'attente)
static void io_bound_process(void) {
    while (1) {
        // Simuler une attente I/O
        timer_wait(5);  // Attendre 50ms
    }
}

// =============================================================================
// Commandes intégrées - AIDE
// =============================================================================

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("=========================================\n");
    printf("     Mini-Shell myos-i686 v1.0\n");
    printf("=========================================\n\n");

    printf("COMMANDES DE BASE:\n");
    printf("  help           Affiche cette aide\n");
    printf("  clear          Efface l'ecran\n");
    printf("  info           Informations systeme\n");
    printf("  uptime         Temps depuis le demarrage\n");
    printf("\n");

    printf("GESTION DES PROCESSUS:\n");
    printf("  ps             Liste tous les processus\n");
    printf("  spawn [nom]    Cree un processus de test\n");
    printf("  kill <pid>     Termine un processus\n");
    printf("  block <pid>    Bloque un processus\n");
    printf("  unblock <pid>  Debloque un processus\n");
    printf("\n");

    printf("ORDONNANCEMENT (6 algorithmes):\n");
    printf("  sched [type]   Affiche/change l'ordonnanceur\n");
    printf("                 fcfs, rr, priority, sjf, srtf, mlfq\n");
    printf("  queue          Affiche la file READY\n");
    printf("  log            Journal d'execution\n");
    printf("\n");

    printf("IPC (Inter-Process Communication):\n");
    printf("  mbox <cmd>     Mailbox: create, list, send, recv, delete\n");
    printf("\n");

    printf("SYNCHRONISATION:\n");
    printf("  mutex <cmd>    Mutex: create, lock, unlock, list, delete\n");
    printf("  sem <cmd>      Semaphore: create, wait, signal, list, delete\n");
    printf("\n");

    printf("MEMOIRE:\n");
    printf("  mem <cmd>      Memoire: stats, map, test\n");
    printf("\n");

    printf("TESTS ET DEMOS:\n");
    printf("  bench <n>      Lance n processus de test\n");
    printf("  demo           Demo complete du scheduler\n");
    printf("  states         Montre les transitions d'etats\n");
    printf("\n");
}

// =============================================================================
// Commandes intégrées - SYSTEME
// =============================================================================

static void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;

    extern void terminal_clear(void);
    terminal_clear();
}

static void cmd_info(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Informations Systeme ===\n\n");
    printf("OS:            myos-i686 v1.0\n");
    printf("Architecture:  x86 (i686, 32-bit)\n");
    printf("Timer:         PIT @ 100 Hz (10ms/tick)\n");
    printf("Ordonnanceur:  %s\n", scheduler_type_to_string(scheduler_get_type()));
    printf("Processus:     %u actifs / %u max\n", process_count(), 32);
    printf("Uptime:        %u ms (%u ticks)\n",
           (uint32_t)timer_get_ms(), (uint32_t)timer_get_ticks());
    printf("\n");
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

    printf("Uptime: %uh %um %us (%u ticks)\n",
           hours, minutes, seconds, (uint32_t)timer_get_ticks());
}

// =============================================================================
// Commandes intégrées - PROCESSUS
// =============================================================================

static void cmd_ps(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    process_list();
}

static void cmd_spawn(int argc, char** argv) {
    char name[32];

    if (argc >= 2) {
        string_copy_n(name, argv[1], 32);
    } else {
        // Générer un nom automatique
        name[0] = 'p';
        name[1] = 'r';
        name[2] = 'o';
        name[3] = 'c';
        name[4] = '_';
        uint32_t n = test_process_counter++;
        if (n < 10) {
            name[5] = '0' + n;
            name[6] = '\0';
        } else {
            name[5] = '0' + (n / 10);
            name[6] = '0' + (n % 10);
            name[7] = '\0';
        }
    }

    printf("Creation du processus '%s'...\n", name);

    uint32_t pid = process_create(name, test_process_func, PRIORITY_DEFAULT);

    if (pid != 0) {
        process_t* proc = process_get_by_pid(pid);
        if (proc) {
            scheduler_add_process(proc);
        }
        printf("Processus cree avec PID=%u\n", pid);
    } else {
        printf("Erreur: impossible de creer le processus\n");
    }
}

static void cmd_kill(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return;
    }

    uint32_t pid = string_to_uint(argv[1]);

    if (pid == 0) {
        printf("Erreur: PID invalide\n");
        return;
    }

    if (pid == 1) {
        printf("Erreur: impossible de tuer le processus idle (PID=1)\n");
        return;
    }

    process_t* proc = process_get_by_pid(pid);
    if (proc == NULL) {
        printf("Erreur: processus PID=%u non trouve\n", pid);
        return;
    }

    printf("Terminaison du processus '%s' (PID=%u)...\n", proc->name, pid);

    // Retirer du scheduler si dans la queue
    scheduler_remove_process(proc);

    if (process_kill(pid)) {
        printf("Processus termine\n");
    } else {
        printf("Echec de la terminaison\n");
    }
}

static void cmd_block(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: block <pid>\n");
        return;
    }

    uint32_t pid = string_to_uint(argv[1]);

    if (pid == 0) {
        printf("Erreur: PID invalide\n");
        return;
    }

    process_t* proc = process_get_by_pid(pid);
    if (proc == NULL) {
        printf("Erreur: processus PID=%u non trouve\n", pid);
        return;
    }

    if (proc->state == PROCESS_STATE_BLOCKED) {
        printf("Processus deja bloque\n");
        return;
    }

    if (proc->state == PROCESS_STATE_TERMINATED) {
        printf("Erreur: processus deja termine\n");
        return;
    }

    printf("Blocage du processus '%s' (PID=%u)...\n", proc->name, pid);

    // Retirer de la file READY
    scheduler_remove_process(proc);

    // Changer l'état
    process_set_state(proc, PROCESS_STATE_BLOCKED);

    printf("Processus bloque (etait: %s)\n",
           proc->state == PROCESS_STATE_RUNNING ? "RUNNING" : "READY");
}

static void cmd_unblock(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: unblock <pid>\n");
        return;
    }

    uint32_t pid = string_to_uint(argv[1]);

    if (pid == 0) {
        printf("Erreur: PID invalide\n");
        return;
    }

    process_t* proc = process_get_by_pid(pid);
    if (proc == NULL) {
        printf("Erreur: processus PID=%u non trouve\n", pid);
        return;
    }

    if (proc->state != PROCESS_STATE_BLOCKED) {
        printf("Erreur: processus n'est pas bloque (etat: %s)\n",
               process_state_to_string(proc->state));
        return;
    }

    printf("Deblocage du processus '%s' (PID=%u)...\n", proc->name, pid);

    // Remettre dans la file READY
    scheduler_add_process(proc);

    printf("Processus debloque et remis en file READY\n");
}

// =============================================================================
// Commandes intégrées - ORDONNANCEMENT
// =============================================================================

static void cmd_sched(int argc, char** argv) {
    if (argc < 2) {
        printf("Ordonnanceur actuel: %s\n", scheduler_type_to_string(scheduler_get_type()));
        printf("\nTypes disponibles:\n");
        printf("  fcfs     - First Come First Served (non preemptif)\n");
        printf("  rr       - Round Robin (preemptif, quantum=10 ticks)\n");
        printf("  priority - Par priorite (preemptif)\n");
        printf("  sjf      - Shortest Job First (non preemptif)\n");
        printf("  srtf     - Shortest Remaining Time First (preemptif)\n");
        printf("  mlfq     - Multi-Level Feedback Queue (3 niveaux)\n");
        printf("\nUsage: sched <type>\n");
        return;
    }

    if (string_compare(argv[1], "fcfs") == 0) {
        scheduler_set_type(SCHEDULER_FCFS);
        printf("Ordonnanceur: FCFS (First Come First Served)\n");
        printf("  Non preemptif - processus s'executent jusqu'a completion\n");
    } else if (string_compare(argv[1], "rr") == 0) {
        scheduler_set_type(SCHEDULER_ROUND_ROBIN);
        printf("Ordonnanceur: Round Robin\n");
        printf("  Preemptif - quantum de 10 ticks (100ms)\n");
    } else if (string_compare(argv[1], "priority") == 0) {
        scheduler_set_type(SCHEDULER_PRIORITY);
        printf("Ordonnanceur: Priority\n");
        printf("  Preemptif - processus haute priorite en premier\n");
    } else if (string_compare(argv[1], "sjf") == 0) {
        scheduler_set_type(SCHEDULER_SJF);
        printf("Ordonnanceur: SJF (Shortest Job First)\n");
        printf("  Non preemptif - plus court job estime d'abord\n");
    } else if (string_compare(argv[1], "srtf") == 0) {
        scheduler_set_type(SCHEDULER_SRTF);
        printf("Ordonnanceur: SRTF (Shortest Remaining Time)\n");
        printf("  Preemptif - plus court temps restant d'abord\n");
    } else if (string_compare(argv[1], "mlfq") == 0) {
        scheduler_set_type(SCHEDULER_MLFQ);
        printf("Ordonnanceur: MLFQ (Multi-Level Feedback Queue)\n");
        printf("  3 niveaux avec quantum croissant (2, 4, 8 ticks)\n");
    } else {
        printf("Type inconnu: '%s'\n", argv[1]);
        printf("Types: fcfs, rr, priority, sjf, srtf, mlfq\n");
    }
}

static void cmd_queue(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    scheduler_print_queue();
}

static void cmd_log(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    scheduler_print_log();
}

// =============================================================================
// Commandes intégrées - TESTS ET DEMOS
// =============================================================================

static void cmd_bench(int argc, char** argv) {
    uint32_t n = 3;  // Par défaut, 3 processus

    if (argc >= 2) {
        n = string_to_uint(argv[1]);
        if (n == 0 || n > 10) {
            printf("Erreur: n doit etre entre 1 et 10\n");
            return;
        }
    }

    printf("\n=== Benchmark: Creation de %u processus ===\n\n", n);
    printf("Ordonnanceur: %s\n\n", scheduler_type_to_string(scheduler_get_type()));

    uint32_t created = 0;
    char name[32];

    for (uint32_t i = 0; i < n; i++) {
        // Générer le nom
        name[0] = 't';
        name[1] = 'e';
        name[2] = 's';
        name[3] = 't';
        name[4] = '_';
        name[5] = 'A' + i;
        name[6] = '\0';

        // Priorité variable pour démontrer le scheduler priority
        uint32_t priority = PRIORITY_DEFAULT + (i % 5) - 2;

        uint32_t pid = process_create(name, cpu_bound_process, priority);

        if (pid != 0) {
            process_t* proc = process_get_by_pid(pid);
            if (proc) {
                scheduler_add_process(proc);
                printf("  + PID=%u '%s' priorite=%u\n", pid, name, priority);
                created++;
            }
        }
    }

    printf("\n%u processus crees sur %u demandes\n", created, n);
    printf("\nUtilisez 'ps' pour voir la liste\n");
    printf("Utilisez 'log' apres quelques secondes pour voir les context switches\n\n");
}

static void cmd_demo(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("============================================\n");
    printf("   DEMONSTRATION DU SCHEDULER myos-i686\n");
    printf("============================================\n\n");

    printf("Cette demo va:\n");
    printf("  1. Creer 3 processus avec des priorites differentes\n");
    printf("  2. Montrer les etats et transitions\n");
    printf("  3. Permettre de tester FCFS vs RR vs Priority\n\n");

    printf("Processus actuels:\n");
    process_list();

    printf("\nCreation de 3 processus de test...\n\n");

    // Créer 3 processus avec priorités différentes
    char* names[] = {"highprio", "medprio", "lowprio"};
    uint32_t priorities[] = {25, 15, 5};

    for (int i = 0; i < 3; i++) {
        uint32_t pid = process_create(names[i], cpu_bound_process, priorities[i]);
        if (pid != 0) {
            process_t* proc = process_get_by_pid(pid);
            if (proc) {
                scheduler_add_process(proc);
                printf("  + PID=%u '%s' priorite=%u\n", pid, names[i], priorities[i]);
            }
        }
    }

    printf("\n");
    printf("Processus apres creation:\n");
    process_list();

    printf("\nCommandes utiles pour la demo:\n");
    printf("  sched fcfs   - Passer en FCFS\n");
    printf("  sched rr     - Passer en Round Robin\n");
    printf("  sched priority - Passer en Priority\n");
    printf("  queue        - Voir la file READY\n");
    printf("  log          - Voir le journal d'execution\n");
    printf("  block <pid>  - Bloquer un processus\n");
    printf("  unblock <pid>- Debloquer un processus\n");
    printf("\n");
}

static void cmd_states(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== Demonstration des etats de processus ===\n\n");

    printf("Etats possibles:\n");
    printf("  NEW        -> Processus cree, pas encore initialise\n");
    printf("  READY      -> Pret a s'executer, dans la file\n");
    printf("  RUNNING    -> En cours d'execution\n");
    printf("  BLOCKED    -> Bloque (attente I/O, sleep, etc.)\n");
    printf("  TERMINATED -> Termine\n\n");

    printf("Transitions:\n");
    printf("  NEW -> READY       : process_create() termine\n");
    printf("  READY -> RUNNING   : scheduler selectionne le processus\n");
    printf("  RUNNING -> READY   : preemption (quantum epuise)\n");
    printf("  RUNNING -> BLOCKED : attente I/O ou sleep\n");
    printf("  BLOCKED -> READY   : I/O complete ou reveil\n");
    printf("  RUNNING -> TERMINATED : process_exit() ou kill\n\n");

    printf("Creation d'un processus pour demonstrer...\n");

    uint32_t pid = process_create("state_demo", test_process_func, PRIORITY_DEFAULT);
    if (pid == 0) {
        printf("Erreur: impossible de creer le processus\n");
        return;
    }

    process_t* proc = process_get_by_pid(pid);
    if (proc == NULL) {
        printf("Erreur: processus non trouve\n");
        return;
    }

    printf("  PID=%u cree, etat=%s\n", pid, process_state_to_string(proc->state));

    // Ajouter au scheduler (READY)
    scheduler_add_process(proc);
    printf("  Ajoute au scheduler, etat=%s\n", process_state_to_string(proc->state));

    // Bloquer
    scheduler_remove_process(proc);
    process_set_state(proc, PROCESS_STATE_BLOCKED);
    printf("  Bloque, etat=%s\n", process_state_to_string(proc->state));

    // Débloquer
    process_set_state(proc, PROCESS_STATE_READY);
    scheduler_add_process(proc);
    printf("  Debloque, etat=%s\n", process_state_to_string(proc->state));

    printf("\nUtilisez 'ps' pour voir l'etat actuel des processus\n\n");
}

// =============================================================================
// Commandes IPC
// =============================================================================

static void cmd_mbox(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mbox <create|list|send|recv|delete> [args]\n");
        printf("  mbox create <nom>     - Cree une mailbox\n");
        printf("  mbox list             - Liste les mailboxes\n");
        printf("  mbox send <id> <msg>  - Envoie un message\n");
        printf("  mbox recv <id>        - Recoit un message\n");
        printf("  mbox delete <id>      - Supprime une mailbox\n");
        return;
    }

    if (string_compare(argv[1], "create") == 0) {
        if (argc < 3) {
            printf("Usage: mbox create <nom>\n");
            return;
        }
        uint32_t id = mailbox_create(argv[2]);
        if (id > 0) {
            printf("Mailbox creee: id=%u\n", id);
        }
    } else if (string_compare(argv[1], "list") == 0) {
        ipc_print_mailboxes();
    } else if (string_compare(argv[1], "send") == 0) {
        if (argc < 4) {
            printf("Usage: mbox send <id> <message>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (mailbox_send(id, 1, argv[3], 32)) {
            printf("Message envoye\n");
        } else {
            printf("Echec envoi\n");
        }
    } else if (string_compare(argv[1], "recv") == 0) {
        if (argc < 3) {
            printf("Usage: mbox recv <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        message_t msg;
        if (mailbox_receive(id, &msg)) {
            printf("Message recu: type=%u, data='%s'\n", msg.type, msg.data);
        } else {
            printf("Aucun message\n");
        }
    } else if (string_compare(argv[1], "delete") == 0) {
        if (argc < 3) {
            printf("Usage: mbox delete <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (mailbox_destroy(id)) {
            printf("Mailbox supprimee\n");
        } else {
            printf("Mailbox non trouvee\n");
        }
    } else {
        printf("Sous-commande inconnue: %s\n", argv[1]);
    }
}

// =============================================================================
// Commandes Synchronisation
// =============================================================================

static void cmd_mutex(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mutex <create|lock|unlock|list|delete> [args]\n");
        printf("  mutex create <nom>  - Cree un mutex\n");
        printf("  mutex lock <id>     - Verrouille (trylock)\n");
        printf("  mutex unlock <id>   - Deverrouille\n");
        printf("  mutex list          - Liste les mutex\n");
        printf("  mutex delete <id>   - Supprime un mutex\n");
        return;
    }

    if (string_compare(argv[1], "create") == 0) {
        if (argc < 3) {
            printf("Usage: mutex create <nom>\n");
            return;
        }
        uint32_t id = mutex_create(argv[2]);
        if (id > 0) {
            printf("Mutex cree: id=%u\n", id);
        }
    } else if (string_compare(argv[1], "lock") == 0) {
        if (argc < 3) {
            printf("Usage: mutex lock <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (mutex_trylock(id)) {
            printf("Mutex verrouille\n");
        } else {
            printf("Mutex deja pris\n");
        }
    } else if (string_compare(argv[1], "unlock") == 0) {
        if (argc < 3) {
            printf("Usage: mutex unlock <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (mutex_unlock(id)) {
            printf("Mutex deverrouille\n");
        } else {
            printf("Echec deverrouillage\n");
        }
    } else if (string_compare(argv[1], "list") == 0) {
        sync_print_mutexes();
    } else if (string_compare(argv[1], "delete") == 0) {
        if (argc < 3) {
            printf("Usage: mutex delete <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (mutex_destroy(id)) {
            printf("Mutex supprime\n");
        }
    } else {
        printf("Sous-commande inconnue: %s\n", argv[1]);
    }
}

static void cmd_sem(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: sem <create|wait|signal|list|delete> [args]\n");
        printf("  sem create <nom> <val>  - Cree un semaphore\n");
        printf("  sem wait <id>           - Wait (P) - decremente\n");
        printf("  sem signal <id>         - Signal (V) - incremente\n");
        printf("  sem list                - Liste les semaphores\n");
        printf("  sem delete <id>         - Supprime un semaphore\n");
        return;
    }

    if (string_compare(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: sem create <nom> <valeur_initiale>\n");
            return;
        }
        int32_t val = (int32_t)string_to_uint(argv[3]);
        uint32_t id = semaphore_create(argv[2], val, 0);
        if (id > 0) {
            printf("Semaphore cree: id=%u, valeur=%d\n", id, val);
        }
    } else if (string_compare(argv[1], "wait") == 0) {
        if (argc < 3) {
            printf("Usage: sem wait <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (semaphore_trywait(id)) {
            printf("Wait OK, nouvelle valeur=%d\n", semaphore_get_value(id));
        } else {
            printf("Wait impossible (valeur <= 0)\n");
        }
    } else if (string_compare(argv[1], "signal") == 0) {
        if (argc < 3) {
            printf("Usage: sem signal <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (semaphore_signal(id)) {
            printf("Signal OK, nouvelle valeur=%d\n", semaphore_get_value(id));
        } else {
            printf("Signal impossible\n");
        }
    } else if (string_compare(argv[1], "list") == 0) {
        sync_print_semaphores();
    } else if (string_compare(argv[1], "delete") == 0) {
        if (argc < 3) {
            printf("Usage: sem delete <id>\n");
            return;
        }
        uint32_t id = string_to_uint(argv[2]);
        if (semaphore_destroy(id)) {
            printf("Semaphore supprime\n");
        }
    } else {
        printf("Sous-commande inconnue: %s\n", argv[1]);
    }
}

// =============================================================================
// Commandes Memoire
// =============================================================================

static void cmd_mem(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mem <stats|map|test|alloc|free>\n");
        printf("  mem stats         - Affiche statistiques memoire\n");
        printf("  mem map           - Affiche carte memoire\n");
        printf("  mem test          - Test allocateur\n");
        return;
    }

    if (string_compare(argv[1], "stats") == 0) {
        memory_print_stats();
    } else if (string_compare(argv[1], "map") == 0) {
        memory_print_map();
    } else if (string_compare(argv[1], "test") == 0) {
        memory_test();
    } else {
        printf("Sous-commande inconnue: %s\n", argv[1]);
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
    // Base
    { "help",    "Affiche l'aide",              cmd_help },
    { "clear",   "Efface l'ecran",              cmd_clear },
    { "info",    "Informations systeme",        cmd_info },
    { "uptime",  "Temps depuis demarrage",      cmd_uptime },

    // Processus
    { "ps",      "Liste les processus",         cmd_ps },
    { "spawn",   "Cree un processus test",      cmd_spawn },
    { "kill",    "Termine un processus",        cmd_kill },
    { "block",   "Bloque un processus",         cmd_block },
    { "unblock", "Debloque un processus",       cmd_unblock },

    // Ordonnancement
    { "sched",   "Change l'ordonnanceur",       cmd_sched },
    { "queue",   "File READY",                  cmd_queue },
    { "log",     "Journal d'execution",         cmd_log },

    // Tests
    { "bench",   "Benchmark multi-processus",   cmd_bench },
    { "demo",    "Demo complete",               cmd_demo },
    { "states",  "Demo des etats",              cmd_states },

    // IPC
    { "mbox",    "Gestion mailboxes",           cmd_mbox },

    // Synchronisation
    { "mutex",   "Gestion mutex",               cmd_mutex },
    { "sem",     "Gestion semaphores",          cmd_sem },

    // Memoire
    { "mem",     "Gestion memoire",             cmd_mem },

    { NULL, NULL, NULL }
};

// =============================================================================
// Implémentation du shell
// =============================================================================

void shell_init(void) {
    printf("[SHELL] Initialisation du mini-shell...\n");

    input_pos = 0;
    test_process_counter = 0;

    for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
        input_buffer[i] = 0;
    }

    printf("[SHELL] Shell initialise (15 commandes)\n");
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

    printf("Commande inconnue: '%s'\n", argv_buffer[0]);
    printf("Tapez 'help' pour la liste des commandes\n");
}

void shell_run(void) {
    printf("\n");
    printf("============================================================\n");
    printf("         Bienvenue dans myos-i686 Mini-Shell !             \n");
    printf("============================================================\n");
    printf("\n");
    printf("Tapez 'help' pour la liste des commandes\n");
    printf("Tapez 'demo' pour une demonstration complete\n\n");

    printf("myos> ");

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            printf("\n");

            input_buffer[input_pos] = '\0';

            if (input_pos > 0) {
                shell_execute(input_buffer);
            }

            input_pos = 0;
            for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
                input_buffer[i] = 0;
            }

            printf("myos> ");

        } else if (c == '\b') {
            if (input_pos > 0) {
                input_pos--;
                input_buffer[input_pos] = '\0';
                printf("\b \b");
            }

        } else if (c >= 32 && c < 127) {
            if (input_pos < SHELL_BUFFER_SIZE - 1) {
                input_buffer[input_pos++] = c;
                printf("%c", c);
            }
        }
    }
}
