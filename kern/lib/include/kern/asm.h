#ifndef DUNKOS_KERN_ASM_H
#define DUNKOS_KERN_ASM_H

#include <stdint.h>
#include <kern/msr.h>

/* Prevent optimizations that may break serializability. */
#define barrier() __asm__ volatile ("mfence" : : : "memory")

#define unreachable __builtin_unreachable

static inline void outb(uint8_t value, uint16_t port)
{
    barrier();
    __asm__ volatile (
        "outb %[value], %[port]"
        :: [value] "r" (value), [port] "r" (port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    barrier();
    uint8_t value;
    __asm__ (
        "inb %[port], %[value]"
        : [value] "=r" (value)
        : [port] "r" (port)
    );
    return value;
}

__attribute__((noreturn)) static inline void jump(uint64_t where)
{
    barrier();
    __asm__ volatile ("jmp %0" :: "r" (where) : "memory");
    unreachable();
}

#endif