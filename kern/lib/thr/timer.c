
#include <util/list.h>
#include <timer.h>
#include <thread.h>
#include <asm.h>
#include <interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include <algo.h>
#include <util/debug.h>

#define TIME_SLICE 100

static uint64_t ticks;
static uint64_t thread_ticks;

static struct lock sleep_queue_lock;
static struct list sleep_queue;

static interrupt_handler timer_interrupt;

extern bool _enable_apic(void);
extern void _enable_apic_timer(int64_t quantum);

void init_timer(void)
{
	ticks = 0;
	list_init(&sleep_queue);
	lock_init(&sleep_queue_lock);

	if (_enable_apic()) {
		install_interrupt_handler(INTR_TYPE_TIMER, &timer_interrupt);
		puts("Enabled APIC...");
		_enable_apic_timer(TIME_SLICE);
	}
}

static void sleep_ticks(int64_t t)
{
	struct thread *cur = get_current_thread();
	cur->sleep_end = ticks + t;

	lock_acquire(&sleep_queue_lock);
	list_push_back(&sleep_queue, &cur->sleep_elem);
	lock_release(&sleep_queue_lock);
	semaphore_dec(&cur->sleep_sema);
}

void sleep_timespec(const struct timespec *duration)
{
	int64_t nanos;
	if (safemul(duration->seconds, NANO_PER_SEC, &nanos)
		&& safeadd(nanos, duration->nanoseconds, &nanos)) {
		sleep_ticks(div_rnd_up(nanos, TIME_SLICE));
	}
}

static enum interrupt_defer timer_interrupt(struct interrupt *intr, 
							void *intrframe_, 
							struct register_state *registers)
{
	puts("TIME'S UP!!!");
	halt();
	enum interrupt_level old_level = disable_interrupts();
	enum interrupt_defer action = INTRDEFR_NONE;
	struct benign_interrupt_frame *frame = intrframe_;

	ticks++;
	if (++thread_ticks > TIME_SLICE) {
		action = INTRDEFR_YIELD;
		thread_ticks = 0;
	}
	
	struct list_elem *sleeper_el;
	for (sleeper_el = list_begin(&sleep_queue); 
		 sleeper_el != list_end(&sleep_queue); 
		 sleeper_el = list_next(sleeper_el)) {

		struct thread *sleeper = elem_value(sleeper_el, 
									struct thread, sleep_elem);

		if (sleeper->sleep_end <= ticks)
			semaphore_inc(&sleeper->sleep_sema);
	}

	return action;
}