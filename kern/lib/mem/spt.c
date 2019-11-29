
#include <kern/vmmgmt.h>
#include <kern/heap.h>
#include <kern/process.h>
#include <kern/pml4.h>

struct pt_con {
    size_t buckets, entries;
    struct list *bucket_con;
};

static void pt_con_init(struct pt_con *c)
{

}

static inline size_t pt_con_size(struct pt_con *c)
{
    return c->entries;
}

static void pt_con_insert(struct pt_con *c, struct page *p)
{

}

static struct page *pt_con_find(struct pt_con *c, void *useraddr)
{

}

struct page_table {
    struct pt_con con;
};

void page_table_create(struct page_table **ppt)
{
    struct page_table *pt = calloc(1, sizeof *pt);
    pt_con_init(&pt->con);
}

static inline bool page_within_segment(void *p)
{
    struct process *cur = current_process();
    uintptr_t ptr = (uintptr_t)p;

    return ((uintptr_t)cur->code_segment_begin <= ptr
    && ptr < (uintptr_t)cur->code_segment_end)
    || ((uintptr_t)cur->data_segment_begin <= ptr
    && ptr < (uintptr_t)cur->data_segment_end);
}

static inline bool page_valid_stack(void *p)
{
    uintptr_t ptr = (uintptr_t)p;
    uintptr_t sp = (uintptr_t)current_process()->base.context.sp;
    return ((uintptr_t)STACKLIMIT <= p) && (ptr >= (sp - 8));
}

static inline bool page_within_heap(void *p)
{
    struct process *cur = current_process();
    uintptr_t ptr = (uintptr_t)p;

    return ((uintptr_t)cur->heap_begin <= ptr
    && ptr < (uintptr_t)cur->heap_end);
}

bool page_is_valid(void *p)
{
    uintptr_t ptr = (uintptr_t)p;
    return ((uintptr_t)USERLIMIT <= ptr)
        && (ptr < (uintptr_t)PHYSBASE)
        && (page_within_segment(p) || page_valid_stack(p)
        || page_within_heap(p));
}

void page_load(void *p)
{
    struct process *pr = current_process();
    struct pt_con *con = &pr->spt->con;
    struct page *pg = pt_con_find(con, p);
    if (pg == NULL) {
        pg = calloc(1, sizeof *pg);
        pg->ondisk = false;
        pg->useraddr = p;
        pt_con_insert(con, &pg);
    }

    struct frame *f = allocate_frame();
    pg->phys = f;
    list_push_back(&f->aliases, &pg->alias_elem);

    pml4_map_address(pr->pml4, p, 
        kernel_to_phys(frame_to_kernaddr(f)));   

    if (pg->ondisk)
        page_read_from_disk(pg);
}