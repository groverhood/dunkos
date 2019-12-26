#ifndef DUNKOS_HEAP_H
#define DUNKOS_HEAP_H

#include <stddef.h>

void init_heap(void);

void *kmalloc(size_t);
void *kcalloc(size_t nmemb, size_t size);
void kfree(void *);

#endif