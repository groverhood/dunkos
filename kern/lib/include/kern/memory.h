#ifndef DUNKOS_MEMORY_H
#define DUNKOS_MEMORY_H

#include <stddef.h>

#define PAGESIZE (0x200000)
#define MAXPHYSADDR ((void *)((1UL << 48UL)))

/**
 *  Initialize OS virtual memory system.
 **/
void init_memory(void);

void *palloc(void);

void pfree(void *page);

#endif