.extern syscall_table

/* Pseudo-function that preserves the syscall argument
   registers, invokes whichever system call was desired
   by the user, and executes sysret. */
.global syscall_handler
.type syscall_handler, @function
syscall_handler:
    call *syscall_table(, %rax, 8)
    sysret