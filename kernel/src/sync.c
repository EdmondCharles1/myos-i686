/*
 * sync.c - Implementation des primitives de synchronisation
 */

#include "sync.h"
#include "printf.h"
#include "scheduler.h"

// =============================================================================
// Variables globales
// =============================================================================

// Tables des mutex et semaphores
static mutex_t mutexes[MAX_MUTEXES];
static semaphore_t semaphores[MAX_SEMAPHORES];

// Compteurs d'ID
static uint32_t next_mutex_id = 1;
static uint32_t next_sem_id = 1;

// =============================================================================
// Fonctions utilitaires
// =============================================================================

// Compare deux chaines
static int str_cmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Copie une chaine
static void str_cpy(char* dest, const char* src, size_t max) {
    size_t i;
    for (i = 0; i < max - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// =============================================================================
// Mutex - Fonctions internes
// =============================================================================

static mutex_t* find_mutex_by_id(uint32_t id) {
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].active && mutexes[i].id == id) {
            return &mutexes[i];
        }
    }
    return NULL;
}

static mutex_t* find_free_mutex(void) {
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (!mutexes[i].active) {
            return &mutexes[i];
        }
    }
    return NULL;
}

static void mutex_add_waiter(mutex_t* m, process_t* p) {
    if (m->waiter_count < SYNC_MAX_WAITERS) {
        m->waiters[m->waiter_count++] = p;
    }
}

static process_t* mutex_remove_waiter(mutex_t* m) {
    if (m->waiter_count == 0) {
        return NULL;
    }

    // FIFO: retirer le premier
    process_t* p = m->waiters[0];

    for (uint32_t i = 0; i < m->waiter_count - 1; i++) {
        m->waiters[i] = m->waiters[i + 1];
    }
    m->waiter_count--;

    return p;
}

// =============================================================================
// Semaphore - Fonctions internes
// =============================================================================

static semaphore_t* find_sem_by_id(uint32_t id) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].active && semaphores[i].id == id) {
            return &semaphores[i];
        }
    }
    return NULL;
}

static semaphore_t* find_free_sem(void) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphores[i].active) {
            return &semaphores[i];
        }
    }
    return NULL;
}

static void sem_add_waiter(semaphore_t* s, process_t* p) {
    if (s->waiter_count < SYNC_MAX_WAITERS) {
        s->waiters[s->waiter_count++] = p;
    }
}

static process_t* sem_remove_waiter(semaphore_t* s) {
    if (s->waiter_count == 0) {
        return NULL;
    }

    // FIFO: retirer le premier
    process_t* p = s->waiters[0];

    for (uint32_t i = 0; i < s->waiter_count - 1; i++) {
        s->waiters[i] = s->waiters[i + 1];
    }
    s->waiter_count--;

    return p;
}

// =============================================================================
// Implementation API - Initialisation
// =============================================================================

void sync_init(void) {
    printf("[SYNC] Initialisation du systeme de synchronisation...\n");

    // Initialiser les mutex
    for (int i = 0; i < MAX_MUTEXES; i++) {
        mutexes[i].id = 0;
        mutexes[i].name[0] = '\0';
        mutexes[i].active = false;
        mutexes[i].locked = false;
        mutexes[i].owner_pid = 0;
        mutexes[i].waiter_count = 0;
        mutexes[i].lock_count = 0;
        mutexes[i].contention_count = 0;
    }

    // Initialiser les semaphores
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        semaphores[i].id = 0;
        semaphores[i].name[0] = '\0';
        semaphores[i].active = false;
        semaphores[i].value = 0;
        semaphores[i].initial_value = 0;
        semaphores[i].waiter_count = 0;
        semaphores[i].wait_count = 0;
        semaphores[i].post_count = 0;
    }

    next_mutex_id = 1;
    next_sem_id = 1;

    printf("[SYNC] Max mutex: %d, Max semaphores: %d\n",
           MAX_MUTEXES, MAX_SEMAPHORES);
    printf("[SYNC] Systeme de synchronisation initialise\n");
}

// =============================================================================
// Implementation API - Mutex
// =============================================================================

