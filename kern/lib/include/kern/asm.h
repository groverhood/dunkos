#ifndef DUNKOS_KERN_ASM_H
#define DUNKOS_KERN_ASM_H

#include <stdint.h>
#include <msr.h>
#include <cpuid.h>

#define atomic _Atomic
#define _Unused __attribute__((unused))

/* Prevent optimizations that may break serializability. */
#define barrier() __asm__ volatile ("mfence" : : : "memory")

#define unreachable __builtin_unreachable

#define out(value, port) do {\
        barrier();\
        __asm__ __volatile__ ("out %0, %%dx" :: "a" (value), "d" (port));\
    } while (0)

#define in(port, pvalue) do {\
        barrier();\
        __asm__ __volatile__ ("in %%dx, %0" : "=a" (*pvalue) : "d" (port));\
    } while (0)

__attribute__((noreturn)) static inline void jump(uint64_t where)
{
    barrier();
    __asm__ volatile ("jmp %0" :: "r" (where) : "memory");
    unreachable();
}

static inline void insb(uint16_t port, void *addr, size_t count)
{
    barrier();
    __asm__ volatile (
        "insb" 
        : "=D" (addr), "=c" (count)
        : "d" (port), "0" (addr), "1" (count)
        : "memory"
    );
}

static inline void outsb(void *addr, uint16_t port, size_t count)
{
    barrier();
    __asm__ volatile (
        "outsb" 
        : "=S" (addr), "=c" (count) 
        : "d" (port), "0" (addr), "1" (count)
        : "memory"
    );
}

#define cpuid __cpuid

#define PACKED __attribute__((packed))

#endif