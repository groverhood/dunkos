
ENTRY(_start)
SECTIONS 
{
    
    . = 1M;

    .text : {
        *(.boot);
        *(.text)
        _etext = .;
    }

    .data : {
        *(.data)
        _edata = .;
    }

    .bss : {
        *(.bss)
        _end = .;
    }
}