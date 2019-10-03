
#include <stdio.h>
#include <console.h>
#include <memory.h>

/**
 *  Initialize kernel state and begin OS process.
 **/
void kernel(void)
{
	init_console();
	puts("Initialized console... printing \"Hello, world!\", and \"1\"");
	puts("Hello, world!");
	printf("%i\n");

	//puts("\nInitializing memory system...");
	//init_memory();
	//puts("Memory system initialized!");
}