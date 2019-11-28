#include <stdio.h>
#include <kern/paging.h>
#include <kern/memory.h>
#include <kern/pml4.h>
#include <util/bitmap.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <kern/synch.h>


/* The page fault handler. */
static interrupt_handler page_fault;
static size_t *default_pml4;

static inline uintptr_t get_faultaddr(void)
{
	uintptr_t a;
	__asm__ ("movq %%cr2, %0" : "=r" (a));

	return (a & ADDRMASK);
}

static enum interrupt_defer page_fault(struct interrupt *intr, 
								void *intrframe_,
								struct register_state *registers)
{
	struct fault_interrupt_frame *intrframe = intrframe_;
	unsigned long error = intrframe->error;

	void *faultaddr;
	__asm__ ("movq %%cr2, %0" : "=r" (faultaddr));
	size_t *pml4 = get_pml4();

	if (!(error & 0x1)) {
		pml4_map_address(pml4, page_round_down(faultaddr), page_allocate(0));
	}

	return INTRDEFR_NONE;
}

extern void prep_paging_ia32e(void);

struct page_pool {
	size_t pages;
	struct bitmap *occupancy_map;
	void *base;
	struct lock lck;
};

static void page_pool_init(struct page_pool *, size_t pages, void *base);
static void *page_pool_alloc(struct page_pool *, size_t npages);
static void page_pool_free(struct page_pool *, void *, size_t npages);
static bool page_pool_contains(struct page_pool *, void *);

static void page_pool_map(struct page_pool *, size_t *pml4);

static struct page_pool kernel_pool;
static struct page_pool user_pool;

/* The end of the kernel static data segment. */
static uintptr_t reserved_end;


void init_paging(void)
{
	extern size_t _reserved_end;
	reserved_end = (uintptr_t)&_reserved_end;

	/* Number of pages excluding those reserved for the kernel. */
	uint64_t total_pages = (PHYSSIZE - reserved_end) / PAGESIZE;
	uint64_t user_pages = total_pages / 2;
	uint64_t kernel_pages = total_pages - user_pages;

	uint8_t *base = phys_to_kernel(&_reserved_end);
	printf("Kernel page base: %p\n", base);
	printf("User page base: %p\n", base + kernel_pages * PAGESIZE);
	install_interrupt_handler(INTR_TYPE_PGFAULT, &page_fault);	


	size_t pgno;
	for (pgno = 0; pgno < total_pages; ++pgno) {
		uint8_t *kernaddr = base + pgno * PAGESIZE;
		pml4_map_address_buffer(get_default_pml4(), 
					kernaddr, kernel_to_phys(kernaddr));
	}
	
	page_pool_init(&kernel_pool, kernel_pages, base);
	page_pool_init(&user_pool, user_pages, base + kernel_pages * PAGESIZE);
}

void *page_allocate(enum page_allocation_flags flags)
{
	struct page_pool *pool = (flags & PAL_USER) ? &user_pool : &kernel_pool;

	void *page = page_pool_alloc(pool, 1);
	if (flags & PAL_ZERO)
		memset(page, 0, PAGESIZE);

	return page;
}

void page_free(void *page)
{
	void *frame = kernel_to_phys(page);
	struct page_pool *pool = page_pool_contains(&kernel_pool, frame) 
							? &kernel_pool : &user_pool;
	
	page_pool_free(pool, frame, 1);
}


static void page_pool_init(struct page_pool *p, size_t pages, void *base)
{
	size_t bitmap_pages = (size_t)page_round_up((void *)p->pages) / PAGESIZE;
	uint8_t *page_base = (uint8_t *)base + pages * PAGESIZE;
	size_t bmap_page;
	size_t *pml4 = get_default_pml4();

	p->occupancy_map = bitmap_create_buffer(pages, base, pages);
	p->base = (uint8_t *)base + bitmap_pages * PAGESIZE;
	p->pages = pages - bitmap_pages;
	lock_init(&p->lck);
}

static void *page_pool_alloc(struct page_pool *p, size_t npages)
{
	void *page;

	//lock_acquire(&p->lck);
	size_t pg_no = bitmap_scan_and_flip(p->occupancy_map, 0, npages, 0);

	//lock_release(&p->lck);

	if (pg_no == BITMAP_NPOS) {
		page = NULL;
	} else {
		uint8_t *base = p->base;
		page = base + pg_no * PAGESIZE;
	}

	return page;
}

static void page_pool_free(struct page_pool *p, void *pg, size_t npages)
{
	size_t pg_no = ((uintptr_t)pg - (uintptr_t)p->base) / PAGESIZE;
	bitmap_set(p->occupancy_map, pg_no, npages, 0);
}

static bool page_pool_contains(struct page_pool *pool, void *p)
{
	uintptr_t base = (uintptr_t)pool->base;
	return base <= (uintptr_t)p && (uintptr_t)p < (base + pool->pages * PAGESIZE);
}
