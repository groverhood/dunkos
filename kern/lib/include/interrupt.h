#ifndef DUNKOS_INTERRUPT_H
#define DUNKOS_INTERRUPT_H

#include <stdbool.h>

#define INTR_COUNT (0x100)

void init_interrupts(void);

enum interrupt_level {
	INTR_DISABLED,
	INTR_ENABLED,
	INTR_CONTEXT
};

enum interrupt_type {
	INTR_TYPE_DIV0
};

struct interrupt_frame {
	unsigned long rax;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long r8, r9, r10, r11;

	unsigned long error_code;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

typedef void interrupt_handler(struct interrupt_frame *);

enum interrupt_level disable_interrupts(void);

enum interrupt_level get_interrupt_level(void);
enum interrupt_level set_interrupt_level(enum interrupt_level level);

void install_interrupt_handler(enum interrupt_type which, interrupt_handler *handler);

bool in_interrupt(void);

#endif