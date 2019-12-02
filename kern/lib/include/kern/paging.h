#ifndef DUNKOS_PAGING_H
#define DUNKOS_PAGING_H

#include <interrupt.h>
#include <stddef.h>
#include <stdbool.h>

void init_paging(void);

enum page_allocation_flags {
	PAL_ZERO = 1,
	PAL_USER
};

void *page_allocate_multiple(enum page_allocation_flags, size_t);
void *page_allocate(enum page_allocation_flags);
void page_free(void *);

void *get_user_base(void);
void *get_kernel_base(void);

size_t get_user_pages(void);
size_t get_kernel_pages(void);

#endif