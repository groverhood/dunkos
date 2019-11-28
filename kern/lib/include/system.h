#ifndef DUNKOS_USR_SYSTEM_H
#define DUNKOS_USR_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __DUNKOS_USR
typedef int pid_t;
#else
#include <kern/process.h>
#endif

/* System call header. */

pid_t fork(void);
pid_t getpid(void);

int exec(const char *file, char **argv);
void exit(int);
int wait(pid_t);

bool create(const char *file);
bool remove(const char *file);

int open(const char *file);
void close(int fd);

ssize_t write(int fd, const void *, size_t);
ssize_t read(int fd, void *, size_t);

#endif