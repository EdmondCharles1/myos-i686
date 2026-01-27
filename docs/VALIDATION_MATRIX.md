# Matrice de Validation - myos-i686

Ce document recapitule l'etat de toutes les fonctionnalites du projet selon le cahier des charges.

---

## Legende

| Statut | Description |
|--------|-------------|
| OK | Fonctionnalite implementee et testable |
| PARTIEL | Implementation partielle ou non testee |
| MANQUANT | Non implemente |

---

## A) Exigences Obligatoires

| # | Exigence | Statut | Fichiers | Fonctions Cles | Commande Test |
|---|----------|--------|----------|----------------|---------------|
| 1 | Demarrage Mini-OS (Multiboot) | OK | boot.asm, kernel.c | kernel_main() | `make run` |
| 2 | Execution QEMU stable | OK | Makefile | N/A | `make run` |
| 3 | PCB complet | OK | process.h/c | process_t, process_create() | `ps` |
| 4 | Etats READY/RUNNING/BLOCKED/TERMINATED | OK | process.h | process_state_t | `ps`, `states` |
| 5 | Transitions correctes (block/unblock/exit) | OK | scheduler.c, process.c | scheduler_block_process(), scheduler_unblock_process() | `block`, `unblock`, `kill` |
| 6 | FCFS | OK | scheduler.c | select_next_fcfs() | `sched fcfs` |
| 7 | Round Robin (quantum + preemption) | OK | scheduler.c | select_next_round_robin(), scheduler_schedule() | `sched rr`, `log` |
| 8 | PIT configure | OK | timer.c | timer_init(100) | `uptime` |
| 9 | Journal d'execution (exec_log) | OK | scheduler.c | log_execution(), exec_log[] | `log` |
| 10 | Mini-shell fonctionnel | OK | shell.c | shell_run(), shell_execute() | `help` |
| 11 | Commandes minimales | OK | shell.c | cmd_* | `help`, `ps`, `sched`, `log`, `uptime` |

---

## B) Bonus

| # | Exigence | Statut | Fichiers | Fonctions Cles | Commande Test |
|---|----------|--------|----------|----------------|---------------|
| 6 | Ordonnancement Priority (preemptif) | OK | scheduler.c | select_next_priority(), find_highest_priority() | `sched priority`, `demo` |
| 7a | SJF (Shortest Job First) | OK | scheduler.c | select_next_sjf(), find_shortest_job() | `sched sjf`, `bench` |
| 7b | SRTF (Shortest Remaining Time First) | OK | scheduler.c | select_next_srtf(), find_shortest_remaining() | `sched srtf`, `bench` |
| 8 | MLFQ (3 niveaux) | OK | scheduler.c, scheduler.h | mlfq_queues[], get_mlfq_quantum(), scheduler_mlfq_boost() | `sched mlfq`, `queue` |
| 9a | IPC: mbox_create | OK | ipc.c | mbox_create() | `mbox create <nom>` |
| 9b | IPC: mbox_send | OK | ipc.c | mbox_send() | `mbox send <id> <msg>` |
| 9c | IPC: mbox_recv | OK | ipc.c | mbox_recv() | `mbox recv <id>` |
| 9d | IPC: gestion full/empty | OK | ipc.c | IPC_ERROR_FULL, IPC_ERROR_EMPTY, mbox_send_blocking(), mbox_recv_blocking() | `mbox test` |
| 10a | mutex_lock/unlock | OK | sync.c | mutex_lock(), mutex_unlock(), mutex_trylock() | `mutex lock/unlock <id>` |
| 10b | semaphore wait/post | OK | sync.c | sem_wait(), sem_post(), sem_trywait() | `sem wait/post <id>` |
| 10c | Blocage/reveil via BLOCKED | OK | sync.c, scheduler.c | scheduler_block_process(), scheduler_unblock_process() | `mutex test`, `sem test` |
| 11a | Pool allocator statique (64KB) | OK | memory.c | memory_pool[], memory_bitmap[] | `mem stats` |
| 11b | kmalloc/kfree | OK | memory.c | kmalloc(), kfree() | `mem test` |
| 11c | kcalloc | OK | memory.c | kcalloc() | `mem test` |
| 11d | Commande shell mem | OK | shell.c | cmd_mem() | `mem test`, `mem stats`, `mem bitmap` |

---

## C) Structure Modulaire

| Module | Fichiers | Statut | Description |
|--------|----------|--------|-------------|
| Process | process.c, process.h | OK | PCB, creation, etats, table des processus |
| Scheduler | scheduler.c, scheduler.h | OK | FCFS, RR, Priority, SJF, SRTF, MLFQ |
| Timer | timer.c, timer.h | OK | PIT 100Hz, ticks, ms |
| IPC | ipc.c, ipc.h | OK | Mailboxes avec buffer circulaire |
| Sync | sync.c, sync.h | OK | Mutex et Semaphores |
| Memory | memory.c, memory.h | OK | Pool allocator 64KB avec bitmap |
| Shell | shell.c, shell.h | OK | Toutes les commandes |
| Makefile | Makefile | OK | Detection automatique des modules |

