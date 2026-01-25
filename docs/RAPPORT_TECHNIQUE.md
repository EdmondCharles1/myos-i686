# Rapport Technique - myos-i686
## Mini-Système d'Exploitation x86 32 bits

---

**Projet universitaire - Systèmes d'exploitation**

**Version :** 1.0
**Date :** Janvier 2026
**Architecture :** i686 (x86 32 bits)

---

## Table des matières

1. Introduction
2. Architecture générale
3. Module de boot et initialisation
4. Gestion des interruptions
5. Gestion des processus
6. Ordonnancement
7. Communication inter-processus (IPC)
8. Primitives de synchronisation
9. Gestion de la mémoire
10. Interface utilisateur (Shell)
11. Tests et validation
12. Limites et améliorations futures
13. Conclusion

---

## 1. Introduction

### 1.1 Objectifs du projet

myos-i686 est un mini-système d'exploitation éducatif développé pour
démontrer les concepts fondamentaux des systèmes d'exploitation modernes :

- **Gestion de tâches** : Création, états et terminaison de processus
- **Ordonnancement** : Implémentation de 6 algorithmes différents
- **Communication** : IPC via mailboxes
- **Synchronisation** : Mutex et sémaphores
- **Mémoire** : Allocateur simple (pool allocator)

### 1.2 Environnement technique

| Composant | Spécification |
|-----------|---------------|
| Architecture cible | i686 (Intel 32 bits) |
| Compilateur | i686-linux-gnu-gcc |
| Assembleur | NASM |
| Émulateur | QEMU (qemu-system-i386) |
| Standard C | C11 freestanding |
| Boot | Multiboot (compatible GRUB) |

### 1.3 Structure du projet

```
myos-i686/
├── boot.asm              # Bootstrap Multiboot
├── Makefile              # Système de build
├── kernel/
│   └── src/
│       ├── kernel.c      # Point d'entrée principal
│       ├── printf.c/h    # Sortie formatée
│       ├── idt.c/h       # Table des descripteurs d'interruption
│       ├── isr.c/h       # Routines de service d'interruption
│       ├── irq.c/h       # Gestion des IRQ matérielles
│       ├── pic.c/h       # Contrôleur PIC 8259
│       ├── timer.c/h     # Timer PIT (100 Hz)
│       ├── process.c/h   # Gestion des processus
│       ├── scheduler.c/h # Ordonnanceur multi-algorithmes
│       ├── keyboard.c/h  # Driver clavier PS/2
│       ├── shell.c/h     # Interface shell interactive
│       ├── ipc.c/h       # Communication inter-processus
│       ├── sync.c/h      # Mutex et sémaphores
│       ├── memory.c/h    # Gestion mémoire
│       ├── types.h       # Types de base
│       └── linker.ld     # Script de liaison
└── docs/
    ├── DEMO_VIDEO.md     # Script de démonstration
    └── RAPPORT_TECHNIQUE.md
```

---

## 2. Architecture générale

### 2.1 Vue d'ensemble

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATIONS (Shell)                      │
├─────────────────────────────────────────────────────────────┤
│                    INTERFACE SYSTÈME                         │
│  ┌─────────┐ ┌──────────┐ ┌───────┐ ┌────────┐ ┌────────┐   │
│  │ Process │ │Scheduler │ │  IPC  │ │  Sync  │ │ Memory │   │
│  └─────────┘ └──────────┘ └───────┘ └────────┘ └────────┘   │
├─────────────────────────────────────────────────────────────┤
│                   COUCHE MATÉRIELLE                          │
│  ┌────────┐ ┌───────┐ ┌────────┐ ┌──────────┐               │
│  │  IDT   │ │  PIC  │ │ Timer  │ │ Keyboard │               │
│  └────────┘ └───────┘ └────────┘ └──────────┘               │
├─────────────────────────────────────────────────────────────┤
│                      BOOTSTRAP                               │
│                    (boot.asm - Multiboot)                    │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Flux d'initialisation

1. **BIOS/QEMU** charge le kernel via Multiboot
2. **boot.asm** configure la pile et appelle `kernel_main()`
3. **kernel_main()** initialise tous les sous-systèmes :
   - SSP (Stack Smashing Protection)
   - IDT (Interrupt Descriptor Table)
   - ISR (CPU exceptions 0-31)
   - IRQ (Hardware interrupts)
   - Timer (PIT à 100 Hz)
   - Process Manager
   - Scheduler
   - Memory Manager
   - IPC
   - Synchronization
   - Shell