int32_t mutex_create(const char* name) {
    if (name == NULL || name[0] == '\0') {
        return SYNC_ERROR_PARAM;
    }

    // Verifier si existe deja
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].active && str_cmp(mutexes[i].name, name) == 0) {
            return SYNC_ERROR_EXISTS;
        }
    }

    mutex_t* m = find_free_mutex();
    if (m == NULL) {
        return SYNC_ERROR_NOSLOT;
    }

    m->id = next_mutex_id++;
    str_cpy(m->name, name, SYNC_NAME_LEN);
    m->active = true;
    m->locked = false;
    m->owner_pid = 0;
    m->waiter_count = 0;
    m->lock_count = 0;
    m->contention_count = 0;

    printf("[SYNC] Mutex '%s' cree (ID=%u)\n", name, m->id);

    return (int32_t)m->id;
}

int32_t mutex_destroy(uint32_t id) {
    mutex_t* m = find_mutex_by_id(id);
    if (m == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    // Debloquer tous les processus en attente
    while (m->waiter_count > 0) {
        process_t* p = mutex_remove_waiter(m);
        if (p != NULL) {
            scheduler_unblock_process(p);
        }
    }

    printf("[SYNC] Mutex '%s' (ID=%u) detruit\n", m->name, id);

    m->active = false;
    m->id = 0;
    m->name[0] = '\0';

    return SYNC_SUCCESS;
}

int32_t mutex_find(const char* name) {
    if (name == NULL) {
        return SYNC_ERROR_PARAM;
    }

    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].active && str_cmp(mutexes[i].name, name) == 0) {
            return (int32_t)mutexes[i].id;
        }
    }

    return SYNC_ERROR_NOTFOUND;
}

int32_t mutex_lock(uint32_t id) {
    mutex_t* m = find_mutex_by_id(id);
    if (m == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    process_t* current = scheduler_get_current();
    uint32_t current_pid = current ? current->pid : 0;

    if (!m->locked) {
        // Mutex libre, le prendre
        m->locked = true;
        m->owner_pid = current_pid;
        m->lock_count++;
        return SYNC_SUCCESS;
    }

    // Mutex deja pris
    m->contention_count++;

    if (current == NULL) {
        // Pas de processus courant (ne devrait pas arriver)
        return SYNC_ERROR_BUSY;
    }

    // Bloquer le processus
    current->block_reason = BLOCK_REASON_MUTEX;
    current->block_resource = m;
    mutex_add_waiter(m, current);
    scheduler_block_process(current);

    // Apres deblocage, on a le mutex
    m->lock_count++;
    return SYNC_SUCCESS;
}

int32_t mutex_trylock(uint32_t id) {
    mutex_t* m = find_mutex_by_id(id);
    if (m == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    if (m->locked) {
        return SYNC_ERROR_BUSY;
    }

    process_t* current = scheduler_get_current();
    m->locked = true;
    m->owner_pid = current ? current->pid : 0;
    m->lock_count++;

    return SYNC_SUCCESS;
}

int32_t mutex_unlock(uint32_t id) {
    mutex_t* m = find_mutex_by_id(id);
    if (m == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    process_t* current = scheduler_get_current();
    uint32_t current_pid = current ? current->pid : 0;

    // Verifier que c'est le proprietaire (ou qu'il n'y a pas de processus courant)
    if (m->owner_pid != 0 && m->owner_pid != current_pid && current != NULL) {
        printf("[SYNC] ATTENTION: unlock par PID=%u, owner=%u\n",
               current_pid, m->owner_pid);
        // On permet quand meme le unlock pour la demo
    }

    // Y a-t-il des processus en attente?
    process_t* waiter = mutex_remove_waiter(m);

    if (waiter != NULL) {
        // Transferer le mutex au premier waiter
        m->owner_pid = waiter->pid;
        scheduler_unblock_process(waiter);
    } else {
        // Liberer le mutex
        m->locked = false;
        m->owner_pid = 0;
    }

    return SYNC_SUCCESS;
}

void mutex_print_all(void) {
    printf("\n=== Mutex ===\n");
    printf("ID   | Nom              | Etat     | Owner | Wait | Locks | Cont\n");
    printf("-----|------------------|----------|-------|------|-------|------\n");

    bool found = false;
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].active) {
            found = true;
            mutex_t* m = &mutexes[i];
            printf("%-4u | %-16s | %-8s | %-5u | %4u | %5u | %4u\n",
                   m->id,
                   m->name,
                   m->locked ? "LOCKED" : "FREE",
                   m->owner_pid,
                   m->waiter_count,
                   m->lock_count,
                   m->contention_count);
        }
    }

    if (!found) {
        printf("(aucun mutex)\n");
    }
    printf("\n");
}

