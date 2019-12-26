#include <system.h>
#include <kern/asm.h>
#include <kern/timer.h>
#include <kern/fdtable.h>
#include <stdint.h>

/* IA32_LSTAR msr. */
#define SYSCALL_ADDR 0xC0000082

/* Cannot be static, as we use this in sysentry. */
void *syscall_table[SYS_COUNT];

/* In reality, this isn't a function, but rather the location of a call
   instruction that uses the syscall number as an offset into the syscall
   table. */
extern void syscall_handler(void);

void init_syscalls(void)
{
    syscall_table[SYS_CLOSE] = &close;
    syscall_table[SYS_CREATE] = &create;
    syscall_table[SYS_EXEC] = &exec;
    syscall_table[SYS_EXIT] = &exit;
    syscall_table[SYS_FORK] = &fork;
    syscall_table[SYS_GETPID] = &getpid;
    syscall_table[SYS_OPEN] = &open;
    syscall_table[SYS_READ] = &read;
    syscall_table[SYS_REMOVE] = &remove;
    syscall_table[SYS_WAIT] = &wait;
    syscall_table[SYS_WRITE] = &write;
    syscall_table[SYS_CHMOD] = &chmod;
    syscall_table[SYS_CHMODFD] = &chmodfd;
    syscall_table[SYS_SLEEP] = &sleep;
    syscall_table[SYS_SLEEPTS] = &sleepts;

    wrmsr(SYSCALL_ADDR, (uint64_t)&syscall_handler);
}

pid_t fork(void)
{
    struct process *frk = fork_process();

    /* This may seem like an evil setup for goto, but really the goal of this 
       is to ensure that our newly forked child process can be identified 
       within itself. */
    frk->base.context.ip = &&fork_return;
    thread_unblock(process_get_base(frk));
    return (fork_process()->base.id);

fork_return:
    return (0);
}

pid_t getpid(void)
{
    return (current_process()->base.id);
}

int exec(const char *file, char **argv)
{
    exec_process(file, argv);

    /* Return -1 upon failure, as we shouldn't reach this
       point in the process. */
    return -1;
}

void exit(int status)
{
    exit_process(status);
}

int wait(pid_t p)
{
    int status = -1;
    struct process *cur = current_process();
    struct list_elem *e;

    for (e = list_begin(&cur->children); e != list_end(&cur->children);
        e = list_next(e))
    {
        struct process *chld;
        struct thread *t = elem_value(e, struct thread, child_elem);
        if (is_process(t) && t->id == p) {
            chld = (struct process *)t;

            semaphore_dec(&chld->wait_sema);
            status = chld->exit_code;
            list_remove(&cur->children, &t->child_elem);
            semaphore_inc(&chld->reap_sema);
        }
    }

    return (status);
}

bool create(const char *file, mode_t m)
{
    return create_file(current_process()->cwd, file, m);
}

bool remove(const char *file)
{
    return remove_file(current_process()->cwd, file);
}

int open(const char *file, int flags)
{
    return fd_get(fdtable_open(current_process()->fdtable, file));
}

void close(int fd)
{
    fdtable_free(current_process()->fdtable, fd);
}

bool eof(int fd)
{
    return fd_eof(fdtable_lookupfd(current_process()->fdtable, fd));
}

ssize_t write(int fd, const void *src, size_t bytes)
{
    return fd_write(fdtable_lookupfd(current_process()->fdtable, fd), src, bytes);
}

ssize_t read(int fd, void *dest, size_t bytes)
{
    return fd_read(fdtable_lookupfd(current_process()->fdtable, fd), dest, bytes);
}

void sleep(long ms)
{
    int64_t nano = (ms * NANO_PER_SEC / MS_PER_SEC) % NANO_PER_SEC;
    int64_t seconds = (ms / MS_PER_SEC);

    struct timespec ts;
    ts.nanoseconds = nano;
    ts.seconds = seconds;

    sleep_timespec(&ts);
}

void sleepts(const struct timespec *ts)
{
    sleep_timespec(ts);
}
