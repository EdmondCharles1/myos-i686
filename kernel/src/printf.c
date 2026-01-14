/*
 * printf.c - Implementation of formatted printing
 * 
 * Inspiré de : https://wiki.osdev.org/Formatted_Printing
 */

#include "printf.h"

// =============================================================================
// Fonctions de conversion
// =============================================================================

/**
 * Inverse une chaîne de caractères
 */
static void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/**
 * Convertit un entier en chaîne (base 2-36)
 */
void itoa(int value, char* str, int base) {
    int i = 0;
    int is_negative = 0;

    // Cas spécial : 0
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Gestion du signe négatif (uniquement en base 10)
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }

    // Conversion
    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }

    // Ajouter le signe négatif
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    // Inverser la chaîne
    reverse(str, i);
}

/**
 * Convertit un entier non signé en chaîne (base 2-36)
 */
void utoa(unsigned int value, char* str, int base) {
    int i = 0;

    // Cas spécial : 0
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Conversion
    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }

    str[i] = '\0';

    // Inverser la chaîne
    reverse(str, i);
}

/**
 * Convertit un entier non signé en hexadécimal majuscule
 */
static void utoa_hex_upper(unsigned int value, char* str) {
    int i = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while (value != 0) {
        int rem = value % 16;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        value = value / 16;
    }

    str[i] = '\0';
    reverse(str, i);
}

// =============================================================================
// strlen (utilitaire interne)
// =============================================================================

static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

// =============================================================================
// printf - Fonction principale
// =============================================================================

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int written = 0;
    char buffer[32]; // Buffer temporaire pour conversions

    for (size_t i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++; // Avancer au caractère suivant

            switch (format[i]) {
                case 'c': {
                    // Caractère
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    written++;
                    break;
                }

                case 's': {
                    // Chaîne de caractères
                    const char* s = va_arg(args, const char*);
                    if (s == NULL) {
                        s = "(null)";
                    }
                    terminal_write(s);
                    written += strlen(s);
                    break;
                }

                case 'd':
                case 'i': {
                    // Entier signé (décimal)
                    int value = va_arg(args, int);
                    itoa(value, buffer, 10);
                    terminal_write(buffer);
                    written += strlen(buffer);
                    break;
                }

                case 'u': {
                    // Entier non signé (décimal)
                    unsigned int value = va_arg(args, unsigned int);
                    utoa(value, buffer, 10);
                    terminal_write(buffer);
                    written += strlen(buffer);
                    break;
                }

                case 'x': {
                    // Hexadécimal minuscule
                    unsigned int value = va_arg(args, unsigned int);
                    utoa(value, buffer, 16);
                    terminal_write(buffer);
                    written += strlen(buffer);
                    break;
                }

                case 'X': {
                    // Hexadécimal majuscule
                    unsigned int value = va_arg(args, unsigned int);
                    utoa_hex_upper(value, buffer);
                    terminal_write(buffer);
                    written += strlen(buffer);
                    break;
                }

                case 'p': {
                    // Pointeur (hexadécimal avec préfixe 0x)
                    unsigned int value = va_arg(args, unsigned int);
                    terminal_write("0x");
                    utoa(value, buffer, 16);
                    terminal_write(buffer);
                    written += 2 + strlen(buffer);
                    break;
                }

                case '%': {
                    // Symbole %
                    terminal_putchar('%');
                    written++;
                    break;
                }

                default: {
                    // Format non reconnu : afficher tel quel
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    written += 2;
                    break;
                }
            }
        } else {
            // Caractère normal
            terminal_putchar(format[i]);
            written++;
        }
    }

    va_end(args);
    return written;
}

// =============================================================================
// snprintf - Version avec buffer limité
// =============================================================================

int snprintf(char* buf, size_t size, const char* format, ...) {
    // TODO: À implémenter si nécessaire
    // Pour l'instant, version simplifiée non implémentée
    (void)buf;
    (void)size;
    (void)format;
    return 0;
}