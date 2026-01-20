/*
 * shell.c - Implementation du mini-shell (version ASCII)
 */

#include "shell.h"
#include "keyboard.h"
#include "printf.h"
#include "process.h"
#include "scheduler.h"
#include "timer.h"

// =============================================================================
// Variables globales
// =============================================================================

static char input_buffer[SHELL_BUFFER_SIZE];
static uint32_t input_pos = 0;

static char* argv_buffer[SHELL_MAX_ARGS];

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
// Commandes intégrées
// =============================================================================

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n=== Mini-Shell myos-i686 ===\n\n");
    printf("Commandes disponibles:\n");
    printf("  help         - Affiche cette aide\n");
    printf("  clear        - Efface l'ecran\n");
    printf("  ps           - Liste les processus\n");
    printf("  kill <pid>   - Termine un processus\n");
    printf("  sched <type> - Change l'ordonnanceur (fcfs/rr)\n");
    printf("  log          - Affiche le journal d'execution\n");
    printf("  queue        - Affiche la file READY\n");
    printf("  uptime       - Affiche le temps d'execution\n");
    printf("  info         - Informations systeme\n");
    printf("  reboot       - Redemarrage (simulation)\n");
    printf("\n");
}

static void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    extern void terminal_clear(void);
    terminal_clear();
    printf("myos-i686 shell > ");
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
    
    uint32_t pid = 0;
    char* p = argv[1];
    while (*p >= '0' && *p <= '9') {
        pid = pid * 10 + (*p - '0');
        p++;
    }
    
    if (pid == 0) {
        printf("PID invalide\n");
        return;
    }
    
    printf("Tentative de terminaison du processus PID=%u...\n", pid);
    
    if (process_kill(pid)) {
        printf("Processus termine avec succes\n");
    } else {
        printf("Echec: processus introuvable\n");
    }
}

static void cmd_sched(int argc, char** argv) {
    if (argc < 2) {
        printf("Ordonnanceur actuel: %s\n", scheduler_type_to_string(scheduler_get_type()));
        printf("Usage: sched <fcfs|rr>\n");
        return;
    }
    
    if (string_compare(argv[1], "fcfs") == 0) {
        scheduler_set_type(SCHEDULER_FCFS);
        printf("Ordonnanceur change: FCFS\n");
    } else if (string_compare(argv[1], "rr") == 0) {
        scheduler_set_type(SCHEDULER_ROUND_ROBIN);
        printf("Ordonnanceur change: Round Robin\n");
    } else {
        printf("Type inconnu. Utilisez: fcfs ou rr\n");
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
    printf("OS: myos-i686 v0.7\n");
    printf("Architecture: x86 (32-bit)\n");
    printf("Compilateur: i686-elf-gcc\n");
    printf("Timer: 100 Hz (10ms/tick)\n");
    printf("Ordonnanceur: %s\n", scheduler_type_to_string(scheduler_get_type()));
    printf("Processus actifs: %u / %u\n", process_count(), 32);
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
// Table des commandes
// =============================================================================

typedef void (*shell_command_fn)(int argc, char** argv);

typedef struct {
    const char* name;
    const char* description;
    shell_command_fn handler;
} shell_command_t;

static const shell_command_t builtin_commands[] = {
    { "help",   "Affiche l'aide",                   cmd_help },
    { "clear",  "Efface l'ecran",                   cmd_clear },
    { "ps",     "Liste les processus",              cmd_ps },
    { "kill",   "Termine un processus",             cmd_kill },
    { "sched",  "Change l'ordonnanceur",            cmd_sched },
    { "log",    "Journal d'execution",              cmd_log },
    { "queue",  "Affiche la file READY",            cmd_queue },
    { "uptime", "Temps d'execution",                cmd_uptime },
    { "info",   "Informations systeme",             cmd_info },
    { "reboot", "Redemarre le systeme",             cmd_reboot },
    { NULL, NULL, NULL }
};

// =============================================================================
// Implémentation du shell
// =============================================================================

void shell_init(void) {
    printf("[SHELL] Initialisation du mini-shell...\n");
    
    input_pos = 0;
    
    for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
        input_buffer[i] = 0;
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

void shell_run(void) {
    printf("\n");
    printf("============================================================\n");
    printf("         Bienvenue dans myos-i686 Mini-Shell !             \n");
    printf("============================================================\n");
    printf("\n");
    printf("Tapez 'help' pour la liste des commandes\n\n");
    
    printf("myos-i686 shell > ");
    
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
            
            printf("myos-i686 shell > ");
            
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
