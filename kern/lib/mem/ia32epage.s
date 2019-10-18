
.global prep_paging_ia32e
prep_paging_ia32e:
	movq %cr4, %rax
	orq $(1 << 17), %rax
	movq %rax, %cr4
	ret