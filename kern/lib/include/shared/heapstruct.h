#ifndef DUNKOS_HEAPSTRUCT_H
#define DUNKOS_HEAPSTRUCT_H

#include <synch.h>
#include <stdint.h>
#include <memunit.h>
#include <util/list.h>

#ifndef HEAPBLOCKSZ
#define HEAPBLOCKSZ PAGESIZE
#endif

struct heap_desc {
	size_t block_size;
	/* Size of arena in blocks. */
	size_t arena_size;
	struct list free_list;
	struct lock desc_lock;
};

/* Magic number for detecting arena corruption. */
#define ARENA_MAGIC 0x9A548EED

struct heap_arena {
	size_t magic;
	struct heap_desc *desc;
	size_t free_count;
};

struct heap_block {
	struct list_elem free_elem;
};

inline static struct heap_arena *block_get_arena(struct heap_block *b)
{
	return (struct heap_arena *)(((uintptr_t)b / HEAPBLOCKSZ) * HEAPBLOCKSZ);
}

inline static struct heap_block *arena_get_block(struct heap_arena *a, size_t i)
{
	return (struct heap_block *)((uint8_t *)a + (sizeof *a) + i * a->desc->block_size);
}

#endif