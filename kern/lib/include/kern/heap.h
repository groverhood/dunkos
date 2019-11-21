#ifndef DUNKOS_HEAP_H
#define DUNKOS_HEAP_H

#include <stddef.h>

void init_heap(void);

void *malloc(size_t);
void *calloc(size_t nmemb, size_t size);
void free(void *);

#endif