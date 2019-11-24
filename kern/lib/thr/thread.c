
#include <kern/paging.h>
#include <kern/thread.h>
#include <kern/synch.h>
#include <kern/heap.h>
#include <kern/timer.h>
#include <algo.h>

static struct list ready_threads;
static struct thread *running_thread;
static struct thread *idle_thread;
static struct thread *initial_thread;
static tid_t next_tid;

static struct list free_threads;

bool threads_init;

static list_comparator thread_priority_highest;
static void init_thread(struct thread *thr);
static tid_t assign_id(void);
static thread_function idle_thread_func;

void init_threads(void)
{
	list_init(&ready_threads);

	next_tid = 0;

	/* Create the idle thread. */
	create_thread(&idle_thread, idle_thread_func, NULL);

	/* The initial thread's stack has already been supplied
	   in the reserved region of physical memory. */
	initial_thread = calloc(1, sizeof *initial_thread);
	init_thread(initial_thread);

	struct thread_context *ctx = &initial_thread->context;
	__asm__ (
		"movq (%%rsp), %%rdi\n\t"
		"movq %%rdi, %0\n\t"
		"movq %%rsp, %1"
		: "=m" (ctx->ip), "=r" (ctx->sp)
	);
	
	threads_init = true;
	running_thread = initial_thread;

	init_timer();
}

#define KTHREAD_STACK_SIZE (0x1000)

static void start_thread(struct thread *thr)
{
	set_interrupt_level(INTR_ENABLED);
	thr->context.entry(thr->context.aux);
	exit_thread();
}

void create_thread(struct thread **dest, thread_function *fn, void *aux)
{
	struct thread *newthr;
	newthr = malloc(sizeof *newthr + KTHREAD_STACK_SIZE);
	*dest = newthr;
	
	init_thread(newthr);

	newthr->magic = KTHREAD_MAGIK;

	struct thread_context *c = &newthr->context;
	c->sp = (size_t *)((uint8_t *)(newthr) + sizeof *newthr + KTHREAD_STACK_SIZE);
	c->entry = fn;
	c->aux = aux;
	c->ip = &start_thread;

	enum interrupt_level old_level = disable_interrupts();
	thread_unblock(newthr);
	set_interrupt_level(old_level);
}

void init_thread(struct thread *thr)
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
		return idle_thread;
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

#define SAVE_CONTEXT "pushfq\n\tpushq %%rbp\n\tmovq %%rsi, %%rbp\n\t"
#define RESTORE_CONTEXT "movq %%rbp, %%rsi\n\tpopq %%rbp\n\tpopfq\n\t" 
#define EXTRA_CLOBBER\
    "rcx", "rbx", "rdx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
#define SWITCH_TO_CFUNC __switch_to

struct thread *__switch_to(struct thread *prev, struct thread *next)
{
	
	return next;
}

#define switch_to(prev_, next_, last_)\
	asm volatile (\
		SAVE_CONTEXT\
		"movq %%rsp, %c[thread_rsp](%[prev])\n\t" /* Save %rsp. */\
		"movq %c[thread_rsp](%[next]), %%rsp\n\t" /* Restore %rsp. */\
		"call " name(SWITCH_TO_CFUNC) "\n\t"\
		"jmp *%c[thread_ip](%[next])\n\t"\
		RESTORE_CONTEXT\
		: "=a" (last_)\
		: [next] "S" (next_), [prev] "D" (prev_),\
		  [thread_rsp] "i" (offsetof(struct thread, context.sp)),\
		  [thread_ip] "i" (offsetof(struct thread, context.ip))\
		: "memory", "cc", EXTRA_CLOBBER\
	)

__attribute__((noreturn)) void schedule_next(void) 
{
	register struct thread *next = next_thread();
	register struct thread_context *c = &next->context;

	switch_to(running_thread, next, running_thread);
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

static void idle_thread_func(void *aux)
{
	puts("Idling...");
	while (1) {}
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
