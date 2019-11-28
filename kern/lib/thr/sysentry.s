.extern syscall_table

.global syscall_handler
.type @function
syscall_handler:
    jmp *syscall_table(, %rax, 8)