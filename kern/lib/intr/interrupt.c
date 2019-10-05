
#include <stdio.h>
#include <interrupt.h>

extern void _init_tss(void);
extern void _ltr(void);

extern void _init_idt(void);
extern void _lidt(void);

extern void _idt_descriptor(int which, unsigned long offset, unsigned short selector, int flags);

/* Tertiary structure which contains "user"-friendly information
   about the interrupt. */
struct interrupt {
	const char *name;
	interrupt_handler *handler;
};

static struct interrupt intr_table[INTR_COUNT];
static enum interrupt_level intr_level;

static void intr_default(struct interrupt_frame *intrframe);
static void isr_common_stub(void);

void init_interrupts(void)
{
	//_init_tss();
	//_ltr();

	_init_idt();
	_lidt();

	intr_level = INTR_ENABLED;

	int i;
	for (i = 0; i < INTR_COUNT; ++i) {
		intr_table[i].name = "Unknown interrupt";
		intr_table[i].handler = &intr_default;
		_idt_descriptor(i, (unsigned long)isr_common_stub, 0x08, 0x8E);
	}

	intr_table[INTR_TYPE_DIV0].name = "Division by zero";
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
	case INTR_CONTEXT: 
	case INTR_DISABLED: __asm__ volatile ("cli"); break;
	case INTR_ENABLED: __asm__ volatile ("sti"); break;
	}
	intr_level = level;
	return old_level;
}

static void intr_default(struct interrupt_frame *intrframe)
{
	puts("Interrupt!");
}

static void isr_common_stub(void)
{
	enum interrupt_level old_level;
	struct interrupt_frame *intrframe;

	old_level = set_interrupt_level(INTR_CONTEXT);

	__asm__ volatile (
		"pushq %%r11\n\t"
		"pushq %%r10\n\t"
		"pushq %%r9\n\t"
		"pushq %%r8\n\t"
		"pushq %%rdi\n\t"
		"pushq %%rsi\n\t"
		"pushq %%rdx\n\t"
		"pushq %%rcx\n\t"
		"pushq %%rax\n\t"
		"mov %%rsp, %[rsp]\n\t"
		"cld\n\t"
		: [rsp] "=r" (intrframe)
	);

	size_t intr = (intrframe->error_code >> 3);
	intr_table[intr].handler(intrframe);

	__asm__ volatile (
		"addq %0, %%rsp\n\t"
		"iretq"
		:
		: "i" (sizeof(struct interrupt_frame))
	);

	set_interrupt_level(old_level);
}

void install_interrupt_handler(enum interrupt_type which, interrupt_handler *handler)
{
}

bool in_interrupt(void)
{
	return (intr_level == INTR_CONTEXT);
}