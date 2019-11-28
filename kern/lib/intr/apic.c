
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <algo.h>
#include <string.h>
#include <kern/asm.h>
#include <kern/paging.h>
#include <kern/memory.h>
#include <kern/pml4.h>

#define APIC_BASE ((void *)0xFEE00000)
#define APIC_ENABLE (1 << 11)
#define APIC_BSP_PROCESSOR (1 << 8)
#define APIC_SPINT_OFS (0x0F0)
#define APIC_TIMER_OFS (0x320)
#define APIC_COUNT_OFS (0x380)
#define APIC_DVCFG_OFS (0x3E0)
#define APIC_TIMER_ONE (0 << 17)
#define APIC_TIMER_PRD (1 << 17)
#define APIC_SOFT_ENABLE (1 << 8)

static uint32_t *apic_base;

static inline void set_apic(int what, uint32_t val)
{
	size_t ofs = what / sizeof *apic_base;
	apic_base[ofs] |= val;
}

void _enable_apic_timer(void)
{
	/* Set to periodic mode. This will automatically unmask the
	   apic timer (the mask we send through does not have bit 16 
	   set). */
	set_apic(APIC_SPINT_OFS, APIC_SOFT_ENABLE | INTR_TYPE_SPURIOUS);
	set_apic(APIC_TIMER_OFS, APIC_TIMER_PRD | INTR_TYPE_TIMER);

	/* 200 ticks. */
	set_apic(APIC_COUNT_OFS, 200);
	/* Divide by 1. */
	set_apic(APIC_DVCFG_OFS, 0x0B);

	puts("Enabled APIC timer...");
}

bool _enable_apic(void)
{
	int cpuid_retval;
	bool enabled_apic;

	__asm__ volatile (
		"movl $1, %%eax\n\t"
		"cpuid\n\t"
		"movl %%edx, %0"
	:	"=r" (cpuid_retval)
	);
	
	enabled_apic = !!(cpuid_retval & (1 << 9));

	if (enabled_apic) {
		apic_base = page_allocate(0);
		memcpy(apic_base, phys_to_kernel(APIC_BASE), 0x1000);
		wrmsr(0x01B, (uintptr_t)kernel_to_phys(apic_base) | APIC_ENABLE | APIC_BSP_PROCESSOR);
	}
	
	printf("APIC base: %p\n", apic_base);

	return enabled_apic;
}