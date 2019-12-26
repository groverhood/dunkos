#ifndef DUNKOS_USR_SYSTEM_H
#define DUNKOS_USR_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <timespec.h>
#include <mode.h>

#ifdef __DUNKOS_USR
typedef int pid_t;
#else
#include <kern/process.h>
#endif


/* Number used to index the syscall table. */
enum syscall_number {
    SYS_FORK,
    SYS_GETPID,
    SYS_EXEC,
    SYS_EXIT,
    SYS_WAIT,
    SYS_CREATE,
    SYS_REMOVE,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_WRITE,
    SYS_READ,
    SYS_CHMOD,
    SYS_CHMODFD,
    SYS_SLEEP,
    SYS_SLEEPTS,

    SYS_COUNT
};

/* Spawn an identical process with a new pid.
   The new process will start at the return
   of the fork() invocation. */
pid_t fork(void);

/* Get the current process's id. */
pid_t getpid(void);

/* Execute the file at the provided path with
   the given arguments. */
int exec(const char *file, char **argv);

/* Exit the current process with the given
   status. */
void exit(int);

/* Wait for the specified process. */
int wait(pid_t);

/* Create an empty file at the provided path
   with the given mode. */
bool create(const char *file, mode_t);

/* Remove/unlink the provided path. */ 
bool remove(const char *file);

/* Change the mode at the given path. */
bool chmod(const char *file, mode_t);

#define OPEN_RD ((int)1)
#define OPEN_WR ((int)2)
#define OPEN_RW (OPEN_RD | OPEN_WR)

/* Open a file stream for the provided path, 
   returning its file descriptor. */
int open(const char *file, int flags);

/* Close the specified file descriptor. */
void close(int fd);

/* Has the stream terminated? */
bool eof(int fd);

/* Change the mode for the specified file
   descriptor. */
bool chmodfd(int fd, mode_t);

/* Write N bytes from the provided buffer to
   the specified file stream. */
ssize_t write(int fd, const void *, size_t);

/* Read N bytes into the provided buffer from
   the specified file stream. */
ssize_t read(int fd, void *, size_t);

/* Sleep for specified number of milliseconds. */
void sleep(long ms);

/* Sleep for the specified duration. */
void sleepts(const struct timespec *);

#endif