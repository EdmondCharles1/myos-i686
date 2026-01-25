/*
 * ipc.c - Inter-Process Communication (Mailbox / Message Queue)
 */

#include "ipc.h"
#include "printf.h"
#include "timer.h"
#include "process.h"

// =============================================================================
// Variables globales
// =============================================================================

static mailbox_t mailboxes[MAX_MAILBOXES];
static uint32_t next_mailbox_id = 1;
static uint32_t total_messages_sent = 0;
static uint32_t total_messages_received = 0;

// =============================================================================
// Fonctions utilitaires
// =============================================================================

static void string_copy(char* dest, const char* src, uint32_t max_len) {
    uint32_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int string_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void memory_copy(void* dest, const void* src, uint32_t len) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
}

// =============================================================================
// Implémentation
// =============================================================================

void ipc_init(void) {
    printf("[IPC] Initialisation du systeme IPC...\n");

    for (int i = 0; i < MAX_MAILBOXES; i++) {
        mailboxes[i].id = 0;
        mailboxes[i].in_use = false;
        mailboxes[i].read_pos = 0;
        mailboxes[i].write_pos = 0;
        mailboxes[i].count = 0;
        mailboxes[i].owner_pid = 0;
        mailboxes[i].total_sent = 0;
        mailboxes[i].total_received = 0;
    }

    next_mailbox_id = 1;
    total_messages_sent = 0;
    total_messages_received = 0;

    printf("[IPC] Systeme IPC initialise (%d mailboxes max)\n", MAX_MAILBOXES);
}

uint32_t mailbox_create(const char* name) {
    if (name == NULL) {
        return 0;
    }

    // Chercher un slot libre
    mailbox_t* mbox = NULL;
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (!mailboxes[i].in_use) {
            mbox = &mailboxes[i];
            break;
        }
    }

    if (mbox == NULL) {
        printf("[IPC] Erreur: plus de mailbox disponible\n");
        return 0;
    }

    // Initialiser la mailbox
    mbox->id = next_mailbox_id++;
    string_copy(mbox->name, name, MAILBOX_NAME_LEN);
    mbox->in_use = true;
    mbox->read_pos = 0;
    mbox->write_pos = 0;
    mbox->count = 0;

    process_t* current = process_get_current();
    mbox->owner_pid = current ? current->pid : 0;

    mbox->total_sent = 0;
    mbox->total_received = 0;

    printf("[IPC] Mailbox creee: id=%u, nom='%s'\n", mbox->id, mbox->name);

    return mbox->id;
}

bool mailbox_destroy(uint32_t id) {
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].in_use && mailboxes[i].id == id) {
            printf("[IPC] Mailbox detruite: id=%u, nom='%s'\n",
                   mailboxes[i].id, mailboxes[i].name);
            mailboxes[i].in_use = false;
            mailboxes[i].id = 0;
            return true;
        }
    }
    return false;
}

uint32_t mailbox_find(const char* name) {
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].in_use &&
            string_compare(mailboxes[i].name, name) == 0) {
            return mailboxes[i].id;
        }
    }
    return 0;
}

static mailbox_t* get_mailbox(uint32_t id) {
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].in_use && mailboxes[i].id == id) {
            return &mailboxes[i];
        }
    }
    return NULL;
}

bool mailbox_send(uint32_t mailbox_id, uint32_t type, const void* data, uint32_t len) {
    mailbox_t* mbox = get_mailbox(mailbox_id);
    if (mbox == NULL) {
        return false;
    }

    // Vérifier si la mailbox est pleine
    if (mbox->count >= MAX_MESSAGES) {
        printf("[IPC] Mailbox '%s' pleine!\n", mbox->name);
        return false;
    }

    // Limiter la taille des données
    if (len > MESSAGE_DATA_SIZE) {
        len = MESSAGE_DATA_SIZE;
    }

    // Créer le message
    message_t* msg = &mbox->messages[mbox->write_pos];

    process_t* current = process_get_current();
    msg->sender_pid = current ? current->pid : 0;
    msg->receiver_pid = 0;  // Broadcast par défaut
    msg->type = type;
    msg->timestamp = (uint32_t)timer_get_ticks();

    if (data != NULL && len > 0) {
        memory_copy(msg->data, data, len);
    }
    msg->data_len = len;

    // Avancer la position d'écriture
    mbox->write_pos = (mbox->write_pos + 1) % MAX_MESSAGES;
    mbox->count++;

    // Statistiques
    mbox->total_sent++;
    total_messages_sent++;

    return true;
}

bool mailbox_receive(uint32_t mailbox_id, message_t* out_msg) {
    mailbox_t* mbox = get_mailbox(mailbox_id);
    if (mbox == NULL || out_msg == NULL) {
        return false;
    }

    // Vérifier s'il y a des messages
    if (mbox->count == 0) {
        return false;
    }

    // Lire le message
    message_t* msg = &mbox->messages[mbox->read_pos];
    memory_copy(out_msg, msg, sizeof(message_t));

    // Avancer la position de lecture
    mbox->read_pos = (mbox->read_pos + 1) % MAX_MESSAGES;
    mbox->count--;

    // Statistiques
    mbox->total_received++;
    total_messages_received++;

    return true;
}

uint32_t mailbox_pending(uint32_t mailbox_id) {
    mailbox_t* mbox = get_mailbox(mailbox_id);
    if (mbox == NULL) {
        return 0;
    }
    return mbox->count;
}

void ipc_print_mailboxes(void) {
    printf("\n=== Mailboxes ===\n\n");

    bool found = false;
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].in_use) {
            found = true;
            mailbox_t* mbox = &mailboxes[i];
            printf("  [%u] '%s'\n", mbox->id, mbox->name);
            printf("      Messages: %u/%u\n", mbox->count, MAX_MESSAGES);
            printf("      Envoyes: %u, Recus: %u\n",
                   mbox->total_sent, mbox->total_received);
        }
    }

    if (!found) {
        printf("  (aucune mailbox)\n");
    }
    printf("\n");
}

void ipc_print_stats(void) {
    printf("\n=== Statistiques IPC ===\n\n");
    printf("  Messages envoyes:   %u\n", total_messages_sent);
    printf("  Messages recus:     %u\n", total_messages_received);

    uint32_t active = 0;
    for (int i = 0; i < MAX_MAILBOXES; i++) {
        if (mailboxes[i].in_use) {
            active++;
        }
    }
    printf("  Mailboxes actives:  %u/%u\n", active, MAX_MAILBOXES);
    printf("\n");
}
