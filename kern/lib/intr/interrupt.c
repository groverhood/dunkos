
#include <stdio.h>
#include <interrupt.h>

extern void _init_tss(void);
extern void _ltr(void);

extern void _init_idt(void);
extern void _lidt(void);

typedef interrupt_handler intr_stub;

extern void _idt_descriptor(int which, unsigned long offset, unsigned short selector, int flags);
extern bool _enable_apic(void);

/* Tertiary structure which contains "user"-friendly information
   about the interrupt. */
struct interrupt {
	const char *name;
	enum interrupt_type id;

	intr_stub *stub;
};

static struct interrupt intr_table[INTR_COUNT];
static enum interrupt_level intr_level;

static void intr_default(struct interrupt_frame *intrframe);
static void isr_common_stub(struct interrupt_frame *intrframe);

void init_interrupts(void)
{
	_init_idt();
	_lidt();

	puts("Initialized Task State Segment...");

	_init_tss();
	_ltr();

	puts("Initialized Interrupt Descriptor Table...");

	if (_enable_apic()) {
		puts("Enabled APIC...");
	}

	int i;
	for (i = 0; i < INTR_COUNT; ++i) {
		intr_table[i].id = i;
		intr_table[i].name = "Unknown interrupt";
		install_interrupt_handler(i, &isr_common_stub);
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

extern struct task_state_segment _tss;

static intr_void isr_common_stub(struct interrupt_frame *intrframe)
{
	enum interrupt_level old_level;
	struct register_state *registers;

	old_level = set_interrupt_level(INTR_CONTEXT);

	__asm__ ("movq %%rsp, %0"
		     : "=r" (registers));

	set_interrupt_level(old_level);
}

void install_interrupt_handler(enum interrupt_type which, interrupt_handler *handler)
{
	_idt_descriptor(which, (unsigned long)handler, 0x08, 0x8E);
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