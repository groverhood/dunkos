#include <synch.h>
#include <system.h>

void lock_init(struct lock *lock)
{
    lock->holder = -1;
    lock->sema = sget(1);
}

void lock_acquire(struct lock *lock)
{
    sdec(lock->sema);
    lock->holder = getpid();
}

void lock_release(struct lock *lock)
{
    lock->holder = -1;
    sinc(lock->sema);
}