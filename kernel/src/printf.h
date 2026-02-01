#ifndef PRINTF_H
#define PRINTF_H
#include <stdarg.h>
#include <stddef.h>
int printf(const char* format, ...);
int vprintf(const char* format, va_list args);
int snprintf(char* buf, size_t size, const char* format, ...);
#endif