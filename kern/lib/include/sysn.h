#ifndef DUNKOS_SYSN_H
#define DUNKOS_SYSN_H

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
    SYS_COUNT
};

#endif