#include <system.h>
#include <stdint.h>
#include <synch.h>

#define SYSCALL_CLOBBER "rcx", "r11", "memory"
#define SYSCALL_ASM __asm__ volatile

#define reinterpret(val, ty) _Generic(((ty)val), void: (void)val, default: *(ty *)&val)

#define syscall0(retval, n)\
    ({\
        uint64_t res;\
        SYSCALL_ASM ("syscall" : "=a" (res)\
        : "a" (n) : SYSCALL_CLOBBER);\
        *(retval *)&res;\
    })

#define syscall1(retval, n, arg0)\
    ({\
        uint64_t res;\
        SYSCALL_ASM ("syscall" : "=a" (res)\
        : "a" (n), "D" (arg0) : SYSCALL_CLOBBER);\
        *(retval *)&res;\
    })

#define syscall2(retval, n, arg0, arg1)\
    ({\
        uint64_t res;\
        SYSCALL_ASM ("syscall" : "=a" (res)\
        : "a" (n), "D" (arg0), "S" (arg1) : SYSCALL_CLOBBER);\
        *(retval *)&res;\
    })

#define syscall3(retval, n, arg0, arg1, arg2)\
    ({\
        uint64_t res;\
        SYSCALL_ASM ("syscall" : "=a" (res)\
        : "a" (n), "D" (arg0), "S" (arg1), "d" (arg2)\
        : SYSCALL_CLOBBER);\
        *(retval *)&res;\
    })

pid_t fork(void)
{
    return syscall0(pid_t, SYS_FORK);
}

pid_t getpid(void)
{
    return syscall0(pid_t, SYS_GETPID);
}

int exec(const char *file, char **argv)
{
    return syscall2(int, SYS_EXEC, file, argv);
}

void exit(int status)
{
    syscall1(void, SYS_EXIT, status);
}

int wait(pid_t p)
{
    return syscall1(int, SYS_WAIT, p);
}

bool create(const char *file, mode_t m)
{
    return syscall2(bool, SYS_CREATE, file, m);
}

bool remove(const char *file)
{
    return syscall1(bool, SYS_REMOVE, file);
}

bool chmod(const char *file, mode_t m)
{
    return syscall2(bool, SYS_CHMOD, file, m);
}

int open(const char *file, int flags)
{
    return syscall2(int, SYS_OPEN, file, flags);
}

void close(int fd)
{
    syscall1(void, SYS_CLOSE, fd);
}

bool chmodfd(int fd, mode_t m)
{
    return syscall2(bool, SYS_CHMODFD, fd, m);
}

ssize_t write(int fd, const void *src, size_t bytes)
{
    return syscall3(ssize_t, SYS_OPEN, fd, src, bytes);
}

ssize_t read(int fd, void *dest, size_t bytes)
{
    return syscall3(ssize_t, SYS_READ, fd, dest, bytes);
}

void sleep(long ms)
{
    syscall1(void, SYS_SLEEP, ms);
}

void sleepts(const struct timespec *ts)
{
    syscall1(void, SYS_SLEEPTS, ts);
}

void *growdata(size_t newsz)
{
    return syscall1(void *, SYS_GROWDATA, newsz);
}

sema_t sget(unsigned long value)
{
    return syscall1(sema_t, SYS_SGET, value);
}

void sunget(sema_t sema)
{
    syscall1(void, SYS_SUNGET, sema);
}

void sinc(sema_t sema)
{
    syscall1(void, SYS_SINC, sema);
}

void sdec(sema_t sema)
{
    syscall1(void, SYS_SDEC, sema);
}