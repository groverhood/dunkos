#ifndef DUNKOS_KERN_MSR_H
#define DUNKOS_KERN_MSR_H

#include <stdint.h>
#include <stdio.h>

/* Write [%edx:%eax] to the provided msr. */
static inline void wrmsr(uint32_t where, uint64_t val)
{
    __asm__ volatile (
        "movq %%rsi, %%rax\n\t"
        "movq %%rsi, %%rdx\n\t"
        "shrq $32, %%rdx\n\t"
        "wrmsr"
        :
        : "c" (where), "S" (val)
    );
}

/* Read [%edx:%eax] from the provided msr. */
static inline uint64_t rdmsr(uint32_t where)
{
    uint32_t eax, edx;
    __asm__ volatile (
        "rdmsr\n\t"
        : "=a" (eax), "=d" (edx)
        : "c" (where)
    );
    return (((uint64_t)edx) << 32UL) | (uint64_t)eax;
}

#endif