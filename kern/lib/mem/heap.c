
/**
 *  A simple heap implementation based off of Stanford's PintOS implementation. All credits
 *  go to Ben Pfaff.
 **/

#include <heap.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <synch.h>
#include <paging.h>
#include <thread.h>
#include <util/debug.h>
#include <util/list.h>
#include <heapstruct.h>

/* Acquire the descriptor lock if we have multiprogramming
   enabled. */
static inline void desc_lock_acquire(struct heap_desc *d)
{
	if (threads_init)
		lock_acquire(&d->desc_lock);
}

/* Release the descriptor lock if we have multiprogramming
   available. */
static inline void desc_lock_release(struct heap_desc *d)
{
	if (threads_init)
		lock_release(&d->desc_lock);
}

static struct heap_desc descriptors[16];

void init_heap(void)
{
	/* Initialize heap descriptors. */
	size_t i;
	for (i = 0; i < sizeof descriptors / sizeof *descriptors; ++i) {
		size_t block_size = (8 << i);
		struct heap_desc *d = (descriptors + i);
		d->block_size = block_size;
		d->arena_size = (PAGESIZE - sizeof(struct heap_arena)) / block_size;
		lock_init(&d->desc_lock);
		list_init(&d->free_list);
	}
}

void *kmalloc(size_t size)
{
	struct heap_block *b;
	struct heap_arena *a;
	struct heap_desc *d;

	if (size == 0)
		b = NULL;
	else {
		size_t i;
		for (i = 0; i < sizeof descriptors / sizeof *descriptors; ++i)
			if (descriptors[i].block_size >= size)
				break;
		
		if (i == sizeof descriptors / sizeof *descriptors) {

		} else {
			d = (descriptors + i);

			desc_lock_acquire(d);

			if (list_empty (&d->free_list)) {
				a = page_allocate(PAL_ZERO);

				a->desc = d;
				a->free_count = d->arena_size;
				a->magic = ARENA_MAGIC;

				for (i = 0; i < d->arena_size; ++i) {
					b = arena_get_block(a, i);
					list_push_back(&d->free_list, &b->free_elem);
				}
			}
			
			b = elem_value(list_pop_front(&d->free_list), struct heap_block, free_elem);
			memset(b, 0, d->block_size);
			a = block_get_arena(b);
			assert(a->magic == ARENA_MAGIC);
			a->free_count--;
			desc_lock_release(d);
		}
	}
	
	return b;
}

void *kcalloc(size_t nmemb, size_t size)
{
	/* By virtue of memory management, we will always
	   zero out kernel pages upon allocation, and heap
	   blocks upon calls to kmalloc(). */
	return kmalloc(nmemb * size);
}

void kfree(void *p)
{
	if (p == NULL)
		return;

	struct heap_block *b = p;
	struct heap_arena *a = block_get_arena(p);
	struct heap_desc *d = a->desc;
	desc_lock_acquire(d);

	list_push_back(&d->free_list, &b->free_elem);

	a->free_count++;
	if (a->free_count == d->arena_size) {
		size_t block;
		for (block = 0; block < d->arena_size; ++block)
			list_remove(&d->free_list, &arena_get_block(a, block)->free_elem);

		page_free(a);
	} else {
		list_push_back(&d->free_list, &b->free_elem);
	}

	desc_lock_release(d);
}