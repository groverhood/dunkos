
.section .text
.code32
.extern gdt64p
.extern gdt64data
.extern prep_paging_ia32
.global _start

_start:
	movl $stack_top, %esp

	call check_multiboot
	call check_cpuid
	call check_long_mode
	call prep_paging_ia32
	
	lgdt (gdt64p)

	movl $gdt64data, %eax
	movw %ax, %ss
	movw %ax, %ds
	movw %ax, %es

	jmp $0x08, $long_mode_start
	
check_multiboot:
	cmpl $0x36D76289, %eax
	jne .no_multiboot
	ret
.no_multiboot:
	movb $0x30, %al
	jmp error

check_cpuid:
	pushfl
	pop %eax

	movl %eax, %ecx
	xorl $(1 << 21), %eax

	push %eax
	popfl

	pushfl
	pop %eax

	push %ecx
	popfl

	cmpl %ecx, %eax
	je .no_cpuid
	ret
	
.no_cpuid:
	movb $0x31, %al
	jmp error

check_long_mode:
	movl $0x80000000, %eax
	cpuid
	cmpl $0x80000001, %eax
	jb .no_long_mode

	movl $0x80000001, %eax
	cpuid
	testl $(1 << 29), %edx
	jz .no_long_mode
	ret

.no_long_mode:
	mov $0x32, %al
	jmp error

error:
	movl $0x4F524F45, (0xB8000)
	movl $0x4F3A4F52, (0xB8004)
	movl $0x4F3A4F52, (0xB8008)
	movb %al, (0xB800A)
	hlt

.section .bss
.global pml4_table
.global pdp_table
.global pagedir_table

.align 0x1000
pml4_table:
.space 0x1000

.align 0x1000
pdp_table:
.space 0x1000

.align 0x1000
pagedir_table:
.space 0x1000

stack_bottom:
.space 0x3000
stack_top:
