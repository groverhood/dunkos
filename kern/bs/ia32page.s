
.section .text
.code32
.global prep_paging_ia32
.extern pml4_table
.extern pdp_table
.extern pagedir_table

set_up_page_tables:
	movl $pdp_table, %eax
	orl $0x03, %eax
	movl %eax, (pml4_table)

	movl $pagedir_table, %eax
	orl $0x03, %eax
	movl %eax, (pdp_table)
	
	movl $0, %ecx
	.map_pagedir_table:
		movl %ecx, %eax
		shll $21, %eax
		orl $0x83, %eax
		movl %eax, pagedir_table(, %ecx, 8)
		addl $1, %ecx
		cmpl $0x200, %ecx
		jl .map_pagedir_table

	ret

prep_paging_ia32:
	call set_up_page_tables

	movl %cr0, %eax
	andl $(~(1 << 31)), %eax
	movl %eax, %cr0
	
	movl $pml4_table, %eax
	movl %eax, %cr3

	movl %cr4, %eax
	orl $(1 << 5), %eax
	movl %eax, %cr4

	movl $0xC0000080, %ecx
	rdmsr
	orl $(1 << 8), %eax
	wrmsr

	movl %cr0, %eax
	orl $(1 << 31), %eax
	movl %eax, %cr0

	ret

