/*
 * ipc.c - Implementation des mailboxes IPC
 */

#include "ipc.h"
#include "printf.h"
#include "scheduler.h"

// =============================================================================
// Variables globales
// =============================================================================

// Table des mailboxes
static mailbox_t mailboxes[MAX_MAILBOXES];

// Compteur d'ID
static uint32_t next_mbox_id = 1;

// =============================================================================
// Fonctions utilitaires
// =============================================================================

// Compare deux chaines
static int str_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Copie une chaine
static void str_copy(char* dest, const char* src, size_t max) {
    size_t i;
    for (i = 0; i < max - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Copie des donnees
static void mem_copy(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

// Trouve une mailbox par ID
static mailbox_t* find_mailbox_by_id(uint32_t id) {
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].active && mailboxes[i].id == id) {
            return &mailboxes[i];
        }
    }
    return NULL;
}

// Trouve un slot libre
static mailbox_t* find_free_slot(void) {
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (!mailboxes[i].active) {
            return &mailboxes[i];
        }
    }
    return NULL;
}

// =============================================================================
// Implementation de l'API
// =============================================================================

void ipc_init(void) {
    printf("[IPC] Initialisation du systeme IPC...\n");

    // Initialiser toutes les mailboxes
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        mailboxes[i].id = 0;
        mailboxes[i].name[0] = '\0';
        mailboxes[i].active = false;
        mailboxes[i].head = 0;
        mailboxes[i].tail = 0;
        mailboxes[i].count = 0;
        mailboxes[i].waiting_send = NULL;
        mailboxes[i].waiting_recv = NULL;
        mailboxes[i].total_sent = 0;
        mailboxes[i].total_received = 0;
    }

    next_mbox_id = 1;

    printf("[IPC] Max mailboxes: %d, Capacite: %d messages\n",
           MAX_MAILBOXES, MAILBOX_CAPACITY);
    printf("[IPC] Systeme IPC initialise\n");
}

int32_t mbox_create(const char* name) {
    if (name == NULL || name[0] == '\0') {
        return IPC_ERROR_PARAM;
    }

    // Verifier si une mailbox avec ce nom existe deja
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].active && str_compare(mailboxes[i].name, name) == 0) {
            return IPC_ERROR_EXISTS;
        }
    }

    // Trouver un slot libre
    mailbox_t* mbox = find_free_slot();
    if (mbox == NULL) {
        return IPC_ERROR_NOSLOT;
    }

    // Initialiser la mailbox
    mbox->id = next_mbox_id++;
    str_copy(mbox->name, name, MAILBOX_NAME_LEN);
    mbox->active = true;
    mbox->head = 0;
    mbox->tail = 0;
    mbox->count = 0;
    mbox->waiting_send = NULL;
    mbox->waiting_recv = NULL;
    mbox->total_sent = 0;
    mbox->total_received = 0;

    printf("[IPC] Mailbox '%s' creee (ID=%u)\n", name, mbox->id);

    return (int32_t)mbox->id;
}

int32_t mbox_destroy(uint32_t id) {
    mailbox_t* mbox = find_mailbox_by_id(id);
    if (mbox == NULL) {
        return IPC_ERROR_NOTFOUND;
    }

    // Debloquer les processus en attente (avec erreur)
    if (mbox->waiting_send != NULL) {
        scheduler_unblock_process(mbox->waiting_send);
    }
    if (mbox->waiting_recv != NULL) {
        scheduler_unblock_process(mbox->waiting_recv);
    }

    printf("[IPC] Mailbox '%s' (ID=%u) detruite\n", mbox->name, id);

    mbox->active = false;
    mbox->id = 0;
    mbox->name[0] = '\0';

    return IPC_SUCCESS;
}

int32_t mbox_find(const char* name) {
    if (name == NULL) {
        return IPC_ERROR_PARAM;
    }

    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].active && str_compare(mailboxes[i].name, name) == 0) {
            return (int32_t)mailboxes[i].id;
        }
    }

    return IPC_ERROR_NOTFOUND;
}

