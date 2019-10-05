
#include <stdbool.h>
#include <string.h>
#include <interrupt.h>

struct idt_reference {
	unsigned short limit;
	unsigned long  offset;
} __attribute__((packed));

struct idt_descriptor {
	unsigned short offsetl;
	unsigned short selector;
	unsigned char  ist;
	unsigned char  typeattr;
	unsigned short offsetm;
	unsigned       offseth;
	unsigned       reserved;
} __attribute__((packed));

struct idt_reference _pidt;
struct idt_descriptor _idt[INTR_COUNT];

void _idt_descriptor(int which, unsigned long offset, unsigned short selector, int flags)
{
	/* This is a readable/naive solution. Writing
	   two ulongs or one __uint128_t is more optimal. */
	struct idt_descriptor *pidte = (_idt + which);

	pidte->offsetl = (offset & 0xFFFF);
	pidte->selector = (selector);
	pidte->ist = 0;
	pidte->typeattr = flags;
	pidte->offsetm = ((offset >> 16) & 0xFFFF);
	pidte->offseth = (offset >> 32);
	pidte->reserved = 0;
}

void _init_idt(void) 
{
	_pidt.limit = (INTR_COUNT) * sizeof(struct idt_descriptor) - 1;
	_pidt.offset = (unsigned long)_idt;

	memset(_idt, 0, sizeof(struct idt_descriptor) * INTR_COUNT);

}

void _lidt(void)
{
	__asm__ volatile (
		"lidt %0\n\t"
		:
		: "X" (_pidt)
	);
}