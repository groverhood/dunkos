/**
 *  The heap algorithm here is very similar to the one ripped from PintOS,
 *  which is used in the kernel, but this time with an evil twist. To avoid
 *  excessive traps and shifting around of the bss limit, arenas will now
 *  steal from one another if not enough space is available. This will only
 *  occur so many times before it is decided that stealing more memory from
 *  the next arena will cause more harm than performance benefits.
 **/

#define MINDESCALLOC 3
#define MAXDESCALLOC 17
#define DESCNT ((MAXDESCALLOC) - (MINDESCALLOC))
#define HEAPBLOCKSZ (1 << (DESCNT))

#include <stdlib.h>
#include <stdbool.h>
#include <memunit.h>
#include <heapstruct.h>
#include <system.h>
#include <synch.h>
#include <util/list.h>

extern size_t _bheap;
/* The heap is the last part of the data segment. */
extern size_t _ebss;

/* Growable heap limit. */
static size_t maxheapsz = PAGESIZE, curheapsz = 0;

static void *heap_block_get(size_t sz, bool zero);
static void heap_block_free(void *ptr);

void *malloc(size_t sz)
{
    return heap_block_get(sz, false);
}

void *calloc(size_t nmemb, size_t sz)
{
    return heap_block_get(nmemb * sz, true);
}

void free(void *ptr)
{
    heap_block_free(ptr);
}

/* This allows for a maximum grow factor of 1.5. */
#define maxwhine(nxt) (int)(nxt->base.arena_size / 4)

struct heap_arena_whiner {
    struct heap_arena base;

    /* If our heap_arena complains of insufficient space too often,
       we may as well concede and allocate more memory to the data
       segment using growdata(). */
    int whine; 
};


/* Allocate one extra descriptor so that we don't need special logic to tell
   the largest one that it can't steal from its neighbor. */
static struct heap_desc descriptors[DESCNT + 1];

static void *heap_block_get(size_t sz, bool zero)
{
    
}

static void heap_block_free(void *ptr)
{

}

void init_userheap(void)
{
    const size_t base = 8, arenasz = (4 * PAGESIZE) / DESCNT;
    size_t i;
    for (i = 0; i < DESCNT; ++i) {
        struct heap_desc *d = (descriptors + i);
        d->block_size = (base << i);
        d->arena_size = (arenasz - sizeof(struct heap_arena_whiner)) / d->block_size;

        lock_init(&d->desc_lock);
        list_init(&d->free_list);
    }
}