---

## D) Commandes Shell Disponibles

### Commandes de Base
| Commande | Statut | Description |
|----------|--------|-------------|
| help | OK | Affiche l'aide |
| clear | OK | Efface l'ecran |
| info | OK | Informations systeme |
| uptime | OK | Temps d'execution |
| reboot | OK | Simulation redemarrage |

### Gestion des Processus
| Commande | Statut | Description |
|----------|--------|-------------|
| ps | OK | Liste les processus |
| kill <pid> | OK | Termine un processus |
| spawn [n] [burst] | OK | Cree n processus |
| bench | OK | Benchmark avec 3 processus |
| demo | OK | Demo ordonnancement |
| states | OK | Demo transitions d'etats |
| block <pid> | OK | Bloque un processus |
| unblock <pid> | OK | Debloque un processus |

### Ordonnancement
| Commande | Statut | Description |
|----------|--------|-------------|
| sched | OK | Affiche ordonnanceur actuel |
| sched fcfs | OK | Passe en FCFS |
| sched rr | OK | Passe en Round Robin |
| sched priority | OK | Passe en Priority |
| sched sjf | OK | Passe en SJF |
| sched srtf | OK | Passe en SRTF |
| sched mlfq | OK | Passe en MLFQ |
| log | OK | Journal d'execution |
| queue | OK | File READY |

### Memoire
| Commande | Statut | Description |
|----------|--------|-------------|
| mem | OK | Affiche stats |
| mem stats | OK | Statistiques memoire |
| mem test | OK | Test allocation |
| mem bitmap | OK | Affiche bitmap |

### IPC (Mailboxes)
| Commande | Statut | Description |
|----------|--------|-------------|
| mbox list | OK | Liste mailboxes |
| mbox create <nom> | OK | Cree mailbox |
| mbox send <id> <msg> | OK | Envoie message |
| mbox recv <id> | OK | Recoit message |
| mbox test | OK | Test complet IPC |

### Synchronisation
| Commande | Statut | Description |
|----------|--------|-------------|
| mutex list | OK | Liste mutex |
| mutex create <nom> | OK | Cree mutex |
| mutex lock <id> | OK | Verrouille |
| mutex unlock <id> | OK | Deverrouille |
| mutex test | OK | Test mutex |
| sem list | OK | Liste semaphores |
| sem create <nom> <val> | OK | Cree semaphore |
| sem wait <id> | OK | Decremente (P) |
| sem post <id> | OK | Incremente (V) |
| sem test | OK | Test semaphore |

---

## E) Details Techniques

### Ordonnanceurs Implementes

| Type | Preemptif | Description | Algorithme |
|------|-----------|-------------|------------|
| FCFS | Non | First Come First Served | FIFO simple |
| Round Robin | Oui | Quantum de temps | Rotation avec quantum=10 ticks |
| Priority | Oui | Par priorite | Selection highest priority, preemption |
| SJF | Non | Shortest Job First | Selection burst_time minimum |
| SRTF | Oui | Shortest Remaining Time | Selection remaining_time minimum, preemption |
| MLFQ | Oui | Multi-Level Feedback | 3 niveaux (quantum 5/10/20), demotion, boost periodique |

### Configuration MLFQ
- Niveau 0: quantum = 5 ticks (haute priorite)
- Niveau 1: quantum = 10 ticks (moyenne priorite)
- Niveau 2: quantum = 20 ticks (basse priorite)
- Allotment: 30 ticks avant demotion
- Boost: tous les 500 ticks

### Configuration Memoire
- Pool: 64 KB (65536 octets)
- Taille bloc: 64 octets
- Nombre blocs: 1024
- Bitmap: 128 octets

### Configuration IPC
- Max mailboxes: 16
- Capacite mailbox: 8 messages
- Taille message max: 64 octets

### Configuration Sync
- Max mutex: 16
- Max semaphores: 16
- Max waiters: 8 par ressource

---

## F) Verification Multiboot

```bash
make check
# Doit afficher: "Header Multiboot valide"
```

---

## G) Resume Final

| Categorie | Total | OK | PARTIEL | MANQUANT |
|-----------|-------|-----|---------|----------|
| Obligatoire | 11 | 11 | 0 | 0 |
| Bonus | 14 | 14 | 0 | 0 |
| Modules | 7 | 7 | 0 | 0 |
| Commandes | 30+ | 30+ | 0 | 0 |

**Couverture: 100% du cahier des charges**

---

## H) Comment Tester

1. Compiler: `make`
2. Verifier Multiboot: `make check`
3. Lancer: `make run`
4. Dans le shell, taper `help` pour voir les commandes
5. Suivre les scenarios dans TEST_SCENARIOS.md

---

*Document genere pour myos-i686 v0.8*
