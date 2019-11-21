
#include <stdio.h>
#include <console.h>
#include <interrupt.h>
#include <kern/memory.h>
#include <kern/paging.h>
#include <kern/timer.h>
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
	init_paging();
	init_heap();
	puts("Memory system initialized!");

	printf("%p %p\n", malloc(4));

	// init_threads();
	// init_timer();
}