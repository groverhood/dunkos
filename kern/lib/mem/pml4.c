
#include <kern/pml4.h>
#include <kern/heap.h>
#include <kern/memory.h>
#include <kern/paging.h>
#include <stdint.h>
#include <string.h>

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
	return calloc(512, sizeof(size_t));
}

void pml4_activate(size_t *pml4)
{
	if (!pml4)
		pml4 = default_pml4;

	__asm__ volatile ("movq %0, %%cr3" :: "r" (pml4));
}

void pml4_destroy(size_t *pml4)
{
	free(pml4);
}

void pml4_map_address_buffer(size_t *pml4, void *addr, void *frame)
{
	uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;
	size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);
	if (pdp == NULL) {
		pdp = base_pt_pool;
		base_pt_pool += 512;
		pml4[pml4_offset] = (size_t)pdp | 0x3;
	}

	uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;
	size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);
	if (pgd == NULL) {
		pgd = base_pt_pool;
		base_pt_pool += 512;
		pdp[pdp_offset] = (size_t)pgd | 0x3;
	}
	
	uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;
	pgd[pgd_offset] = (size_t)frame | 0x83;
}

void pml4_map_address(size_t *pml4, void *addr, void *frame)
{
	uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;
	size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);
	if (pdp == NULL) {
		pdp = kernel_to_phys(calloc(512, sizeof(size_t)));
		pml4[pml4_offset] = (size_t)pdp | 0x3;
	}

	uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;
	size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);
	if (pdp == NULL) {
		pdp = kernel_to_phys(calloc(512, sizeof(size_t)));
		pdp[pml4_offset] = (size_t)pdp | 0x3;
	}
	
	uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;
	pgd[pgd_offset] = (size_t) frame | 0x3;
}

void pml4_set_writable(size_t *pml4, void *addr, bool writable)
{
	uintptr_t pml4_offset = (((uintptr_t)addr) & 0xFF8000000000) >> 39;
	size_t *pdp = (size_t *)(pml4[pml4_offset] & ~0xFFF);
	uintptr_t pdp_offset = (((uintptr_t)addr) & 0x7FC0000000) >> 30;
	size_t *pgd = (size_t *)(pdp[pdp_offset] & ~0xFFF);
	uintptr_t pgd_offset = (((uintptr_t)addr) & 0x3FE00000) >> 21;
	size_t pg = pgd[pgd_offset];
	
	pgd[pgd_offset] = (pg & (~(writable << 1)) | (writable << 1));
}

void pml4_set_accessed(size_t *pml4, void *addr, bool accessed)
{

}

void pml4_set_dirty(size_t *pml4, void *addr, bool dirty)
{

}

bool pml4_is_writable(size_t *pml4, void *addr)
{

}

bool pml4_is_accessed(size_t *pml4, void *addr)
{

}

bool pml4_is_dirty(size_t *pml4, void *addr)
{

}


static void _pml_copy(size_t *dest_pml, const size_t *src_pml, bool cow, int level)
{
	size_t i;
	int writable = (!cow << 1);
	if (level > 2) {
		for (i = 0; i < 512; ++i) {
			size_t e = src_pml[i];
			if (e & ~0xFFF) {
				size_t *pml = calloc(512, sizeof *pml);
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

