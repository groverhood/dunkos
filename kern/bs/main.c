
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

	puts("Initialized console... printing \"Hello, world!\"...");
	puts("Hello, world!");

	init_interrupts();

	__asm__ volatile ("movq $1, %rax");
	__asm__ volatile ("int $2");
	//puts("\nInitializing memory system...");
	//init_memory();
	//puts("Memory system initialized!");
}