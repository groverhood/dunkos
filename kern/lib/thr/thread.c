
#include <kern/paging.h>
#include <kern/thread.h>
#include <kern/synch.h>
#include <kern/heap.h>
#include <kern/timer.h>
#include <kern/process.h>
#include <stdio.h>
#include <algo.h>
#include <kern/asm.h>

static struct list ready_threads;
static struct thread *running_thread;

/* Doubles as the idle thread after startup. */
static struct thread *initial_thread;
static tid_t next_tid;

static struct list free_threads;

bool threads_init;

static list_comparator thread_priority_highest;
static tid_t assign_id(void);
static thread_function idle_thread_func;

void init_threads(void)
{
	list_init(&ready_threads);
	struct process *initial_process;

	next_tid = 0;

	/* The initial thread's stack has already been supplied
	   in the reserved region of physical memory. */
	initial_process = kcalloc(1, sizeof *initial_process);
	process_init(initial_process);

	initial_thread = process_get_base(initial_process);

	threads_init = true;
	running_thread = initial_thread;
}

static void start_thread(void)
{
	set_interrupt_level(INTR_ENABLED);
	struct thread *cur = get_current_thread();
	cur->context.entry(cur->context.aux);
	exit_thread();
}

void create_thread(struct thread **dest, thread_function *fn, void *aux)
{
	struct thread *newthr;
	newthr = kcalloc(1, sizeof *newthr + KTHREAD_STACK_SIZE);
	*dest = newthr;
	
	thread_init(newthr);

	newthr->magic = KTHREAD_MAGIK;

	struct thread_context *c = &newthr->context;
	c->sp = thread_stack_top(newthr);
	c->entry = fn;
	c->aux = aux;
	c->ip = &start_thread;

	thread_unblock(newthr);
}

void thread_init(struct thread *thr)
{
	list_init(&thr->donors);

	thr->priority = PRI_DEFAULT;
	thr->sleep_end = -1;
	thr->id = assign_id();

	semaphore_init(&thr->sleep_sema, 0);
}

struct thread *get_current_thread(void)
{
	return running_thread;
}

tid_t get_current_id(void)
{
	return get_current_thread()->id;
}

int get_current_priority(void)
{
	return thread_get_priority(get_current_thread());
}

void set_current_priority(int new_priority)
{
	struct thread *thr = get_current_thread();

	thr->priority = new_priority;
}

enum thread_status get_current_status(void)
{
	return get_current_thread()->status;
}

static struct thread *next_thread(void)
{
	if (list_empty(&ready_threads)) {
		return initial_thread;
	}	

	return elem_value(list_pop_front(&ready_threads), struct thread, status_elem);
}

void yield_current(void)
{
	struct thread *current_thread = get_current_thread();
	if (current_thread->status != THRSTAT_BLOCKED) {
		list_push_back(&ready_threads, &current_thread->status_elem);
	}

	schedule_next();
}

void block_current(void)
{
	get_current_thread()->status = THRSTAT_BLOCKED;
	yield_current();
}

int thread_get_priority(struct thread *restrict thr)
{
	return !list_empty(&thr->donors) ? 
			  thread_get_priority(
				elem_value(list_front(&thr->donors), struct thread, donor_elem)
			  )
			: thr->priority;
}

void thread_block(struct thread *thr)
{	
	if (thr == get_current_thread()) {
		block_current();
	} else {
		thr->status = THRSTAT_BLOCKED;
		list_remove(&ready_threads, &thr->status_elem);
	}
}

void thread_unblock(struct thread *thr)
{
	enum interrupt_level old_level = disable_interrupts();

	list_push_back(&ready_threads, &thr->status_elem);
	thr->status = THRSTAT_READY;

	set_interrupt_level(old_level);
}

__attribute__((noreturn)) void switch_to(struct thread *prev, struct thread *next)
{
	if (prev->status == THRSTAT_DEAD)
		kfree(prev);

	void *ip = next->context.ip;
	__asm__ volatile (
		"movq %0, %%rsp\n"
		"jmp *%1" 
		:: "m" (next->context.sp), "r" (ip)
	);

	set_interrupt_level(INTR_ENABLED);
}

__attribute__((noreturn)) void schedule_next(void) 
{
	struct thread *next = next_thread();
	struct thread *prev = running_thread;
	running_thread = next;
	switch_to(prev, next);
	unreachable();
}

__attribute__((noreturn)) void exit_thread(void)
{
	disable_interrupts();
	get_current_thread()->status = THRSTAT_DEAD;
	schedule_next();
}

static bool thread_priority_highest(struct list_elem *le, struct list_elem *ri)
{
	return thread_compare(ri, le, status_elem);
}

static tid_t assign_id(void)
{
	return next_tid++;
}

/* SYNCHRONIZATION IMPL */

void semaphore_init(struct semaphore *sema, unsigned long value)
{
	sema->value = value;
	list_init(&sema->waiters);
}

void semaphore_inc(struct semaphore *sema)
{
	struct list *waiters;

	enum interrupt_level old_level = disable_interrupts();
	waiters = &sema->waiters;
	sema->value++;

	if (!list_empty(waiters)) {
		struct thread *top_waiter = elem_value(list_pop_front(waiters), 
												struct thread, status_elem);
		thread_unblock(top_waiter);
	}

	set_interrupt_level(old_level);
}

void semaphore_dec(struct semaphore *sema)
{
	enum interrupt_level old_level;
	unsigned long value;

	/* Ensure atomicity. */
	old_level = disable_interrupts();
	value = sema->value;

	while (value == 0) {
		list_push_back(&sema->waiters, &get_current_thread()->status_elem);
		block_current();
	}
	
	sema->value = value - 1;
	set_interrupt_level(old_level);
}

void lock_init(struct lock *l)
{
	semaphore_init(&l->binary, 1);
	l->holder = NULL;
}

void lock_acquire(struct lock *l)
{
	semaphore_dec(&l->binary);
	l->holder = get_current_thread();
}

void lock_release(struct lock *l)
{
	l->holder = NULL;
	semaphore_inc(&l->binary);
}

void condvar_init(struct condvar *c)
{
	list_init(&c->waiters);
}

void condvar_wait(struct condvar *c, struct lock *l)
{
	lock_release(l);
	list_push_back(&c->waiters, &get_current_thread()->status_elem);
	block_current();
	lock_acquire(l);
}

void condvar_signal_one(struct condvar *c, struct lock *l)
{
	lock_release(l);
	list_push_back(&ready_threads, list_pop_front(&c->waiters));
	lock_acquire(l);
}

void condvar_signal_all(struct condvar *c, struct lock *l)
{
	while (!list_empty(&c->waiters))
		condvar_signal_one(c, l);
}
