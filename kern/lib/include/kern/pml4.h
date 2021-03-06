#ifndef PML4_H
#define PML4_H

#include <stddef.h>
#include <stdbool.h>

size_t *pml4_alloc(void);
size_t *get_default_pml4(void);
void pml4_activate(size_t *);
void pml4_destroy(size_t *);

void pml4_map_address_buffer(size_t *pml4, void *addr, void *frame);
void pml4_map_address(size_t *pml4, void *addr, void *frame);
void pml4_clear_mapping(size_t *pml4, void *addr);

void pml4_set_writable(size_t *pml4, void *addr, bool writable);
void pml4_set_accessed(size_t *pml4, void *addr, bool accessed);
void pml4_set_dirty(size_t *pml4, void *addr, bool dirty);

bool pml4_is_writable(size_t *pml4, void *addr);
bool pml4_is_accessed(size_t *pml4, void *addr);
bool pml4_is_dirty(size_t *pml4, void *addr);

/* Copy src -> dest pml4. If cow is set, then upon writing to src's physical
   frames, dest will be allocated a new frame. */
void pml4_copy(size_t *dest_pml4, const size_t *src_pml4, bool cow);

/**
 *  Retrieves the current process's pml4,
 * 	the root of the paging system.
 **/
static inline size_t *get_pml4(void)
{
	size_t *pml4;

	__asm__ ("movq %%cr3, %0" : "=r" (pml4));
	return pml4;
}

#endif