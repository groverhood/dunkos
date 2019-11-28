
ENTRY(_start)
SECTIONS 
{
    
    . = 1M;
    _begin = .;

    .text : {
        *(.boot);
        *(.text);
        _etext = .;
    }

    . = 2M;

    .data : {
        *(.data);
        _edata = .;
    }

    .bss : {
        *(.bss);
        _end = .;
    }

    . = 4M;
    _pool_begin = .;

    . = 6M;
    _apic_base = .;

    . = 8M;
    _reserved_end = .;
}