/*
 * types.h - Types communs pour le kernel
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

// Définition de bool si nécessaire (pas de stdbool.h en bare-metal)
#ifndef __cplusplus
#ifndef _BOOL_DEFINED
#define _BOOL_DEFINED
typedef _Bool bool;
#define true 1
#define false 0
#endif
#endif

#endif // TYPES_H
