
.section .text
.code32
.global enable_paging
.global set_up_page_tables
.extern p2_table
.extern p3_table
.extern p4_table

set_up_page_tables:
	movl $p3_table, %eax
	orl $0x03, %eax
	movl %eax, (p4_table)

	movl $p2_table, %eax
	orl $0x03, %eax
	movl %eax, (p3_table)
	
	movl $0, %ecx
	.map_p2_table:
		movl %ecx, %eax
		shll $21, %eax
		orl $0x83, %eax
		movl %eax, p2_table(, %ecx, 8)
		addl $1, %ecx
		cmpl $0x200, %ecx
		jl .map_p2_table

	ret

enable_paging:
	movl %cr0, %eax
	andl $(~(1 << 31)), %eax
	movl %eax, %cr0

	movl $p4_table, %eax
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