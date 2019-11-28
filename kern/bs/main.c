
#include <stdio.h>
#include <console.h>
#include <interrupt.h>
#include <kern/pml4.h>
#include <kern/timer.h>
#include <kern/memory.h>
#include <kern/paging.h>
#include <kern/thread.h>
#include <kern/heap.h>
#include <kern/process.h>
#include <algo.h>

static void idle_loop(void);

/* Initialize kernel state and begin idle task. */
void kernel(void)
{
	init_console();

	puts("Initializted console... printing \"Hello, world!\"...");
	puts("Hello, world!");

	init_interrupts();

	puts("\nInitializing memory system...");
	init_pml4();
	init_paging();
	init_heap();
	puts("Memory system initialized!\n");

	init_threads();
	init_syscalls();
	init_timer();
	puts("Threads initialized!");

	/* Begin the terminal shell. */
	static char *dash_argv[2];
	dash_argv[0] = "/bin/dash";
	dash_argv[1] = NULL;

	exec_process("/bin/dash", dash_argv);
	
	/* Become the idle task.
	   
	   "Be formless, shapeless, like water. Now you put water into a cup, 
	   it becomes the cup. You put water into a bottle, it becomes the 
	   bottle. You put it in a teapot, it becomes the teapot. Now water 
	   can flow or it can crash. Be water, my friend."
	   
	   -- Bruce Lee */   
	puts("\nIdling...");
	while (1) idle_loop();
}

static void idle_loop(void)
{
	yield_current();
}