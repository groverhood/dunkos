#ifndef DUNKOS_THREAD_H
#define DUNKOS_THREAD_H

#include <stddef.h>
#include <stdbool.h>
#include <interrupt.h>
#include <util/list.h>
#include <kern/synch.h>

typedef int tid_t;

#define PRI_MIN 	0x00
#define PRI_DEFAULT 0x1F
#define PRI_MAX		0x3F

enum thread_status {
	THRSTAT_BLOCKED,
	THRSTAT_READY,
	THRSTAT_RUNNING
};

struct thread {
	tid_t id;
	enum thread_status status;

	long sleep_end;
	struct semaphore sleep_sema;
	
	int priority;
	struct list donors;

	struct list_elem status_elem;
	struct list_elem sleep_elem;
	struct list_elem donor_elem;
	struct list_elem child_elem;

	struct list children;

	size_t *pagedir;
	void *ip;
};

typedef void thread_function(void *);

void init_threads(void);

void create_thread(struct thread **, thread_function *, void *);
void init_thread(struct thread *);

struct thread *get_current_thread(void);
tid_t get_current_id(void);
int get_current_priority(void);
void set_current_priority(int);
enum thread_status get_current_status(void);
void yield_current(void);
void block_current(void);
void schedule_next(void);

int get_thread_priority(struct thread *restrict);
void block_thread(struct thread *);
void unblock_thread(struct thread *);

#define thread_compare(lf, ri, elem)\
	({\
		get_thread_priority(elem_value(lf, struct thread, elem))\
		< get_thread_priority(elem_value(ri, struct thread, elem));\
	})

#endif