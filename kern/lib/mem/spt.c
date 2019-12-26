
#include <vmmgmt.h>
#include <heap.h>
#include <process.h>
#include <util/hashtable.h>
#include <pml4.h>
#include <string.h>
#include <algo.h>

struct page_table {
    struct hashtable con;
	/* The root of the process's virtual address space. */
	size_t *pml4;
};

static bool page_equal(struct list_elem *l, struct list_elem *r)
{
    return elem_value(l, struct page, bucket_elem)->useraddr
        == elem_value(r, struct page, bucket_elem)->useraddr;
}

static size_t page_hash(struct list_elem *e)
{
    return hash_uint64((uint64_t)elem_value(e, struct page, bucket_elem)->useraddr);
}

void page_table_create(struct page_table **ppt)
{
    struct page_table *pt = kcalloc(1, sizeof *pt);
    hashtable_init(&pt->con, &page_hash, &page_equal);
    pt->pml4 = pml4_alloc();

    *ppt = pt;
}

static void page_copy(struct list_elem *src_e, void *dest_pt_)
{
    struct page_table *dest_pt = dest_pt_;
    struct page *src_pg = elem_value(src_e, struct page, bucket_elem);

    struct page *pg = kcalloc(1, sizeof *pg);
    memcpy(pg, src_pg, sizeof *pg);

    /* Copy-on-write. */
    if (src_pg->rovp != ROVP_EXCEPT)
        pg->rovp = ROVP_COPY;
    pg->readonly = true;

    list_push_back(&pg->phys->aliases, &pg->alias_elem);
    hashtable_insert(&dest_pt->con, &pg->bucket_elem);

    pml4_map_address(dest_pt->pml4, pg->useraddr, frame_to_kernaddr(pg->phys));
}

void page_table_copy(struct page_table *dest, const struct page_table *src)
{
    hashtable_foreach(&src->con, &page_copy, dest);
}

static void page_destroy(struct list_elem *e, void *aux)
{
    struct page *pg = elem_value(e, struct page, bucket_elem);
    pml4_clear_mapping(current_process()->spt->pml4, pg->useraddr);
    kfree(pg);
}

void page_table_destroy(struct page_table *pt)
{
    hashtable_destroy(&pt->con, &page_destroy, NULL);
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
    struct page key, *pg;
    key.useraddr = p;
    struct process *pr = current_process();
    struct page_table *pt = pr->spt;
    struct hashtable *con = &pt->con;
    struct list_elem *pg_el = hashtable_find(con, &key.bucket_elem);
    
    if (pg_el == NULL) {
        pg = kcalloc(1, sizeof *pg);
        pg->ondisk = false;
        pg->useraddr = p;
        hashtable_insert(con, &pg);
    } else pg = elem_value(pg_el, struct page, bucket_elem);

    struct frame *f = allocate_frame();
    pg->phys = f;
    list_push_back(&f->aliases, &pg->alias_elem);

    pml4_map_address(pt->pml4, p, kernel_to_phys(frame_to_kernaddr(f)));   

    if (pg->ondisk)
        page_read_from_disk(pg);
}