int32_t mbox_send(uint32_t mbox_id, const void* data, uint32_t size) {
    mailbox_t* mbox = find_mailbox_by_id(mbox_id);
    if (mbox == NULL) {
        return IPC_ERROR_NOTFOUND;
    }

    if (data == NULL || size == 0) {
        return IPC_ERROR_PARAM;
    }

    if (size > MESSAGE_MAX_SIZE) {
        size = MESSAGE_MAX_SIZE;  // Tronquer
    }

    // Verifier si la mailbox est pleine
    if (mbox->count >= MAILBOX_CAPACITY) {
        return IPC_ERROR_FULL;
    }

    // Ajouter le message
    ipc_message_t* msg = &mbox->messages[mbox->tail];
    msg->sender_pid = scheduler_get_current() ? scheduler_get_current()->pid : 0;
    msg->size = size;
    mem_copy(msg->data, data, size);

    // Avancer le pointeur d'ecriture
    mbox->tail = (mbox->tail + 1) % MAILBOX_CAPACITY;
    mbox->count++;
    mbox->total_sent++;

    // Debloquer un processus en attente de reception
    if (mbox->waiting_recv != NULL) {
        process_t* p = mbox->waiting_recv;
        mbox->waiting_recv = NULL;
        scheduler_unblock_process(p);
    }

    return IPC_SUCCESS;
}

int32_t mbox_recv(uint32_t mbox_id, void* data, uint32_t max_size,
                  uint32_t* out_size, uint32_t* out_sender) {
    mailbox_t* mbox = find_mailbox_by_id(mbox_id);
    if (mbox == NULL) {
        return IPC_ERROR_NOTFOUND;
    }

    if (data == NULL || max_size == 0) {
        return IPC_ERROR_PARAM;
    }

    // Verifier si la mailbox est vide
    if (mbox->count == 0) {
        return IPC_ERROR_EMPTY;
    }

    // Lire le message
    ipc_message_t* msg = &mbox->messages[mbox->head];

    uint32_t copy_size = msg->size;
    if (copy_size > max_size) {
        copy_size = max_size;
    }

    mem_copy(data, msg->data, copy_size);

    if (out_size != NULL) {
        *out_size = copy_size;
    }
    if (out_sender != NULL) {
        *out_sender = msg->sender_pid;
    }

    // Avancer le pointeur de lecture
    mbox->head = (mbox->head + 1) % MAILBOX_CAPACITY;
    mbox->count--;
    mbox->total_received++;

    // Debloquer un processus en attente d'envoi
    if (mbox->waiting_send != NULL) {
        process_t* p = mbox->waiting_send;
        mbox->waiting_send = NULL;
        scheduler_unblock_process(p);
    }

    return IPC_SUCCESS;
}

int32_t mbox_send_blocking(uint32_t mbox_id, const void* data, uint32_t size) {
    int32_t result = mbox_send(mbox_id, data, size);

    if (result == IPC_ERROR_FULL) {
        // Bloquer le processus courant
        mailbox_t* mbox = find_mailbox_by_id(mbox_id);
        if (mbox != NULL) {
            process_t* current = scheduler_get_current();
            if (current != NULL) {
                current->block_reason = BLOCK_REASON_MBOX_FULL;
                current->block_resource = mbox;
                mbox->waiting_send = current;
                scheduler_block_process(current);
                // Apres deblocage, reessayer
                return mbox_send(mbox_id, data, size);
            }
        }
    }

    return result;
}

int32_t mbox_recv_blocking(uint32_t mbox_id, void* data, uint32_t max_size,
                           uint32_t* out_size, uint32_t* out_sender) {
    int32_t result = mbox_recv(mbox_id, data, max_size, out_size, out_sender);

    if (result == IPC_ERROR_EMPTY) {
        // Bloquer le processus courant
        mailbox_t* mbox = find_mailbox_by_id(mbox_id);
        if (mbox != NULL) {
            process_t* current = scheduler_get_current();
            if (current != NULL) {
                current->block_reason = BLOCK_REASON_MBOX_EMPTY;
                current->block_resource = mbox;
                mbox->waiting_recv = current;
                scheduler_block_process(current);
                // Apres deblocage, reessayer
                return mbox_recv(mbox_id, data, max_size, out_size, out_sender);
            }
        }
    }

    return result;
}

