
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <algo.h>

#define ENABLE_X2APIC (1 << 11)

void _enable_apic_timer(void)
{
	uint32_t result;
	__asm__ volatile (
		"movl $0xFEE003E0, %%ecx\n\t"
		"rdmsr"
		: "=a" (result)
	);

	printf("%x\n", result);
	halt();
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
		__asm__ volatile (
			"movl $0x01B, %%ecx\n\t"
			"rdmsr\n\t"
			"orl %0, %%eax\n\t"
			"wrmsr"
		:
		:	"i" (ENABLE_X2APIC)
		);
	}

	return enabled_apic;
}