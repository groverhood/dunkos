
#include <pml4.h>
#include <heap.h>
#include <memory.h>
#include <paging.h>
#include <stdint.h>
#include <string.h>
#include <util/bitmap.h>

#define PML4_BLOCK_SIZE (512 * sizeof(size_t))
#define PML4_PGSIZE		(1 << 7)
#define PML4_DIRTY		(1 << 6)
#define PML4_ACCESSED 	(1 << 5)
#define PML4_WRITABLE 	(1 << 1)
#define PML4_PRESENT 	(1 << 0)

static size_t *base_pt_pool;
static size_t *default_pml4;

void init_pml4(void)
{
	extern size_t _pool_begin;

	base_pt_pool = &_pool_begin;
	__asm__ ("movq %%cr3, %0" : "=r" (default_pml4));
}

size_t *get_default_pml4(void)
{
	return default_pml4;
}

size_t *pml4_alloc(void)
{
	return kcalloc(512, sizeof(size_t));
}

void pml4_activate(size_t *pml4)
{
	if (!pml4)
		pml4 = default_pml4;

	__asm__ volatile ("movq %0, %%cr3" :: "r" (pml4));
}

void pml4_destroy(size_t *pml4)
{
	kfree(pml4);
}

void pml4_map_address_buffer(size_t *pml4, void *addr, void *frame)
{
	uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;
	size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);
	if (pdp == NULL) {
		pdp = base_pt_pool;
		base_pt_pool += 512;
		pml4[pml4_offset] = (size_t)pdp | (PML4_WRITABLE | PML4_PRESENT);
	}

	uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;
	size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);
	if (pgd == NULL) {
		pgd = base_pt_pool;
		base_pt_pool += 512;
		pdp[pdp_offset] = (size_t)pgd | (PML4_WRITABLE | PML4_PRESENT);
	}
	
	uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;
	pgd[pgd_offset] = (size_t)frame | (PML4_PGSIZE | PML4_WRITABLE | PML4_PRESENT);
}

void pml4_map_address(size_t *pml4, void *addr, void *frame)
{
	uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;
	size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);
	if (pdp == NULL) {
		pdp = kernel_to_phys(kcalloc(512, sizeof(size_t)));
		pml4[pml4_offset] = (size_t)pdp | (PML4_WRITABLE | PML4_PRESENT);
	}

	uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;
	size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);
	if (pdp == NULL) {
		pdp = kernel_to_phys(kcalloc(512, sizeof(size_t)));
		pdp[pml4_offset] = (size_t)pdp | (PML4_WRITABLE | PML4_PRESENT);
	}
	
	uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;
	pgd[pgd_offset] = (size_t) frame | (PML4_WRITABLE | PML4_PRESENT);
}

static inline size_t *pml4_compute_ptloc(size_t *pml4, void *addr)
{
	uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;
	size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);
	uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;
	size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);
	uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;
	return (pgd + pgd_offset);
}


void pml4_clear_mapping(size_t *pml4, void *addr)
{
	*pml4_compute_ptloc(pml4, addr) = 0;
}

#define pml4_set(pml4, addr, cond, value) do {\
			if (cond)\
				*pml4_compute_ptloc(pml4, addr) |= value;\
			else\
				*pml4_compute_ptloc(pml4, addr) &= ~(value);\
		} while(0)

void pml4_set_writable(size_t *pml4, void *addr, bool writable)
{
	pml4_set(pml4, addr, writable, PML4_WRITABLE);
}

void pml4_set_accessed(size_t *pml4, void *addr, bool accessed)
{
	pml4_set(pml4, addr, accessed, PML4_ACCESSED);
}

void pml4_set_dirty(size_t *pml4, void *addr, bool dirty)
{
	pml4_set(pml4, addr, dirty, PML4_DIRTY);
}

#define pml4_get(pml4, addr, test)\
	({\
		uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;\
		size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);\
		uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;\
		size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);\
		uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;\
		!!(pgd[pgd_offset] & test);\
	})

bool pml4_is_writable(size_t *pml4, void *addr)
{
	return pml4_get(pml4, addr, PML4_WRITABLE);
}

bool pml4_is_accessed(size_t *pml4, void *addr)
{
	return pml4_get(pml4, addr, PML4_ACCESSED);
}

bool pml4_is_dirty(size_t *pml4, void *addr)
{
	return pml4_get(pml4, addr, PML4_DIRTY);
}

static void _pml_copy(size_t *dest_pml, const size_t *src_pml, bool cow, int level)
{
	size_t i;
	int writable = (!cow << 1);
	if (level > 2) {
		for (i = 0; i < 512; ++i) {
			size_t e = src_pml[i];
			if (e & ~0xFFF) {
				size_t *pml = kcalloc(512, sizeof *pml);
				_pml_copy(pml, phys_to_kernel((void *)(e & ~0xFFF)), cow, level - 1);
				dest_pml[i] = (size_t)(kernel_to_phys(pml)) | ((e & 0xFFD) | writable);
			}
		}
	} else {
		for (i = 0; i < 512; ++i) {
			dest_pml[i] = (src_pml[i] & ~0xFFF) | ((src_pml[i] & 0xFFD) | writable);
		}
	}
}

void pml4_copy(size_t *dest_pml4, const size_t *src_pml4, bool cow)
{
	_pml_copy(dest_pml4, src_pml4, cow, 4);	
}

