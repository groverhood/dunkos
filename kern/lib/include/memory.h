#ifndef DUNKOS_MEMORY_H
#define DUNKOS_MEMORY_H

#include <stddef.h>

/**
 *  Initialize OS virtual memory system.
 **/
void init_memory(void);

void* palloc(size_t bytes);

#endif