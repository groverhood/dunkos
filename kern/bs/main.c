
#include <stdio.h>
#include <console.h>
#include <interrupt.h>
#include <memory.h>

/**
 *  Initialize kernel state and begin OS process.
 **/
void kernel(void)
{
	init_console();
	init_interrupts();

	puts("Initialized console... printing \"Hello, world!\", and \"1, 2, 3\"");
	puts("Hello, world!");
	printf("%c, %c, %c\n", '1', '2', '3');

	//volatile int x = 0;
	//volatile int y = 1 / x;

	//puts("\nInitializing memory system...");
	//init_memory();
	//puts("Memory system initialized!");
}