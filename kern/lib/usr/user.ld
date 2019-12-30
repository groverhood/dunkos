
/* Layout for the DunkOS virtual address space. */
ENTRY(_start)
SECTIONS
{
    . = 8M;
    /* Place the ungrowable segments first. */
    .text ALIGN(2M) : {
        _btext = ABSOLUTE(.);
        *(.text);
        _etext = ABSOLUTE(ALIGN(2M));
    }

    /* Uninitialized data segment. */
    .data ALIGN(2M) : {
        _bdata = ABSOLUTE(.);
        *(.data);
        /* Heap subsection. */
        _edata = ABSOLUTE(ALIGN(2M));
    }

    /* Initialized data segment. We fit the heap into here so we can have
       zero-initialized blocks to allocate with calloc(). */
    .bss ALIGN(2M) : {
        _bbss = ABSOLUTE(.);
        *(.bss);
        _bheap = ABSOLUTE(ALIGN(2M));
        _ebss = ABSOLUTE(ALIGN(4 * 2M));
    }

    _end = .;
}