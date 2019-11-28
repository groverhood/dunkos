#include <system.h>
#include <sysn.h>
#include <kern/asm.h>
#include <stdint.h>

/* IA32_LSTAR msr. */
#define SYSCALL_ADDR 0xC0000082
#define sysreturn(value) __asm__ volatile ("sysret" :: "a" (value)); __builtin_unreachable()

static void *syscall_table[SYS_COUNT];

/* In reality, this isn't a function, but rather the location of a jump
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

    wrmsr(SYSCALL_ADDR, (uint64_t)&syscall_handler);
}

pid_t fork(void)
{
    sysreturn(fork_process()->base.id);
}

pid_t getpid(void)
{
    sysreturn(current_process()->base.id);
}


int exec(const char *file, char **argv)
{
    exec_process(file, argv);

    /* Return -1 upon failure, as we shouldn't reach this
       point in the process. */
    sysreturn(-1);
}

void exit(int status)
{
    exit_process(status);
    
}

int wait(pid_t p)
{

}


bool create(const char *file)
{

}

bool remove(const char *file)
{

}


int open(const char *file)
{

}

void close(int fd)
{

}


ssize_t write(int fd, const void *src, size_t bytes)
{

}

ssize_t read(int fd, void *dest, size_t bytes)
{

}