4. Activation des interruptions (`sti`)
5. Lancement du shell interactif

---

## 3. Module de boot et initialisation

### 3.1 Header Multiboot

Le fichier `boot.asm` contient le header Multiboot standard :

```nasm
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002           ; Magic number
    dd 0x00000003           ; Flags (align + meminfo)
    dd -(0x1BADB002 + 0x03) ; Checksum
```

### 3.2 Point d'entrée

```nasm
section .text
global _start
extern kernel_main

_start:
    cli                     ; Désactiver interruptions
    mov esp, stack_space + 16384  ; Configurer pile (16KB)
    push 0
    popf                    ; Réinitialiser flags
    call kernel_main        ; Appeler le kernel
    cli
.halt_loop:
    hlt
    jmp .halt_loop
```

### 3.3 Script de liaison (linker.ld)

Le linker script place les sections à partir de 1MB :

```
ENTRY(_start)
SECTIONS {
    . = 1M;
    .text BLOCK(4K) : { *(.multiboot) *(.text) }
    .rodata BLOCK(4K) : { *(.rodata) *(.rodata.*) }
    .data BLOCK(4K) : { *(.data) }
    .bss BLOCK(4K) : { *(COMMON) *(.bss) *(.bss.*) }
}
```

---

## 4. Gestion des interruptions

### 4.1 IDT (Interrupt Descriptor Table)

L'IDT contient 256 entrées pour les exceptions CPU et les IRQ :

| Vecteur | Type | Description |
|---------|------|-------------|
| 0-31 | Exceptions CPU | Division par zéro, Page fault, etc. |
| 32-47 | IRQ matérielles | Timer, Clavier, etc. |
| 48-255 | Réservé | Syscalls potentiels |

### 4.2 Structure d'une entrée IDT

```c
typedef struct {
    uint16_t base_low;    // Bits 0-15 de l'adresse
    uint16_t selector;    // Sélecteur de segment (0x08)
    uint8_t  zero;        // Toujours 0
    uint8_t  flags;       // Type et privilèges
    uint16_t base_high;   // Bits 16-31 de l'adresse
} __attribute__((packed)) idt_entry_t;
```

### 4.3 PIC 8259 (Programmable Interrupt Controller)

Le PIC est remappe pour éviter les conflits avec les exceptions CPU :

- PIC1 (Master) : IRQ 0-7 → Vecteurs 32-39
- PIC2 (Slave) : IRQ 8-15 → Vecteurs 40-47

### 4.4 Timer PIT

Le Programmable Interval Timer génère une interruption à 100 Hz :

```c
void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);              // Mode 3 (square wave)
    outb(0x40, divisor & 0xFF);    // Low byte
    outb(0x40, divisor >> 8);      // High byte
    irq_register_handler(0, timer_handler);
}
```

---

## 5. Gestion des processus

### 5.1 PCB (Process Control Block)

Chaque processus est représenté par une structure PCB :

```c
typedef struct process {
    uint32_t pid;                 // Identifiant unique
    char name[32];                // Nom du processus
    process_state_t state;        // État courant
    uint32_t priority;            // Priorité (0-31)
    cpu_context_t context;        // Registres CPU
    uint32_t stack_base;          // Base de la pile
    uint32_t stack_size;          // Taille de la pile
    uint64_t total_ticks;         // CPU time consommé
    uint32_t time_slice;          // Quantum de temps
    uint32_t remaining_slice;     // Temps restant
    struct process* next;         // Chaînage pour les files
} process_t;
```

### 5.2 États des processus

```
     ┌─────────────────────────────────────────┐
     │                                         │
     ▼                                         │
   ┌─────┐    ┌───────┐    ┌─────────┐    ┌────────────┐
   │ NEW │───►│ READY │◄──►│ RUNNING │───►│ TERMINATED │
   └─────┘    └───────┘    └─────────┘    └────────────┘
                  ▲             │
                  │             ▼
              ┌───────────┐
              │  BLOCKED  │
              └───────────┘
```

### 5.3 Transitions d'états

| Transition | Déclencheur |
|------------|-------------|
| NEW → READY | `process_create()` termine |
| READY → RUNNING | Scheduler sélectionne le processus |
| RUNNING → READY | Préemption (quantum épuisé) |
| RUNNING → BLOCKED | Attente I/O, sleep, etc. |
| BLOCKED → READY | I/O terminée, réveil |
| RUNNING → TERMINATED | `process_exit()` ou kill |

---

## 6. Ordonnancement

### 6.1 Algorithmes implémentés

myos-i686 supporte 6 algorithmes d'ordonnancement :

