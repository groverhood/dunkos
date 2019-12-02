
#include <kern/process.h>
#include <kern/thread.h>
#include <kern/heap.h>
#include <kern/pml4.h>
#include <kern/asm.h>
#include <kern/vmmgmt.h>
#include <string.h>

static void process_init(struct process *pr)
{
    /* Same as &pr->base. */
    struct thread *thr = (struct thread *)pr;
    thread_init(thr);
    thr->magic = PROCESS_MAGIK;
    pr->exit_code = -1;
    page_table_create(&pr->spt);
}

struct process *current_process(void)
{
    struct thread *cur = get_current_thread();
    return is_process(cur) ? (struct process *)(cur) : cur->parent;
}

struct process *fork_process(void)
{
    struct process *cur = current_process();
    struct process *pr = calloc(1, sizeof *pr);
    
    process_init(pr);
    struct thread *thr = process_get_base(pr);

    /* Copy the stack pointer and instruction pointer so as to resume
       execution after the fork() system call. */
    memcpy(&thr->context, &cur->base.context, sizeof(struct thread_context));
    list_push_back(&cur->children, &pr->base.child_elem);

    /* Ensure an identical address space that doesn't mutate the parent's. */
    page_table_copy(pr->spt, cur->spt);

    return pr;
}

void exec_process(const char *file, char **argv)
{
    
}

__attribute__((noreturn)) void exit_process(int status)
{
    struct process *cur = current_process();

    cur->exit_code = status;
    
    struct list_elem *el;
    for (el = list_begin(&cur->children); el != list_end(&cur->children);
        el = list_next(el))
    {
        struct thread *chld = elem_value(el, struct thread, child_elem);
        if (is_process(chld))
            semaphore_inc(&((struct process *)chld)->reap_sema);
    }

    page_table_destroy(cur->spt);

    semaphore_inc(&cur->wait_sema);
    semaphore_dec(&cur->reap_sema);

    exit_thread();
    unreachable();
}
