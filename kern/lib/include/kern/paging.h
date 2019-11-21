#ifndef DUNKOS_PAGING_H
#define DUNKOS_PAGING_H

#include <interrupt.h>
#include <stddef.h>
#include <stdbool.h>

void init_paging(void);

enum page_allocation_flags {
	PAL_ZERO,
	PAL_USER
};

void *page_allocate_multiple(enum page_allocation_flags, size_t);
void *page_allocate(enum page_allocation_flags);
void page_free(void *);

#endif