// =============================================================================
// Implementation API - Semaphore
// =============================================================================

int32_t sem_create(const char* name, int32_t value) {
    if (name == NULL || name[0] == '\0') {
        return SYNC_ERROR_PARAM;
    }

    // Verifier si existe deja
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].active && str_cmp(semaphores[i].name, name) == 0) {
            return SYNC_ERROR_EXISTS;
        }
    }

    semaphore_t* s = find_free_sem();
    if (s == NULL) {
        return SYNC_ERROR_NOSLOT;
    }

    s->id = next_sem_id++;
    str_cpy(s->name, name, SYNC_NAME_LEN);
    s->active = true;
    s->value = value;
    s->initial_value = value;
    s->waiter_count = 0;
    s->wait_count = 0;
    s->post_count = 0;

    printf("[SYNC] Semaphore '%s' cree (ID=%u, value=%d)\n", name, s->id, value);

    return (int32_t)s->id;
}

int32_t sem_destroy(uint32_t id) {
    semaphore_t* s = find_sem_by_id(id);
    if (s == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    // Debloquer tous les processus en attente
    while (s->waiter_count > 0) {
        process_t* p = sem_remove_waiter(s);
        if (p != NULL) {
            scheduler_unblock_process(p);
        }
    }

    printf("[SYNC] Semaphore '%s' (ID=%u) detruit\n", s->name, id);

    s->active = false;
    s->id = 0;
    s->name[0] = '\0';

    return SYNC_SUCCESS;
}

int32_t sem_find(const char* name) {
    if (name == NULL) {
        return SYNC_ERROR_PARAM;
    }

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].active && str_cmp(semaphores[i].name, name) == 0) {
            return (int32_t)semaphores[i].id;
        }
    }

    return SYNC_ERROR_NOTFOUND;
}

int32_t sem_wait(uint32_t id) {
    semaphore_t* s = find_sem_by_id(id);
    if (s == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    s->wait_count++;

    if (s->value > 0) {
        // Ressource disponible
        s->value--;
        return SYNC_SUCCESS;
    }

    // Bloquer le processus
    process_t* current = scheduler_get_current();
    if (current == NULL) {
        return SYNC_ERROR_WOULDBLOCK;
    }

    current->block_reason = BLOCK_REASON_SEM;
    current->block_resource = s;
    sem_add_waiter(s, current);
    scheduler_block_process(current);

    // Apres deblocage, la ressource est disponible
    s->value--;
    return SYNC_SUCCESS;
}

int32_t sem_trywait(uint32_t id) {
    semaphore_t* s = find_sem_by_id(id);
    if (s == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    if (s->value <= 0) {
        return SYNC_ERROR_WOULDBLOCK;
    }

    s->value--;
    s->wait_count++;
    return SYNC_SUCCESS;
}

int32_t sem_post(uint32_t id) {
    semaphore_t* s = find_sem_by_id(id);
    if (s == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }

    s->post_count++;

    // Y a-t-il des processus en attente?
    process_t* waiter = sem_remove_waiter(s);

    if (waiter != NULL) {
        // Reveiller le premier waiter (qui decrementera la valeur)
        s->value++;  // Le waiter va la decrementer
        scheduler_unblock_process(waiter);
    } else {
        // Incrementer la valeur
        s->value++;
    }

    return SYNC_SUCCESS;
}

int32_t sem_getvalue(uint32_t id) {
    semaphore_t* s = find_sem_by_id(id);
    if (s == NULL) {
        return SYNC_ERROR_NOTFOUND;
    }
    return s->value;
}

void sem_print_all(void) {
    printf("\n=== Semaphores ===\n");
    printf("ID   | Nom              | Value | Init | Wait | Waits | Posts\n");
    printf("-----|------------------|-------|------|------|-------|------\n");

    bool found = false;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].active) {
            found = true;
            semaphore_t* s = &semaphores[i];
            printf("%-4u | %-16s | %5d | %4d | %4u | %5u | %5u\n",
                   s->id,
                   s->name,
                   s->value,
                   s->initial_value,
                   s->waiter_count,
                   s->wait_count,
                   s->post_count);
        }
    }

    if (!found) {
        printf("(aucun semaphore)\n");
    }
    printf("\n");
}

