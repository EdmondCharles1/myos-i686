/*
 * printf.h - Formatted printing for myos-i686 kernel
 * 
 * Inspiré de : https://wiki.osdev.org/Formatted_Printing
 */

#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// =============================================================================
// Terminal API (à implémenter dans kernel.c)
// =============================================================================

/**
 * Affiche un caractère
 */
void terminal_putchar(char c);

/**
 * Affiche une chaîne de caractères
 */
void terminal_write(const char* str);

// =============================================================================
// Printf API
// =============================================================================

/**
 * Printf formaté (version complète)
 * 
 * Formats supportés :
 * - %c : caractère
 * - %s : chaîne de caractères
 * - %d / %i : entier signé (décimal)
 * - %u : entier non signé (décimal)
 * - %x : hexadécimal minuscule
 * - %X : hexadécimal majuscule
 * - %p : pointeur (hexadécimal avec 0x)
 * - %% : symbole %
 */
int printf(const char* format, ...);

/**
 * Variante avec buffer limité (sécurisé)
 */
int snprintf(char* buf, size_t size, const char* format, ...);

/**
 * Conversions utilitaires
 */
void itoa(int value, char* str, int base);
void utoa(unsigned int value, char* str, int base);

#endif // PRINTF_H
