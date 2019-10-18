#include <stdio.h>
#include <kern/paging.h>
#include <kern/memory.h>
#include <stdbool.h>

extern struct pml4_entry *pml4_table;
extern struct pdpt_entry *pdp_table;
extern struct pagedir_entry *pagedir_table;

void *kernel_page_top;

static void load_page(void *);

static inline struct pml4_entry *get_pml4_table(void)
{
	struct cr3 _cr3;
	__asm__ (
		"movq %%cr3, %0"
		: "=r" (_cr3)
	);

	return (struct pml4_entry *)(_cr3.addr);
}

static inline struct pdpt_entry *get_pdpt_entry(struct pml4_entry *e)
{
	return (struct pdpt_entry *)(e->addr);
}

static inline struct pagedir_entry *get_pagedir_entry(struct pdpt_entry *e)
{
	return (struct pagedir_entry *)(e->pagedir);
}

static inline void *get_page(struct pagedir_entry *e)
{
	return (void *)(e->pgaddr);
}

enum interrupt_defer page_fault(struct interrupt *intr, 
								void *intrframe_,
								struct register_state *registers)
{
	struct fault_interrupt_frame *intrframe = intrframe_;
	unsigned long error = intrframe->error;

	puts("Page fault!");

	__asm__ volatile ("hlt");

	return INTRDEFR_NONE;
}

void *palloc(void)
{
	
}

void init_paging(void)
{
	kernel_page_top = (void *)(1 << 21);
}

/**
 *  Load page containing virtual
 *  address ADDR.
 **/
static void load_page(void *addr)
{

}