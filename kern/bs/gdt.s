
.section .rodata
.global gdt64p
.global gdt64code
.global gdt64data
gdt64:
	.quad 0
.equ gdt64code, (. - gdt64)
	.quad (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)
.equ gdt64data, (. - gdt64)
	.quad (1<<44) | (1<<47) | (1<<41)
gdt64p:
	.word (. - gdt64 - 1)
	.quad gdt64

//  0x100128