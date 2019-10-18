
#include <util/list.h>
#include <kern/thread.h>
#include <interrupt.h>
#include <stdio.h>

unsigned long ticks;

struct list sleep_queue;

static interrupt_handler timer_interrupt;

extern void _enable_apic_timer(void);

void init_timer(void)
{
	ticks = 0;
	install_interrupt_handler(INTR_TYPE_TIMER, &timer_interrupt);
	_enable_apic_timer();
}

static enum interrupt_defer timer_interrupt(struct interrupt *intr, 
							void *intrframe_, 
							struct register_state *registers)
{
	return INTRDEFR_SCHEDULE;
}