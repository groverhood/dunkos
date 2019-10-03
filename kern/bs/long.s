
.section .text
.global long_mode_start
.extern

long_mode_start:
	call kernel
	hlt