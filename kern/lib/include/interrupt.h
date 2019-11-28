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
	INTR_TYPE_DIV0,
	INTR_TYPE_DBLFAULT = 8,
	INTR_TYPE_PGFAULT = 14,
	INTR_TYPE_TIMER = 0x20,
	INTR_TYPE_SPURIOUS = 0x27
};

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

struct register_state {
	unsigned long rax;
	unsigned long rdx;
	unsigned long rcx;
	unsigned long rbx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long r8, r9, r10, r11;
};

struct benign_interrupt_frame {
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

struct fault_interrupt_frame {
	unsigned long error;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

/** 
 * 	Action we wish to defer to after restoring
 * 	pre-interrupt state.
 **/
enum interrupt_defer {
	INTRDEFR_NONE,
	INTRDEFR_YIELD
};

struct interrupt;

typedef enum interrupt_defer interrupt_handler(struct interrupt *, void *,
											   struct register_state *);

/* Tertiary structure which contains "user"-friendly information
   about the interrupt. */
struct interrupt {
	const char *name;
	enum interrupt_type id;
	interrupt_handler *handler;
};

enum interrupt_level disable_interrupts(void);

enum interrupt_level get_interrupt_level(void);
enum interrupt_level set_interrupt_level(enum interrupt_level level);

void install_interrupt_handler(enum interrupt_type which, interrupt_handler *handler);

bool in_interrupt(void);

#endif