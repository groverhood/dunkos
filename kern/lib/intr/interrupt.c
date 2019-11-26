
#include <stdio.h>
#include <interrupt.h>
#include <kern/thread.h>
#include <algo.h>

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

typedef uint64_t interrupt_code_t;

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

__attribute__((optimize (0))) enum interrupt_level set_interrupt_level(enum interrupt_level level)
{
	enum interrupt_level old_level = intr_level;
	switch (level) {
	case INTR_DISABLED: __asm__ volatile ("cli"); break;
	case INTR_ENABLED: __asm__ volatile ("sti"); break;
	}
	intr_level = level;
	return old_level;
}

static enum interrupt_defer intr_default(struct interrupt *intr, 
						 void *intrframe_,
						 struct register_state *registers)
{
	struct benign_interrupt_frame *frame = intrframe_;
	printf("Unhandled interrupt: %#x\n", intr->id);
	printf("Interrupt occurred at %p\n", frame->rip);
	halt();
}

void isr_common_stub(interrupt_code_t intr, void *intrframe_,
					 struct register_state *registers)
{
	enum interrupt_level old_level;
	enum interrupt_defer action;
	old_level = set_interrupt_level(INTR_CONTEXT);
	action = intr_table[intr].handler(intr_table + intr, intrframe_, registers);
	set_interrupt_level(old_level);
	if (action == INTRDEFR_YIELD)
		yield_current();
}

void install_interrupt_handler(enum interrupt_type which, interrupt_handler *handler)
{
	intr_table[which].handler = handler;
}

bool in_interrupt(void)
{
	return (intr_level == INTR_CONTEXT);
}