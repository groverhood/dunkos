#ifndef DUNKOS_KERN_ASM_H
#define DUNKOS_KERN_ASM_H

#include <stdint.h>
#include <msr.h>

#define atomic _Atomic
#define _Unused __attribute__((unused))

/* Prevent optimizations that may break serializability. */
#define barrier() __asm__ volatile ("mfence" : : : "memory")

#define unreachable __builtin_unreachable

static inline void outb(uint8_t value, uint16_t port)
{
    barrier();
    __asm__ volatile (
        "outb %%al, %%dx"
        :: [value] "a" (value), [port] "d" (port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    barrier();
    uint8_t value;
    __asm__ (
        "inb %%dx, %%al"
        : "=a" (value)
        : "d" (port)
    );
    return value;
}

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

#endif