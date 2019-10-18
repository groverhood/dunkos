#ifndef DUNKOS_PAGING_H
#define DUNKOS_PAGING_H

#include <interrupt.h>
#include <stdbool.h>

struct cr3 {
	unsigned long _ignore0 	: 3;
	unsigned long pwt		: 1;
	unsigned long pcd		: 1;
	unsigned long _ignore1 	: 7;
	unsigned long addr		: 40;
	unsigned long _reserved : 12;
} __attribute__((packed));

#define PML4_COUNT (1 << 9)

struct pml4_entry {

	unsigned long present 	: 1;
	unsigned long rdwr 		: 1;
	unsigned long usrspr 	: 1;
	unsigned long pwt		: 1;
	unsigned long pcd		: 1;
	unsigned long accessed	: 1;
	unsigned long _rest		: 6; /* Must be zero. */

	unsigned long addr 		: 40;
	unsigned long _reserved	: 11;
	unsigned long exec_dsbl : 1;

} __attribute__((packed));

struct pdpt_entry {
	unsigned long present 	: 1;
	unsigned long rdwr 		: 1;
	unsigned long usrspr 	: 1;
	unsigned long pwt		: 1;
	unsigned long pcd		: 1;
	unsigned long accessed	: 1;
	unsigned long _rest		: 6; /* Must be zero. */

	unsigned long pagedir	: 39;
	unsigned long _zero		: 1;

	unsigned long _reserved	: 11;
	unsigned long exec_dsbl : 1;

} __attribute__((packed));

struct pagedir_entry {
	unsigned long present 	: 1;
	unsigned long rdwr 		: 1;
	unsigned long usrspr 	: 1;
	unsigned long pwt		: 1;
	unsigned long pcd		: 1;
	unsigned long accessed	: 1;
	unsigned long dirty		: 1;
	unsigned long pgsize	: 1; /* Must be set to 1. */
	unsigned long global	: 1;
	unsigned long _rest		: 3;

	unsigned long pat		: 1;
	unsigned long _zero0	: 8;
	unsigned long pgaddr	: 31; /* Address of 2MB page. */

	unsigned long _ignore	: 7;
	unsigned long pkey		: 4;
	unsigned long exec_dsbl	: 1;
} __attribute__((packed));

interrupt_handler page_fault;

void init_paging(void);

#endif