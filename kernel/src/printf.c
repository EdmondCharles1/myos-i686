/*
 * printf.c - Implementation complete et robuste de printf pour Kernel
 * Supporte: %s, %c, %d, %u, %x, %X, %p
 * Options:  Largeur (ex: %10d), Alignement gauche (ex: %-10d), Zéro-padding (ex: %08x)
 */

#include "printf.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// =============================================================================
// DÉPENDANCES EXTERNES
// =============================================================================

// Ces fonctions doivent exister dans votre driver VGA/Terminal (terminal.c)
extern void terminal_putchar(char c);
extern void terminal_write(const char* data, size_t size);

// Wrapper interne pour simplifier l'envoi d'un caractère
static void _putchar(char c) {
    terminal_putchar(c);
}

// =============================================================================
// UTILITAIRES INTERNES
// =============================================================================

static size_t _strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

static bool _is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Convertit un entier en string dans un buffer local
// Retourne la longueur de la chaîne générée
static int _itoa(char* buffer, int32_t value, int base, bool uppercase, bool is_signed) {
    char* ptr = buffer;
    char* low = buffer;
    
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    bool negative = false;
    uint32_t uvalue;

    // Gestion du signe pour la base 10
    if (is_signed && base == 10 && value < 0) {
        negative = true;
        uvalue = (uint32_t)(-value);
    } else {
        uvalue = (uint32_t)value;
    }

    // Extraction des chiffres (ordre inverse)
    if (uvalue == 0) {
        *ptr++ = '0';
    } else {
        do {
            *ptr++ = digits[uvalue % base];
            uvalue /= base;
        } while (uvalue);
    }

    if (negative) {
        *ptr++ = '-';
    }

    *ptr = '\0'; // Terminateur

    // Inverser la chaîne (car générée à l'envers)
    int length = ptr - buffer;
    ptr--; 
    while (low < ptr) {
        char tmp = *low;
        *low = *ptr;
        *ptr = tmp;
        low++;
        ptr--;
    }

    return length;
}

// =============================================================================
// LOGIQUE DE FORMATAGE
// =============================================================================

// Structure pour stocker les options parsées (ex: %-10d)
typedef struct {
    bool align_left;   // Flag '-'
    bool pad_zeros;    // Flag '0'
    int width;         // Largeur (ex: 10)
} fmt_options_t;

// Imprime une chaîne en respectant le padding et l'alignement
static int print_padded(const char* str, int len, fmt_options_t* opts) {
    int written = 0;
    int padding = opts->width - len;
    if (padding < 0) padding = 0;
    
    // Si aligné à gauche, on ignore le zéro padding (standard printf)
    char pad_char = (opts->pad_zeros && !opts->align_left) ? '0' : ' ';

    // CAS 1 : Aligné à gauche (Flag '-') -> "Texte   "
    if (opts->align_left) {
        // 1. Texte
        for (int i = 0; i < len; i++) {
            _putchar(str[i]);
            written++;
        }
        // 2. Padding (espaces)
        while (padding > 0) {
            _putchar(' '); 
            written++;
            padding--;
        }
    } 
    // CAS 2 : Aligné à droite (Défaut) -> "   Texte" ou "000Texte"
    else {
        // 1. Padding
        while (padding > 0) {
            _putchar(pad_char);
            written++;
            padding--;
        }
        // 2. Texte
        for (int i = 0; i < len; i++) {
            _putchar(str[i]);
            written++;
        }
    }
    return written;
}

// =============================================================================
// MOTEUR PRINCIPAL (vprintf)
// =============================================================================

int vprintf(const char* format, va_list args) {
    int written = 0;
    char buffer[64]; // Buffer assez grand pour int binaire 64 bits ou autre

    while (*format) {
        // --- 1. Caractère normal ---
        if (*format != '%') {
            _putchar(*format);
            format++;
            written++;
            continue;
        }

        // --- 2. Début d'un format ---
        format++; // Sauter le '%'

        // Vérification de fin de chaîne abrupte
        if (*format == '\0') break;

        // Parsing des options
        fmt_options_t opts = { .align_left = false, .pad_zeros = false, .width = 0 };

        // A. Flags (ex: '-' ou '0')
        while (*format == '-' || *format == '0') {
            if (*format == '-') opts.align_left = true;
            if (*format == '0') opts.pad_zeros = true;
            format++;
        }
        
        // Conflit: alignement gauche annule le padding zéro
        if (opts.align_left) opts.pad_zeros = false;

        // B. Largeur (Width) (ex: '16' dans %-16s)
        while (_is_digit(*format)) {
            opts.width = opts.width * 10 + (*format - '0');
            format++;
        }

        // C. Spécificateur de type
        char specifier = *format;
        format++; // Passer le spécificateur

        switch (specifier) {
            case 'c': { // Char
                char c = (char)va_arg(args, int);
                char str[2] = {c, '\0'};
                written += print_padded(str, 1, &opts);
                break;
            }
            case 's': { // String
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                written += print_padded(s, _strlen(s), &opts);
                break;
            }
            case 'd':   // Signed Int (Decimal)
            case 'i': {
                int val = va_arg(args, int);
                int len = _itoa(buffer, val, 10, false, true);
                written += print_padded(buffer, len, &opts);
                break;
            }
            case 'u': { // Unsigned Int (Decimal)
                uint32_t val = va_arg(args, uint32_t);
                int len = _itoa(buffer, (int32_t)val, 10, false, false);
                written += print_padded(buffer, len, &opts);
                break;
            }
            case 'x': { // Hex min
                uint32_t val = va_arg(args, uint32_t);
                int len = _itoa(buffer, (int32_t)val, 16, false, false);
                written += print_padded(buffer, len, &opts);
                break;
            }
            case 'X': { // Hex maj
                uint32_t val = va_arg(args, uint32_t);
                int len = _itoa(buffer, (int32_t)val, 16, true, false);
                written += print_padded(buffer, len, &opts);
                break;
            }
            case 'p': { // Pointer
                uint32_t val = va_arg(args, uint32_t);
                // On force le format 0x...
                _putchar('0'); _putchar('x'); written += 2;
                int len = _itoa(buffer, (int32_t)val, 16, false, false);
                
                // Ajustement largeur pour le '0x' déjà écrit
                if (opts.width > 2) opts.width -= 2;
                opts.pad_zeros = true; // Souvent on veut 0x000123
                
                written += print_padded(buffer, len, &opts);
                break;
            }
            case '%': {
                _putchar('%');
                written++;
                break;
            }
            default: {
                // Format inconnu (ou fin de chaine), on affiche le %
                _putchar('%');
                if (specifier) { // Si ce n'est pas la fin de string
                     _putchar(specifier);
                     written += 2;
                } else {
                    written++;
                }
                break;
            }
        }
    }
    return written;
}

// =============================================================================
// FONCTIONS PUBLIQUES
// =============================================================================

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = vprintf(format, args);
    va_end(args);
    return written;
}

// Stub pour snprintf (pour éviter les erreurs de link si utilisé ailleurs)
int snprintf(char* buf, size_t size, const char* format, ...) {
    // Implémentation minimale: renvoie 0 pour l'instant
    // Nécessiterait une logique similaire à vprintf mais écrivant dans buf
    (void)buf;
    (void)size;
    (void)format;
    return 0;
}