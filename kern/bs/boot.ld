
ENTRY(_start)
SECTIONS 
{
    
    . = 1M;
    _begin = .;

    .text : {
        *(.boot);
        *(.text);
        _etext = ABSOLUTE(.);
    }

    . = 2M;

    .data : {
        *(.data);
        _edata = ABSOLUTE(.);
    }

    .bss : {
        *(.bss);
        _end = ABSOLUTE(.);
    }

    . = 4M;
    _pool_begin = .;

    . = 6M;
    _apic_base = .;

    . = 8M;
    _reserved_end = .;
}