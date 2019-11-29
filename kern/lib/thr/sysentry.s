.extern syscall_table

.global syscall_handler
.type syscall_handler, @function
syscall_handler:
    call *syscall_table(, %rax, 8)
    sysret