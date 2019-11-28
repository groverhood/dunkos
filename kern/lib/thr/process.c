
#include <kern/process.h>
#include <kern/thread.h>
#include <kern/heap.h>
#include <kern/pml4.h>
#include <kern/asm.h>
#include <string.h>

static void process_init(struct process *pr)
{
    /* Same as &pr->base. */
    struct thread *thr = (struct thread *)pr;
    thread_init(thr);
    thr->magic = PROCESS_MAGIK;
    pr->exit_code = -1;
    pr->pml4 = pml4_alloc();
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

    list_push_back(&cur->children, &pr->child_elem);

    /* Ensure an identical address space that doesn't mutate the parent's. */
    pml4_copy(pr->pml4, cur->pml4, true);

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
        
    }



    exit_thread();
    unreachable();
}
