#ifndef DUNKOS_PROCESS_H
#define DUNKOS_PROCESS_H

#include <kern/synch.h>
#include <util/list.h>
#include <kern/thread.h>

#define PROCESS_MAGIK (~KTHREAD_MAGIK)

typedef tid_t pid_t;

struct process {
	struct thread base;

	/* The root of the process's virtual address space. */
	size_t *pml4;

	/* The exit status of the process. */
	int exit_code;

	/* Used for wait() system call. */
	struct semaphore wait_sema;

	/* Used to ensure resources aren't deallocated prior to
	   the parent needing them. */
	struct semaphore reap_sema;

	/* Child processes. */
	struct list children;

	/* Used to store process in parent's child processes. */
	struct list_elem child_elem;
};

static inline bool is_process(struct thread *thr)
{
	return (thr->magic == PROCESS_MAGIK);
}

static inline struct thread *process_get_base(struct process *p)
{
	return &p->base;
}

static inline struct process *process_get_parent(struct process *p)
{
	return process_get_base(p)->parent;
}

void init_syscalls(void);

struct process *current_process(void);

/* Spawn a new child process. */
struct process *fork_process(void);

/* Override the current process with a new one. */
void exec_process(const char *file, char **argv);

/* Exit the currently executing process with the provided
   exit code. */
void exit_process(int status);

#endif