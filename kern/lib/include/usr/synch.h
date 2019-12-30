#ifndef DUNKOS_SYNCH_H
#define DUNKOS_SYNCH_H

typedef int sema_t;

/* Allocate a semaphore. */
sema_t sget(unsigned long value);
/* Deallocate a semaphore. */
void sunget(sema_t);
void sdec(sema_t);
void sinc(sema_t);

struct lock {
    sema_t sema;
    int holder;
};

void lock_init(struct lock *);
void lock_acquire(struct lock *);
void lock_release(struct lock *);

#endif