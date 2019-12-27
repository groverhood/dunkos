#ifndef DUNKOS_STDLIB_H
#define DUNKOS_STDLIB_H

#include <stddef.h>

void *malloc(size_t sz);
void *calloc(size_t nmemb, size_t sz);

void free(void *);

#endif