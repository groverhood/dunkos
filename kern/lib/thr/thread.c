
#include <kern/paging.h>
#include <kern/thread.h>
#include <kern/synch.h>

static struct list ready_threads;
static struct thread *running_thread;
static struct thread *idle_thread;
static tid_t next_tid;

static struct list free_threads;

static list_comparator thread_priority_highest;
static tid_t assign_id(void);
static struct thread_entry *find_free_thread(void);
static struct thread *thread_alloc(void);

extern void *kernel_page_top;

static void init_threadpool(void)
{
	// struct thread *kernel_threadp_top;
	// size_t i;

	// kernel_threadp_top = kernel_page_top;
	// i = PML4_COUNT;

	// while (i-- > 0) {
	// 	struct thread_entry *te = (threadpool + i);
	// 	te->thr = (--kernel_threadp_top);
	// 	te->marked_free = true;
	// }
}

void init_threads(void)
{
	// struct thread *main_thread;
	// next_tid = 0;

	// init_threadpool();

	// main_thread = thread_alloc();
	// init_thread(main_thread);

	// running_thread = main_thread;
}

static struct thread *thread_alloc(void)
{	
}

void create_thread(struct thread **dest, thread_function *fn, void *aux)
{
	struct thread *newthr;

	newthr = thread_alloc();
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
	return get_thread_priority(get_current_thread());
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

int get_thread_priority(struct thread *restrict thr)
{
	return !list_empty(&thr->donors) ? 
			  get_thread_priority(
				elem_value(list_front(&thr->donors), struct thread, donor_elem)
			  )
			: thr->priority;
}

void block_thread(struct thread *thr)
{	
	if (thr == get_current_thread()) {
		block_current();
	} else {
		thr->status = THRSTAT_BLOCKED;
		list_remove(&ready_threads, &thr->status_elem);
	}
}

void unblock_thread(struct thread *thr)
{
	list_push_back(&ready_threads, &thr->status_elem);
	thr->status = THRSTAT_READY;
}

__attribute__((noreturn)) void schedule_next(void) 
{
	struct thread *next = next_thread();
	running_thread = next;

	__asm__ volatile (
		"jmpq %0"
		:: "m" (next->ip)
	);
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

	waiters = &sema->waiters;
	sema->value++;

	if (!list_empty(waiters)) {
		struct thread *top_waiter = elem_value(list_pop_front(waiters), 
												struct thread, status_elem);
		
		unblock_thread(top_waiter);
	}
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
