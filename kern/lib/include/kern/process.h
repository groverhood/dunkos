#ifndef DUNKOS_PROCESS_H
#define DUNKOS_PROCESS_H

#include <util/list.h>
#include <util/hashtable.h>
#include <kern/synch.h>
#include <kern/thread.h>
#include <kern/vmmgmt.h>

#define PROCESS_MAGIK (~KTHREAD_MAGIK)

typedef tid_t pid_t;

struct process {
	struct thread base;

	/* The exit status of the process. */
	int exit_code;

	/* Used for wait() system call. */
	struct semaphore wait_sema;

	/* Used to ensure resources aren't deallocated prior to
	   the parent needing them. */
	struct semaphore reap_sema;

	/* Child processes and threads. */
	struct list children;

	struct file *executable;
	struct dir *cwd;

	struct page_table *spt;
	struct fdtable *fdtable;

	uint8_t *code_segment_begin;
	uint8_t *code_segment_end;

	uint8_t *data_segment_begin;
	uint8_t *data_segment_end;

	uint8_t *heap_begin;
	uint8_t *heap_end;
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

void process_init(struct process *);

struct process *current_process(void);

/* Spawn a new child process. */
struct process *fork_process(void);

/* Override the current process with a new one. */
void exec_process(const char *file, char **argv);

/* Exit the currently executing process with the provided
   exit code. */
void exit_process(int status);

#endif