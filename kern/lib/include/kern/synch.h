#ifndef DUNKOS_SYNCH_H
#define DUNKOS_SYNCH_H

#include <util/list.h>

/**
 * 	Use these synchronization primitives instead of the provided thread
 *  functions whenever possible. 
 **/

struct semaphore {
	unsigned long value;
	struct list waiters;
};

void semaphore_init(struct semaphore *, unsigned long);
void semaphore_inc(struct semaphore *);
void semaphore_dec(struct semaphore *);

struct lock {
	struct semaphore binary;
	struct thread *holder;
};

void lock_init(struct lock *);
void lock_acquire(struct lock *);
void lock_release(struct lock *);

struct condvar {
	struct list waiters;
};

void condvar_init(struct condvar *);
void condvar_wait(struct condvar *, struct lock *);
void condvar_signal_one(struct condvar *, struct lock *);
void condvar_signal_all(struct condvar *, struct lock *);

#endif