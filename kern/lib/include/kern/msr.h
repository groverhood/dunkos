#ifndef DUNKOS_KERN_MSR_H
#define DUNKOS_KERN_MSR_H

#include <stdint.h>

/* Write [%edx:%eax] to the provided msr. */
static inline void wrmsr(uint32_t where, uint64_t val)
{
    __asm__ volatile (
        "movq %[val], %%rax\n\t"
        "movq %[val], %%rdx\n\t"
        "shrq $32, %%rdx\n\t"
        "movl %[where], %%ecx\n\t"
        "wrmsr"
        :: [where] "r" (where), [val] "r" (val)
    );
}

/* Read [%edx:%eax] from the provided msr. */
static inline uint64_t rdmsr(uint32_t where)
{
    uint32_t eax, edx;
    __asm__ volatile (
        "movl %[where], %%ecx\n\t"
        "rdmsr\n\t"
        "movl %%edx, %[edx]\n\t"
        "movl %%eax, %[eax]"
        : [eax] "=r" (eax), [edx] "=r" (edx)
        : [where] "r" (where)
    );

    return ((uint64_t)edx << 32) | eax;
}

#endif