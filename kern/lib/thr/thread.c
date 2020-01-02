
#include <paging.h>
#include <thread.h>
#include <synch.h>
#include <heap.h>
#include <timer.h>
#include <process.h>
#include <stdio.h>
#include <algo.h>
#include <asm.h>
#include <util/debug.h>

static struct list ready_threads;
static struct thread *running_thread;
static struct thread *idle_thread;
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

    next_tid = 0;

    /* The initial thread's stack has already been supplied
       in the reserved region of physical memory. */
    initial_thread = kcalloc(1, sizeof *initial_thread);
    thread_init(initial_thread);

    create_thread(&idle_thread, &idle_thread_func, NULL);

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

    struct thread_context *c = &newthr->context;
    void **stack = thread_stack_top(newthr) - 1;
    *stack = &start_thread;
    c->sp = (uint8_t *)stack - sizeof(struct register_state) - 8;
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
    thr->magic = KTHREAD_MAGIK;

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
    disable_interrupts();
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
    barrier();
    enum interrupt_level old_level = disable_interrupts();

    if (thr != idle_thread) {
        list_push_back(&ready_threads, &thr->status_elem);
    }
    thr->status = THRSTAT_READY;

    set_interrupt_level(old_level);
}

#define SAVE_CONTEXT "pushf\n\tpushq %%rax\n\tpushq %%rdx\n\tpushq %%rcx\n\tpushq %%rbx\n\tpushq %%rsi\n\tpushq %%rdi\n\tpushq %%r8\n\tpushq %%r9\n\tpushq %%r10\n\tpushq %%r11\n\t"
#define RESTORE_CONTEXT "popq %%r11\n\tpopq %%r10\n\tpopq %%r9\n\tpopq %%r8\n\tpopq %%rdi\n\tpopq %%rsi\n\tpopq %%rbx\n\tpopq %%rcx\n\tpopq %%rdx\n\tpopq %%rax\n\tpopf\n\t"

void switch_to(struct thread *prev, struct thread *next)
{   
    barrier();
    __asm__ volatile (
        SAVE_CONTEXT
        "movq %%rsp, %P[THREAD_RSP](%[PREV])\n\t"
        "movq %P[THREAD_RSP](%[NEXT]), %%rsp\n\t"
        RESTORE_CONTEXT
        ::
        [PREV] "D" (prev), [NEXT] "S" (next),
        [THREAD_RSP] "i" (offsetof(struct thread, context.sp))
    );

}

void schedule_next(void) 
{
    struct thread *next = next_thread();
    struct thread *prev = running_thread;
    running_thread = next;
    switch_to(prev, next);

    if (prev->status == THRSTAT_DEAD) {
        kfree(prev);
    }
}

void exit_thread(void)
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
    while (true) {
        halt();
    }
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
    while (!list_empty(&c->waiters)) {
        condvar_signal_one(c, l);
    }
}
