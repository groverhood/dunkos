#ifndef DUNKOS_STRING_H
#define DUNKOS_STRING_H

#include <stddef.h>

size_t strlen(const char *str);

char *strchr(const char *str, int chr);

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void *dest, int val, size_t n);

int strncmp(const char *left, const char *right, size_t count);

#endif