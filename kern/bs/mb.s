
.section .boot
.align 8
header_begin: 
	.long 0xE85250D6
	.long 0 
	.long (header_end - header_begin)
	.long 0x100000000 - (0xE85250D6 + (header_end - header_begin))
.align 8
end_tag:
    .word 0
    .word 0
    .long 8
header_end:

