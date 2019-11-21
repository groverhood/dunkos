#ifndef DUNKOS_MEMORY_H
#define DUNKOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define PAGESIZE (0x200000)
#define ADDRBITS (48UL)
#define ADDRMASK (~(~(1UL << (ADDRBITS - 1)) + 1))
#define PHYSSIZE (0x100000000)

#define PHYSBASE ((void *)0xFFFF800000000000)

static inline void *phys_to_kernel(void *frame)
{
	return (void *)((uintptr_t)frame + (uintptr_t)PHYSBASE);
}

static inline void *kernel_to_phys(void *kernel)
{
	return (void *)((uintptr_t)kernel - (uintptr_t)PHYSBASE);
}

static inline void *page_round_down(void *addr)
{
	return (void *)((uintptr_t)addr & (~PAGESIZE + 1));
}

static inline void *page_round_up(void *addr)
{
	return (void *)(((uintptr_t)addr & (~PAGESIZE + 1)) + PAGESIZE);
}

#endif