| Algorithme | Type | Description |
|------------|------|-------------|
| FCFS | Non-préemptif | First Come First Served |
| Round Robin | Préemptif | Quantum de 10 ticks (100ms) |
| Priority | Préemptif | Plus haute priorité en premier |
| SJF | Non-préemptif | Shortest Job First |
| SRTF | Préemptif | Shortest Remaining Time First |
| MLFQ | Préemptif | Multi-Level Feedback Queue |

### 6.2 Structure de la file READY

```c
typedef struct {
    process_t* head;    // Premier processus
    process_t* tail;    // Dernier processus
    uint32_t count;     // Nombre de processus
} process_queue_t;
```

### 6.3 MLFQ (Multi-Level Feedback Queue)

Implémentation à 3 niveaux :

| Niveau | Quantum | Priorité |
|--------|---------|----------|
| 0 | 2 ticks | Haute |
| 1 | 4 ticks | Moyenne |
| 2 | 8 ticks | Basse |

Règles :
- Les nouveaux processus entrent au niveau 0
- Quantum épuisé → descendre d'un niveau
- Favorise les processus interactifs (I/O-bound)

### 6.4 Fonction de scheduling

```c
void scheduler_schedule(void) {
    if (current_process == NULL) {
        current_process = select_next_process();
        // ...
    }

    current_process->total_ticks++;
    current_process->remaining_slice--;

    bool should_switch = false;

    switch (current_scheduler) {
        case SCHEDULER_ROUND_ROBIN:
            if (remaining_slice == 0) should_switch = true;
            break;
        case SCHEDULER_PRIORITY:
            // Préemption si processus plus prioritaire
            // ...
    }

    if (should_switch && has_waiting) {
        // Context switch
        // ...
    }
}
```

### 6.5 Journal d'exécution

Le scheduler maintient un journal des exécutions :

```c
typedef struct {
    uint32_t pid;
    char name[16];
    uint64_t start_tick;
    uint64_t end_tick;
    uint32_t duration;
} exec_log_entry_t;
```

---

## 7. Communication inter-processus (IPC)

### 7.1 Mailboxes

Les mailboxes permettent l'échange de messages :

```c
typedef struct {
    uint32_t id;
    char name[16];
    message_t messages[16];   // Buffer circulaire
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
} mailbox_t;
```

### 7.2 Format des messages

```c
typedef struct {
    uint32_t sender_pid;
    uint32_t receiver_pid;
    uint32_t type;
    uint32_t timestamp;
    uint8_t data[64];
    uint32_t data_len;
} message_t;
```

### 7.3 API Mailbox

| Fonction | Description |
|----------|-------------|
| `mailbox_create(name)` | Crée une mailbox |
| `mailbox_send(id, type, data, len)` | Envoie un message |
| `mailbox_receive(id, &msg)` | Reçoit un message |
| `mailbox_destroy(id)` | Supprime une mailbox |

---

## 8. Primitives de synchronisation

### 8.1 Mutex

Les mutex fournissent l'exclusion mutuelle :

```c
typedef struct {
    uint32_t id;
    char name[16];
    bool locked;
    uint32_t owner_pid;
    uint32_t lock_count;    // Support récursif
} mutex_t;
```

**API :**
- `mutex_create(name)` - Crée un mutex
- `mutex_lock(id)` - Verrouille (bloquant)
- `mutex_trylock(id)` - Tente de verrouiller (non-bloquant)
- `mutex_unlock(id)` - Déverrouille

### 8.2 Sémaphores

Les sémaphores contrôlent l'accès aux ressources :

```c
typedef struct {
    uint32_t id;
    char name[16];
    int32_t value;       // Valeur courante
    int32_t max_value;   // Limite (0 = illimitée)
} semaphore_t;
```

**API :**
- `semaphore_create(name, init, max)` - Crée un sémaphore
- `semaphore_wait(id)` - P operation (décrémente)
- `semaphore_signal(id)` - V operation (incrémente)

### 8.3 Protection des sections critiques

```c
static inline void cli(void) {
    __asm__ volatile ("cli");
}

static inline void sti(void) {
    __asm__ volatile ("sti");
}
```

---

## 9. Gestion de la mémoire

### 9.1 Pool Allocator

Implémentation simple basée sur un bitmap :

```c
#define MEMORY_POOL_SIZE    (64 * 1024)  // 64 KB
#define MEMORY_BLOCK_SIZE   64            // 64 bytes

static uint8_t memory_pool[MEMORY_POOL_SIZE];
static uint8_t block_bitmap[MAX_BLOCKS / 8 + 1];
```

