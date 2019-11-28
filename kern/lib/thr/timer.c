
#include <util/list.h>
#include <kern/thread.h>
#include <interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include <algo.h>

#define TIME_SLICE 20

static uint64_t ticks;
static uint64_t thread_ticks;

static struct list sleep_queue;

static interrupt_handler timer_interrupt;

extern bool _enable_apic(void);
extern void _enable_apic_timer(void);

void init_timer(void)
{
	if (_enable_apic()) {
		ticks = 0;
		install_interrupt_handler(INTR_TYPE_TIMER, &timer_interrupt);
		puts("Enabled APIC...");
		_enable_apic_timer();
	}
}

static enum interrupt_defer timer_interrupt(struct interrupt *intr, 
							void *intrframe_, 
							struct register_state *registers)
{
	puts("Time out!!!");
	halt();
	halt();
	halt();

	enum interrupt_level old_level = disable_interrupts();
	enum interrupt_defer action = INTRDEFR_NONE;
	struct benign_interrupt_frame *frame = intrframe_;

	ticks++;

	if (++thread_ticks > TIME_SLICE) {
		action = INTRDEFR_YIELD;
		thread_ticks = 0;
		struct thread_context *cc = &get_current_thread()->context;
		cc->ip = (void *)frame->rip;
		cc->sp = (size_t *)frame->rsp;
	}

	struct thread *sleeper;
	while (!list_empty(&sleep_queue) && 
	(sleeper = elem_value(list_front(&sleep_queue), struct thread, sleep_elem))->sleep_end <= ticks) {
		semaphore_inc(&sleeper->sleep_sema);
		list_pop_front(&sleep_queue);
	}

	set_interrupt_level(old_level);
	return action;
}