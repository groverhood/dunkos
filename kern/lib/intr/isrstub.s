.extern isr_common_stub
.extern _idt_descriptor

.altmacro

.macro defstub n:req
.global isr_stub\n
isr_stub\n:
	pushq %rbp
	movq %rsp, %rbp
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rax
	cld
	movq $\n, %rdi
	leaq 0x8(%rbp), %rsi
	movq %rsp, %rdx
	call isr_common_stub
	popq %rax
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %rbp
	iretq
.endm

.set n, 0
.rept 256
    defstub %n
    .set n, n + 1
.endr


.global _install_stubs
_install_stubs:
	.macro install_stub n
		movl $\n, %edi
		movq $isr_stub\n, %rsi
		movl $0x08, %edx
		movl $0x8E, %ecx
		call _idt_descriptor
	.endm
	.set n, 0
	.rept 256
		install_stub %n
		.set n, n + 1
	.endr
	retq