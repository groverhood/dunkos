#ifndef DUNKOS_VMMGMT_H
#define DUNKOS_VMMGMT_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <synch.h>
#include <util/list.h>
#include <memory.h>
#include <filesys.h>
#include <asm.h>

/* Frame table. Used as a cursor over the page allocation interface. */

struct frame {
    struct list aliases;
    atomic_bool pin;
};

void init_frame_table(void);

struct frame *allocate_frame(void);
struct frame *evict_frame(void);


static inline void frame_pin(struct frame *f)
{
    barrier();
    atomic_store(&f->pin, true);
}

static inline void frame_unpin(struct frame *f)
{
    barrier();
    atomic_store(&f->pin, false);
}

static inline bool frame_pinned(struct frame *f)
{
    barrier();
    return atomic_load(&f->pin);
}

void *frame_to_kernaddr(struct frame *);

/* Disk table. Used to store volatile/code memory upon eviction. */

struct page_disk_storage {
    /* Offset either into a file or a sector number. */
    off_t offset;
    struct file *source;
};

/* Supplemental page table interface. Use this as opposed to the PML4 interface. */
struct page_table;

enum readonly_violation_policy {
    ROVP_EXCEPT,
    ROVP_COPY
};

struct page {
    struct frame *phys;
    void *useraddr;

    struct list_elem bucket_elem;
    struct list_elem alias_elem;

    bool ondisk;
    bool readonly;

    enum readonly_violation_policy rovp;
    struct page_disk_storage diskrep;
};

void page_read_from_disk(struct page *);
void page_write_to_disk(struct page *);

void page_table_create(struct page_table **);
void page_table_copy(struct page_table *dest, const struct page_table *src);
void page_table_destroy(struct page_table *);

extern uintptr_t reserved_end;

#define USERLIMIT (void *)&reserved_end
#define STACKMAX (1 * GB)
#define STACKLIMIT ((void *)((size_t)PHYSBASE - STACKMAX))

bool page_is_valid(void *);
void page_load(void *);
void page_defer_load(void *, struct file *, off_t ofs, bool readonly);

#endif