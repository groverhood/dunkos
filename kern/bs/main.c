
#include <stdio.h>
#include <console.h>
#include <interrupt.h>
#include <kern/pml4.h>
#include <kern/timer.h>
#include <kern/memory.h>
#include <kern/paging.h>
#include <kern/thread.h>
#include <kern/heap.h>

/**
 *  Initialize kernel state and begin OS process.
 **/
void kernel(void)
{
	init_console();

	puts("Initialized console... printing \"Hello, world!\"...");
	puts("Hello, world!");

	init_interrupts();

	puts("\nInitializing memory system...");
	init_pml4();
	init_paging();
	init_heap();
	puts("Memory system initialized!");

	init_threads();
	init_timer();
	puts("Threads initialized!");
	
	exit_thread();
}