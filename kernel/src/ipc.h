/*
 * ipc.h - Communication Inter-Processus (Mailboxes)
 */

#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <stddef.h>
#include "process.h"

// =============================================================================
// Configuration des mailboxes
// =============================================================================

#define MAX_MAILBOXES       16          // Nombre max de mailboxes
#define MAILBOX_NAME_LEN    16          // Longueur max du nom
#define MAILBOX_CAPACITY    8           // Nombre max de messages par mailbox
#define MESSAGE_MAX_SIZE    64          // Taille max d'un message (octets)

// =============================================================================
// Codes de retour
// =============================================================================

#define IPC_SUCCESS         0
#define IPC_ERROR_FULL     -1           // Mailbox pleine
#define IPC_ERROR_EMPTY    -2           // Mailbox vide
#define IPC_ERROR_NOTFOUND -3           // Mailbox inexistante
#define IPC_ERROR_EXISTS   -4           // Mailbox existe deja
#define IPC_ERROR_NOSLOT   -5           // Plus de slot disponible
#define IPC_ERROR_PARAM    -6           // Parametre invalide

// =============================================================================
// Structures
// =============================================================================

// Message IPC
typedef struct {
    uint32_t sender_pid;                // PID de l'expediteur
    uint32_t size;                      // Taille des donnees
    uint8_t data[MESSAGE_MAX_SIZE];     // Donnees du message
} ipc_message_t;

// Mailbox
typedef struct {
    uint32_t id;                        // ID unique
    char name[MAILBOX_NAME_LEN];        // Nom de la mailbox
    bool active;                        // Mailbox active/valide

    // Buffer circulaire de messages
    ipc_message_t messages[MAILBOX_CAPACITY];
    uint32_t head;                      // Index de lecture
    uint32_t tail;                      // Index d'ecriture
    uint32_t count;                     // Nombre de messages

    // File d'attente des processus bloques
    process_t* waiting_send;            // Processus en attente d'envoi (full)
    process_t* waiting_recv;            // Processus en attente de reception (empty)

    // Statistiques
    uint32_t total_sent;                // Messages envoyes
    uint32_t total_received;            // Messages recus
} mailbox_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le systeme IPC
 */
void ipc_init(void);

/**
 * Cree une nouvelle mailbox
 *
 * @param name Nom de la mailbox
 * @return ID de la mailbox (>= 0) ou code erreur (< 0)
 */
int32_t mbox_create(const char* name);

/**
 * Supprime une mailbox
 *
 * @param id ID de la mailbox
 * @return IPC_SUCCESS ou code erreur
 */
int32_t mbox_destroy(uint32_t id);

/**
 * Trouve une mailbox par son nom
 *
 * @param name Nom de la mailbox
 * @return ID de la mailbox ou IPC_ERROR_NOTFOUND
 */
int32_t mbox_find(const char* name);

/**
 * Envoie un message (non bloquant)
 *
 * @param mbox_id ID de la mailbox
 * @param data    Donnees a envoyer
 * @param size    Taille des donnees
 * @return IPC_SUCCESS, IPC_ERROR_FULL, ou autre erreur
 */
int32_t mbox_send(uint32_t mbox_id, const void* data, uint32_t size);

/**
 * Recoit un message (non bloquant)
 *
 * @param mbox_id    ID de la mailbox
 * @param data       Buffer pour les donnees
 * @param max_size   Taille max du buffer
 * @param out_size   Taille reelle du message (sortie)
 * @param out_sender PID de l'expediteur (sortie, peut etre NULL)
 * @return IPC_SUCCESS, IPC_ERROR_EMPTY, ou autre erreur
 */
int32_t mbox_recv(uint32_t mbox_id, void* data, uint32_t max_size,
                  uint32_t* out_size, uint32_t* out_sender);

/**
 * Envoie un message (bloquant si plein)
 * Bloque le processus courant si la mailbox est pleine
 */
int32_t mbox_send_blocking(uint32_t mbox_id, const void* data, uint32_t size);

/**
 * Recoit un message (bloquant si vide)
 * Bloque le processus courant si la mailbox est vide
 */
int32_t mbox_recv_blocking(uint32_t mbox_id, void* data, uint32_t max_size,
                           uint32_t* out_size, uint32_t* out_sender);

/**
 * Retourne le nombre de messages dans une mailbox
 */
int32_t mbox_count(uint32_t mbox_id);

/**
 * Affiche l'etat de toutes les mailboxes
 */
void ipc_print_mailboxes(void);

/**
 * Test du systeme IPC
 */
void ipc_test(void);

#endif // IPC_H
