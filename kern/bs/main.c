
#include <stdio.h>
#include <console.h>
#include <interrupt.h>
#include <pml4.h>
#include <timer.h>
#include <memory.h>
#include <paging.h>
#include <thread.h>
#include <heap.h>
#include <process.h>
#include <block.h>
#include <ide.h>
#include <filesys.h>
#include <system.h>
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

	
	// puts("\nInitializing threads...");
	// init_threads();
	// init_syscalls();
	// init_timer();
	// puts("Threads initialized!");

	// puts("\nInitializing filesystem...");
	// init_ide();
	// init_block_devices();
	// init_filesystem();
	// puts("Filesystem initialized!");

	// /* Begin the terminal shell. */
	// if (fork() == 0) {
	// 	exec_process("/bin/dash", (char *[]){ "/bin/dash", NULL });
	// }
	
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
	if (threads_init) {
		yield_current();
	}
}