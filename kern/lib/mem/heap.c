
/**
 *  A simple heap implementation based off of Stanford's PintOS implementation. All credits
 *  go to Ben Pfaff.
 **/

#include <kern/heap.h>
#include <kern/memory.h>
#include <stdint.h>
#include <string.h>
#include <kern/synch.h>
#include <kern/paging.h>
#include <kern/thread.h>
#include <util/list.h>

struct heap_desc {
	size_t block_size;
	/* Size of arena in blocks. */
	size_t arena_size;
	struct list free_list;
	struct lock desc_lock;
};

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

/* Magic number for detecting arena corruption. */
#define ARENA_MAGIC 0x9a548eed

struct heap_arena {
	size_t magic;
	struct heap_desc *desc;
	size_t free_count;
};

struct heap_block {
	struct list_elem free_elem;
};

static struct heap_desc descriptors[16];

static struct heap_arena *block_get_arena(struct heap_block *);
static struct heap_block *arena_get_block(struct heap_arena *, size_t i);

void init_heap(void)
{
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

void *malloc(size_t size)
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
			a->free_count--;

			desc_lock_release(d);
		}
	}
	
	return b;
}

void *calloc(size_t nmemb, size_t size)
{
	/* By virtue of memory management, we will always
	   zero out kernel pages upon allocation, and heap
	   blocks upon calls to malloc(). */
	return malloc(nmemb * size);
}

void free(void *p)
{
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

static struct heap_arena *block_get_arena(struct heap_block *b)
{
	return page_round_down(b);
}

static struct heap_block *arena_get_block(struct heap_arena *a, size_t i)
{
	return (struct heap_block *)((uint8_t *)a + (sizeof *a) + i * a->desc->block_size);
}