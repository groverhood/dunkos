
#include <stdio.h>
#include <interrupt.h>

extern void _init_tss(void);
extern void _ltr(void);

extern void _init_idt(void);
extern void _lidt(void);

extern void _install_stubs(void);
extern bool _enable_apic(void);

static struct interrupt intr_table[INTR_COUNT];
static enum interrupt_level intr_level;
static int intr_index;
static bool isr_init;
static interrupt_handler intr_default;

extern void isr_stub0(void *);

void init_interrupts(void)
{
	isr_init = true;
	intr_index = 0;

	_init_idt();
	_lidt();

	puts("Initialized Task State Segment...");

	_init_tss();
	_ltr();

	puts("Initialized Interrupt Descriptor Table...");

	if (_enable_apic()) {
		puts("Enabled APIC...");
	}

	_install_stubs();

	int i;
	for (i = 0; i < INTR_COUNT; ++i) {
		intr_table[i].id = i;
		intr_table[i].name = "Unknown interrupt";
		intr_table[i].handler = &intr_default;
	}

	intr_table[INTR_TYPE_DIV0].name = "Division by zero";
	intr_level = INTR_ENABLED;
}

enum interrupt_level disable_interrupts(void)
{
	return set_interrupt_level(INTR_DISABLED);
}

enum interrupt_level get_interrupt_level(void)
{
	return intr_level;
}

enum interrupt_level set_interrupt_level(enum interrupt_level level)
{
	enum interrupt_level old_level = intr_level;
	switch (level) {
	case INTR_DISABLED: __asm__ volatile ("cli"); break;
	case INTR_ENABLED: __asm__ volatile ("sti"); break;
	}
	intr_level = level;
	return old_level;
}

static void intr_default(struct interrupt *intr, 
						 struct interrupt_frame *intrframe,
						 struct register_state *registers)
{
	printf("Unhandled interrupt: 0x%x\n", intr->id);
}

void isr_common_stub(unsigned long intr, struct interrupt_frame *intrframe,
					 struct register_state *registers)
{
	enum interrupt_level old_level;
	old_level = set_interrupt_level(INTR_CONTEXT);
	intr_table[intr].handler(intr_table + intr, intrframe, registers);
	set_interrupt_level(old_level);
}

void install_interrupt_handler(enum interrupt_type which, interrupt_handler *handler)
{
	intr_table[which].handler = handler;
}

void dump_intrfame(struct interrupt_frame *intrframe, 
				   struct register_state *registers)
{ 
	printf("%%rip:\t%lx\n", intrframe->rip);
	printf("%%cs:\t%lx\n", intrframe->cs);
	printf("rflags:\t%lx\n", intrframe->rflags);
	printf("%%rsp:\t%lx\n", intrframe->rsp);
	printf("%%ss:\t%lx\n", intrframe->ss);
}

bool in_interrupt(void)
{
	return (intr_level == INTR_CONTEXT);
}