// =============================================================================
// Tests
// =============================================================================

void mutex_test(void) {
    printf("\n=== Test Mutex ===\n\n");

    // Test 1: Creation
    printf("Test 1: Creation de mutex\n");
    int32_t m1 = mutex_create("test_mutex");
    printf("  Mutex cree: ID=%d\n", m1);

    // Test 2: Lock/Unlock
    printf("\nTest 2: Lock/Unlock\n");
    int32_t r = mutex_lock(m1);
    printf("  Lock: %s\n", r == SYNC_SUCCESS ? "OK" : "ERREUR");

    r = mutex_unlock(m1);
    printf("  Unlock: %s\n", r == SYNC_SUCCESS ? "OK" : "ERREUR");

    // Test 3: Trylock
    printf("\nTest 3: Trylock\n");
    r = mutex_trylock(m1);
    printf("  Trylock (libre): %s\n", r == SYNC_SUCCESS ? "OK" : "ERREUR");

    r = mutex_trylock(m1);
    printf("  Trylock (pris): %s\n", r == SYNC_ERROR_BUSY ? "BUSY (attendu)" : "ERREUR");

    mutex_unlock(m1);

    // Test 4: Find
    printf("\nTest 4: Recherche par nom\n");
    int32_t found = mutex_find("test_mutex");
    printf("  'test_mutex' -> ID=%d\n", found);

    // Afficher l'etat
    mutex_print_all();

    // Nettoyage
    mutex_destroy(m1);

    printf("=== Test Mutex termine ===\n\n");
}

void sem_test(void) {
    printf("\n=== Test Semaphore ===\n\n");

    // Test 1: Creation
    printf("Test 1: Creation de semaphore\n");
    int32_t s1 = sem_create("test_sem", 2);
    printf("  Semaphore cree: ID=%d, value=2\n", s1);

    // Test 2: Wait (decremente)
    printf("\nTest 2: Wait (decremente)\n");
    int32_t r = sem_wait(s1);
    printf("  Wait 1: %s, value=%d\n",
           r == SYNC_SUCCESS ? "OK" : "ERREUR", sem_getvalue(s1));

    r = sem_wait(s1);
    printf("  Wait 2: %s, value=%d\n",
           r == SYNC_SUCCESS ? "OK" : "ERREUR", sem_getvalue(s1));

    // Test 3: Trywait sur sem a 0
    printf("\nTest 3: Trywait sur semaphore a 0\n");
    r = sem_trywait(s1);
    printf("  Trywait: %s\n",
           r == SYNC_ERROR_WOULDBLOCK ? "WOULDBLOCK (attendu)" : "ERREUR");

    // Test 4: Post (incremente)
    printf("\nTest 4: Post (incremente)\n");
    r = sem_post(s1);
    printf("  Post: %s, value=%d\n",
           r == SYNC_SUCCESS ? "OK" : "ERREUR", sem_getvalue(s1));

    // Afficher l'etat
    sem_print_all();

    // Nettoyage
    sem_destroy(s1);

    printf("=== Test Semaphore termine ===\n\n");
}

void sync_test(void) {
    printf("\n========================================\n");
    printf("    TEST COMPLET SYNCHRONISATION\n");
    printf("========================================\n");

    mutex_test();
    sem_test();

    printf("========================================\n");
    printf("    FIN TESTS SYNCHRONISATION\n");
    printf("========================================\n\n");
}

// =============================================================================
// Demo avec processus simules
// =============================================================================

// Variables globales pour la demo
static uint32_t demo_mutex_id = 0;
static volatile int demo_step = 0;

// Fonction d'entree du processus 1 (simule)
static void demo_process1_entry(void) {
    printf("  [P1] Demarrage du processus 1\n");
    printf("  [P1] Tentative d'acquisition du mutex...\n");

    int32_t r = mutex_trylock(demo_mutex_id);
    if (r == SYNC_SUCCESS) {
        printf("  [P1] Mutex acquis avec succes!\n");
        demo_step = 1;  // Signal que P1 a le mutex
    } else {
        printf("  [P1] Echec acquisition: %d\n", r);
    }
}

