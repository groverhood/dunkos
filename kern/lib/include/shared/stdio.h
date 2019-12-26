#ifndef DUNKOS_STDIO_H
#define DUNKOS_STDIO_H

#include <stddef.h>
#include <stdarg.h>

int puts(const char *s);

int vsprintf(char *dest, const char *format, va_list argv);

int sprintf(char *dest, const char *format, ...);

int printf(const char *format, ...);

int putchar(int c);

#endif