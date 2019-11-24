#ifndef DUNKOS_THREAD_H
#define DUNKOS_THREAD_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <interrupt.h>
#include <util/list.h>
#include <kern/synch.h>

typedef int tid_t;

#define PRI_MIN 	0x00
#define PRI_DEFAULT 0x1F
#define PRI_MAX		0x3F

#define KTHREAD_MAGIK	   (0xF42069ACAB131214)

enum thread_status {
	THRSTAT_BLOCKED,
	THRSTAT_READY,
	THRSTAT_RUNNING,
	THRSTAT_DEAD
};

typedef void thread_function(void *);

struct thread_context {
	size_t *sp;
	void *ip;
	thread_function *entry;
	void *aux;
};
struct thread {
	tid_t id;
	enum thread_status status;

	int64_t sleep_end;
	struct semaphore sleep_sema;
	
	int priority;
	struct list donors;

	struct list_elem status_elem;
	struct list_elem sleep_elem;
	struct list_elem donor_elem;

	struct thread_context context;

	uint64_t magic;
};


/* Initialize thread functionality. */
void init_threads(void);

void create_thread(struct thread **, thread_function *, void *aux);

struct thread *get_current_thread(void);
tid_t get_current_id(void);
int get_current_priority(void);
void set_current_priority(int);
enum thread_status get_current_status(void);
void yield_current(void);
void block_current(void);
void schedule_next(void);
void exit_thread(void);

int thread_get_priority(struct thread *restrict);
void thread_block(struct thread *);
void thread_unblock(struct thread *);

#define thread_compare(lf, ri, elem)\
	({\
		thread_get_priority(elem_value(lf, struct thread, elem))\
		< thread_get_priority(elem_value(ri, struct thread, elem));\
	})

extern bool threads_init;

#endif