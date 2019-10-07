
#include <stdio.h>
#include <string.h>
#include <interrupt.h>

struct tss_descriptor {
	unsigned short limitl;
	unsigned short basel;
	unsigned char  basem1;
	unsigned char  typeattr;
	unsigned char  limith;
	unsigned char  basem2;
	unsigned       baseh;
	unsigned       reserved;
} __attribute__((packed));

struct task_state_segment _tss;
extern struct tss_descriptor gdt64tss;

void _init_tss(void)
{

	unsigned long limit = sizeof(struct task_state_segment) - 1;
	unsigned long base = (unsigned long)&_tss;

	gdt64tss.limitl = (limit & 0xFFFF);
	gdt64tss.basel = (base & 0xFFFF);
	gdt64tss.basem1 = (base & 0xFF0000) >> 16;
	gdt64tss.typeattr = 0x89;
	gdt64tss.limith = ((limit & 0xF0000) >> 16);
	gdt64tss.basem2 = (base & 0xFF000000) >> 24;
	gdt64tss.baseh = (base >> 32);
}

void _ltr(void)
{
	__asm__ (
		"ltr %0\n\t"
		:
		: "X" (_tss)
	);
}
