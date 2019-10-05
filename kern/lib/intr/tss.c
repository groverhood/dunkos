
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

struct tss_reference {
	unsigned short limit;
	unsigned long offset;
} __attribute__((packed));

struct task_state_segment _tss;
struct tss_descriptor _dtss;
struct tss_reference _tssr;

void _init_tss(void)
{
	memset(&_tss, 0, sizeof(struct task_state_segment));

	unsigned long limit = sizeof(struct task_state_segment) - 1;
	unsigned long base = (unsigned long)&_tss;

	_dtss.limitl = (limit & 0xFFFF);
	_dtss.basel = (base & 0xFFFF);
	_dtss.basem1 = (base & 0xFF0000) >> 16;
	_dtss.typeattr = 0x89;
	_dtss.limith = ((limit & 0xF0000) >> 16) | 0x90;
	_dtss.basem2 = (base & 0xFF000000) >> 24;
	_dtss.baseh = (base >> 32);
	_dtss.reserved = 0;

	_tssr.limit = sizeof(struct tss_descriptor) - 1;
	_tssr.offset = (unsigned long)&_dtss;
}

void _ltr(void)
{
	__asm__ volatile (
		"ltr %0\n\t"
		:
		: "X" (_tssr)
	);
}