### 9.2 Algorithme d'allocation

1. Calculer le nombre de blocs nécessaires
2. Chercher N blocs consécutifs libres dans le bitmap
3. Marquer les blocs comme utilisés
4. Retourner l'adresse calculée

### 9.3 API Mémoire

| Fonction | Description |
|----------|-------------|
| `kmalloc(size)` | Alloue size octets |
| `kfree(ptr)` | Libère la mémoire |
| `kcalloc(n, size)` | Alloue et initialise à zéro |
| `memory_print_stats()` | Affiche les statistiques |
| `memory_print_map()` | Affiche la carte bitmap |

---

## 10. Interface utilisateur (Shell)

### 10.1 Commandes disponibles

| Catégorie | Commandes |
|-----------|-----------|
| Base | help, clear, info, uptime |
| Processus | ps, spawn, kill, block, unblock |
| Ordonnancement | sched, queue, log |
| Tests | bench, demo, states |
| IPC | mbox |
| Synchronisation | mutex, sem |
| Mémoire | mem |

### 10.2 Parsing des commandes

```c
static int parse_command(char* input, char** argv, int max_args) {
    int argc = 0;
    char* p = input;

    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) { *p = '\0'; p++; }
    }
    return argc;
}
```

### 10.3 Driver clavier PS/2

```c
static void keyboard_handler(registers_t* regs) {
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        // Key release
    } else {
        char c = scancode_to_ascii[scancode];
        if (c) buffer_put(c);
    }
}
```

---

## 11. Tests et validation

### 11.1 Tests de build

```bash
make clean && make
# Vérifier : "Compilation réussie !"
```

### 11.2 Tests fonctionnels

| Test | Commande | Résultat attendu |
|------|----------|------------------|
| Boot | `make run` | Shell démarre |
| Processus | `spawn test1` | PID affiché |
| Block/Unblock | `block 2; unblock 2` | État change |
| Ordonnancement | `sched rr; bench 3` | Context switches |
| IPC | `mbox create x; mbox send 1 msg` | Message envoyé |
| Mutex | `mutex create m; mutex lock 1` | Mutex verrouillé |
| Mémoire | `mem test` | Allocations réussies |

### 11.3 Scénarios de test

**Scénario 1 : Round Robin**
```
sched rr
bench 3
log
```
→ Vérifier que les processus alternent toutes les 10 ticks

**Scénario 2 : Priority**
```
sched priority
demo
queue
```
→ highprio (25) exécuté avant medprio (15) et lowprio (5)

**Scénario 3 : IPC**
```
mbox create channel
mbox send 1 "test"
mbox recv 1
```
→ Message "test" reçu

---

## 12. Limites et améliorations futures

### 12.1 Limites actuelles

| Limite | Impact |
|--------|--------|
| Pas de pagination | Limite à 64KB de RAM utilisable |
| Pas de context switch réel | Simulation uniquement |
| Busy-wait pour sync | Consomme CPU inutilement |
| Single-core | Pas de support SMP |

### 12.2 Améliorations possibles

1. **Pagination mémoire** : Support de plus de RAM
2. **Context switch ASM** : Sauvegarde/restauration des registres
3. **Système de fichiers** : FAT ou ext2 simple
4. **Réseau** : Driver NIC et pile TCP/IP
5. **Graphiques** : Mode VESA et GUI simple

---

## 13. Conclusion

myos-i686 est un mini-système d'exploitation éducatif qui démontre
les concepts fondamentaux :

- **Architecture modulaire** claire et extensible
- **6 algorithmes d'ordonnancement** (FCFS, RR, Priority, SJF, SRTF, MLFQ)
- **IPC** via mailboxes pour la communication
- **Primitives de synchronisation** (mutex, sémaphores)
- **Gestion mémoire** avec pool allocator
- **Shell interactif** avec 20+ commandes

Le projet respecte les contraintes de développement bare-metal
(code freestanding, pas de libc) tout en restant simple et
pédagogique pour un contexte universitaire.

---

## Annexe : Compilation et exécution

### Prérequis
```bash
apt install nasm qemu-system-x86 gcc-i686-linux-gnu
```

### Compilation
```bash
make clean && make
```

### Exécution
```bash
qemu-system-i386 -kernel kernel/build/myos.bin -m 32M
```

### Commandes utiles
```
help        # Affiche l'aide
demo        # Démonstration complète
bench 5     # Benchmark avec 5 processus
```

---

*Fin du rapport technique*
