#ifndef DUNKOS_VMMGMT_H
#define DUNKOS_VMMGMT_H

#include <stdbool.h>
#include <stdint.h>
#include <kern/synch.h>
#include <util/list.h>
#include <kern/memory.h>

/* Frame table. Used as a cursor over the page allocation interface. */

struct frame {
    struct list aliases;
    struct lock pin_lock;
    bool pin;
};

void init_frame_table(void);

struct frame *allocate_frame(void);
struct frame *evict_frame(void);

void *frame_to_kernaddr(struct frame *);

/* Disk table. Used to store volatile/code memory upon eviction. */

struct page_disk_storage {
    /* Offset either into a file or a sector number. */
    uint64_t offset;
    struct file *source;
};

void page_read_from_disk(struct page *);
void page_write_to_disk(struct page *);

/* Supplemental page table interface. Use this as opposed to the PML4 interface. */
struct page_table;

struct page {
    struct frame *phys;
    void *useraddr;

    struct list_elem bucket_elem;
    struct list_elem alias_elem;

    bool ondisk;
};

void page_table_create(struct page_table **);
void page_table_copy(struct page_table *dest, const struct page_table *src);

extern void *reserved_end;

#define USERLIMIT reserved_end
#define STACKMAX (1 * GB)
#define STACKLIMIT ((void *)((size_t)PHYSBASE - STACKMAX))

bool page_is_valid(void *);
void page_load(void *);

#endif