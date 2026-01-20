/*
 * shell.h - Mini-shell interactif
 */

#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

#define SHELL_BUFFER_SIZE   256
#define SHELL_MAX_ARGS      16

/**
 * Initialise le shell
 */
void shell_init(void);

/**
 * Démarre la boucle principale du shell (bloquante)
 */
void shell_run(void);

/**
 * Exécute une commande
 */
void shell_execute(const char* command);

#endif // SHELL_H