// Fonction d'entree du processus 2 (simule)
static void demo_process2_entry(void) {
    printf("  [P2] Demarrage du processus 2\n");
    printf("  [P2] Tentative d'acquisition du mutex...\n");

    int32_t r = mutex_trylock(demo_mutex_id);
    if (r == SYNC_SUCCESS) {
        printf("  [P2] Mutex acquis avec succes!\n");
    } else if (r == SYNC_ERROR_BUSY) {
        printf("  [P2] Mutex deja pris - processus BLOQUE!\n");
        printf("  [P2] (En attente que P1 libere le mutex...)\n");
        demo_step = 2;  // Signal que P2 est bloque
    }
}

void mutex_demo(void) {
    printf("\n========================================\n");
    printf("  DEMONSTRATION MUTEX AVEC PROCESSUS\n");
    printf("========================================\n\n");

    printf("Cette demo montre comment les mutex synchronisent les processus.\n\n");

    // Etape 1: Creation du mutex
    printf("=== Etape 1: Creation du mutex ===\n");
    demo_mutex_id = mutex_create("demo_mutex");
    if ((int32_t)demo_mutex_id < 0) {
        printf("Erreur creation mutex: %d\n", (int32_t)demo_mutex_id);
        return;
    }
    printf("Mutex 'demo_mutex' cree (ID=%u)\n\n", demo_mutex_id);
    demo_step = 0;

    // Etape 2: Creation du processus 1
    printf("=== Etape 2: Processus P1 acquiert le mutex ===\n");
    uint32_t pid1 = process_create("P1_demo", demo_process1_entry, 10);
    if (pid1 == 0) {
        printf("Erreur creation processus P1\n");
        mutex_destroy(demo_mutex_id);
        return;
    }

    // Simuler l'execution de P1
    demo_process1_entry();
    process_t* p1 = process_get_by_pid(pid1);
    if (p1) {
        p1->state = PROCESS_STATE_RUNNING;
    }
    printf("\n");

    // Afficher l'etat du mutex
    printf("Etat du mutex:\n");
    mutex_print_all();

    // Etape 3: Creation du processus 2 qui va tenter d'acquerir
    printf("=== Etape 3: Processus P2 tente d'acquerir le mutex ===\n");
    uint32_t pid2 = process_create("P2_demo", demo_process2_entry, 10);
    if (pid2 == 0) {
        printf("Erreur creation processus P2\n");
        process_kill(pid1);
        mutex_destroy(demo_mutex_id);
        return;
    }

    // Simuler l'execution de P2 qui se bloque
    demo_process2_entry();
    process_t* p2 = process_get_by_pid(pid2);
    if (p2 && demo_step == 2) {
        p2->state = PROCESS_STATE_BLOCKED;
        p2->block_reason = BLOCK_REASON_MUTEX;
    }
    printf("\n");

    // Afficher l'etat des processus
    printf("Etat des processus:\n");
    process_list();

    // Etape 4: P1 libere le mutex
    printf("=== Etape 4: P1 libere le mutex ===\n");
    printf("  [P1] Liberation du mutex...\n");
    mutex_unlock(demo_mutex_id);
    printf("  [P1] Mutex libere!\n\n");

    // Etape 5: P2 se reveille et acquiert le mutex
    printf("=== Etape 5: P2 se reveille ===\n");
    if (p2) {
        p2->state = PROCESS_STATE_READY;
        p2->block_reason = 0;
        printf("  [P2] Reveille! Tentative d'acquisition...\n");

        int32_t r = mutex_trylock(demo_mutex_id);
        if (r == SYNC_SUCCESS) {
            printf("  [P2] Mutex acquis avec succes!\n");
            p2->state = PROCESS_STATE_RUNNING;
        }
    }
    printf("\n");

    // Afficher l'etat final
    printf("=== Etat final ===\n");
    mutex_print_all();
    process_list();

    // Nettoyage
    printf("=== Nettoyage ===\n");
    mutex_unlock(demo_mutex_id);
    mutex_destroy(demo_mutex_id);
    process_kill(pid1);
    process_kill(pid2);
    printf("Demo terminee, ressources liberees.\n");

    printf("\n========================================\n");
    printf("  FIN DEMONSTRATION MUTEX\n");
    printf("========================================\n\n");

    printf("Resume:\n");
    printf("- Les mutex permettent l'exclusion mutuelle\n");
    printf("- Un processus qui tente d'acquerir un mutex deja pris est BLOQUE\n");
    printf("- Quand le mutex est libere, le processus en attente est REVEILLE\n");
    printf("- Cela evite les race conditions sur les ressources partagees\n\n");
}
