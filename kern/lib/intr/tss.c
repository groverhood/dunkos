
#include <stdio.h>
#include <string.h>

struct task_state_segment {
	unsigned reserved1;

	unsigned rsp0l;
	unsigned rsp0h;
	unsigned rsp1l;
	unsigned rsp1h;
	unsigned rsp2l;
	unsigned rsp2h;

	unsigned reserved2;
	unsigned reserved3;

	unsigned ist1l;
	unsigned ist1h;
	unsigned ist2l;
	unsigned ist2h;
	unsigned ist3l;
	unsigned ist3h;
	unsigned ist4l;
	unsigned ist4h;
	unsigned ist5l;
	unsigned ist5h;
	unsigned ist6l;
	unsigned ist6h;
	unsigned ist7l;
	unsigned ist7h;

	unsigned reserved4;
	unsigned reserved5;

	unsigned short reserved6;
	unsigned short iomapbase;
} __attribute__((packed));

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
