#ifndef DUNKOS_KERN_MSR_H
#define DUNKOS_KERN_MSR_H

#include <stdint.h>

/* Write [%edx:%eax] to the provided msr. */
static inline void wrmsr(uint32_t where, uint64_t val)
{
    uint64_t upper = val >> 32;
    uint64_t lower = val & 0xFFFFFFFF;
    __asm__ volatile (
        "movl %[where], %%ecx\n\t"
        "movl %[edx], %%edx\n\t"
        "movl %[eax], %%eax\n\t"
        "wrmsr"
        :: [where] "r" (where), [edx] "r" ((uint32_t)upper), 
            [eax] "r" ((uint32_t)lower)
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