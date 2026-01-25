/*
 * ipc.h - Inter-Process Communication (Mailbox / Message Queue)
 */

#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include "types.h"

// =============================================================================
// Configuration
// =============================================================================

#define MAX_MAILBOXES       8       // Nombre maximum de mailboxes
#define MAILBOX_NAME_LEN    16      // Longueur max du nom
#define MAX_MESSAGES        16      // Messages max par mailbox
#define MESSAGE_DATA_SIZE   64      // Taille max des données par message

// =============================================================================
// Structures
// =============================================================================

// Message dans une mailbox
typedef struct {
    uint32_t sender_pid;            // PID de l'expéditeur
    uint32_t receiver_pid;          // PID du destinataire (0 = broadcast)
    uint32_t type;                  // Type de message (défini par l'utilisateur)
    uint32_t timestamp;             // Tick d'envoi
    uint8_t data[MESSAGE_DATA_SIZE]; // Données du message
    uint32_t data_len;              // Longueur des données
} message_t;

// Mailbox (file de messages)
typedef struct {
    uint32_t id;                    // ID de la mailbox
    char name[MAILBOX_NAME_LEN];    // Nom de la mailbox
    bool in_use;                    // Mailbox active?

    message_t messages[MAX_MESSAGES]; // Buffer circulaire de messages
    uint32_t read_pos;              // Position de lecture
    uint32_t write_pos;             // Position d'écriture
    uint32_t count;                 // Nombre de messages en attente

    uint32_t owner_pid;             // PID du propriétaire
    uint32_t total_sent;            // Statistique: messages envoyés
    uint32_t total_received;        // Statistique: messages reçus
} mailbox_t;

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le système IPC
 */
void ipc_init(void);

/**
 * Crée une nouvelle mailbox
 *
 * @param name Nom de la mailbox
 * @return ID de la mailbox, ou 0 en cas d'erreur
 */
uint32_t mailbox_create(const char* name);

/**
 * Supprime une mailbox
 *
 * @param id ID de la mailbox
 * @return true si succès
 */
bool mailbox_destroy(uint32_t id);

/**
 * Trouve une mailbox par son nom
 *
 * @param name Nom de la mailbox
 * @return ID de la mailbox, ou 0 si non trouvée
 */
uint32_t mailbox_find(const char* name);

/**
 * Envoie un message dans une mailbox
 *
 * @param mailbox_id ID de la mailbox
 * @param type Type de message
 * @param data Données du message
 * @param len Longueur des données
 * @return true si succès
 */
bool mailbox_send(uint32_t mailbox_id, uint32_t type, const void* data, uint32_t len);

/**
 * Reçoit un message d'une mailbox (non bloquant)
 *
 * @param mailbox_id ID de la mailbox
 * @param out_msg Pointeur vers le message à remplir
 * @return true si un message a été reçu
 */
bool mailbox_receive(uint32_t mailbox_id, message_t* out_msg);

/**
 * Vérifie si une mailbox a des messages en attente
 *
 * @param mailbox_id ID de la mailbox
 * @return Nombre de messages en attente
 */
uint32_t mailbox_pending(uint32_t mailbox_id);

/**
 * Affiche les informations sur toutes les mailboxes
 */
void ipc_print_mailboxes(void);

/**
 * Affiche les statistiques IPC
 */
void ipc_print_stats(void);

#endif // IPC_H
