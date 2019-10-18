
#include <interrupt.h>
#include <kern/paging.h>
#include <kern/memory.h>

extern void prep_paging_ia32e(void);

void init_memory(void)
{
	prep_paging_ia32e();

	install_interrupt_handler(INTR_TYPE_PGFAULT, &page_fault);	
}