int32_t mbox_count(uint32_t mbox_id) {
    mailbox_t* mbox = find_mailbox_by_id(mbox_id);
    if (mbox == NULL) {
        return IPC_ERROR_NOTFOUND;
    }
    return (int32_t)mbox->count;
}

void ipc_print_mailboxes(void) {
    printf("\n=== Mailboxes IPC ===\n");
    printf("ID   | Nom              | Msgs | Sent | Recv | Wait\n");
    printf("-----|------------------|------|------|------|------\n");

    bool found = false;
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].active) {
            found = true;
            mailbox_t* m = &mailboxes[i];
            printf("%-4u | %-16s | %4u | %4u | %4u | %c%c\n",
                   m->id,
                   m->name,
                   m->count,
                   m->total_sent,
                   m->total_received,
                   m->waiting_send ? 'S' : '-',
                   m->waiting_recv ? 'R' : '-');
        }
    }

    if (!found) {
        printf("(aucune mailbox)\n");
    }

    printf("\nLegende Wait: S=send bloque, R=recv bloque\n\n");
}

void ipc_test(void) {
    printf("\n=== Test du systeme IPC ===\n\n");

    // Test 1: Creation de mailbox
    printf("Test 1: Creation de mailbox\n");
    int32_t mbox1 = mbox_create("test_mbox");
    printf("  Mailbox creee: ID=%d\n", mbox1);

    // Test 2: Envoi de message
    printf("\nTest 2: Envoi de message\n");
    const char* msg = "Hello IPC!";
    int32_t result = mbox_send(mbox1, msg, 11);
    printf("  Envoi '%s': %s\n", msg, result == IPC_SUCCESS ? "OK" : "ERREUR");

    // Test 3: Comptage
    printf("\nTest 3: Comptage\n");
    int32_t count = mbox_count(mbox1);
    printf("  Messages dans mailbox: %d\n", count);

    // Test 4: Reception de message
    printf("\nTest 4: Reception de message\n");
    char buffer[64];
    uint32_t size;
    uint32_t sender;
    result = mbox_recv(mbox1, buffer, sizeof(buffer), &size, &sender);
    if (result == IPC_SUCCESS) {
        buffer[size] = '\0';
        printf("  Recu '%s' (taille=%u, sender=%u)\n", buffer, size, sender);
    } else {
        printf("  Erreur reception: %d\n", result);
    }

    // Test 5: Reception sur mailbox vide
    printf("\nTest 5: Reception sur mailbox vide\n");
    result = mbox_recv(mbox1, buffer, sizeof(buffer), &size, &sender);
    printf("  Resultat: %s\n", result == IPC_ERROR_EMPTY ? "EMPTY (attendu)" : "ERREUR");

    // Test 6: Envoi multiple jusqu'a plein
    printf("\nTest 6: Envoi multiple (remplissage)\n");
    for (int i = 0; i < MAILBOX_CAPACITY + 1; i++) {
        result = mbox_send(mbox1, "X", 1);
        if (result != IPC_SUCCESS) {
            printf("  Mailbox pleine apres %d messages\n", i);
            break;
        }
    }

    // Test 7: Find par nom
    printf("\nTest 7: Recherche par nom\n");
    int32_t found_id = mbox_find("test_mbox");
    printf("  'test_mbox' -> ID=%d\n", found_id);

    found_id = mbox_find("inexistant");
    printf("  'inexistant' -> %s\n",
           found_id == IPC_ERROR_NOTFOUND ? "NON TROUVE (attendu)" : "ERREUR");

    // Test 8: Destruction
    printf("\nTest 8: Destruction\n");
    result = mbox_destroy(mbox1);
    printf("  Destruction: %s\n", result == IPC_SUCCESS ? "OK" : "ERREUR");

    // Afficher l'etat final
    ipc_print_mailboxes();

    printf("=== Test IPC termine ===\n\n");
}
