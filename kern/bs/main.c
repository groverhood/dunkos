
#include <stdio.h>
#include <console.h>
#include <interrupt.h>
#include <kern/memory.h>
#include <kern/timer.h>
#include <kern/thread.h>

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
	init_memory();
	puts("Memory system initialized!");

	init_threads();
	init_timer();
}