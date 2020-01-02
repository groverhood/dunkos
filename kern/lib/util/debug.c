#include <util/debug.h>
#include <algo.h>
#include <stdio.h>
#include <stdbool.h>

void panic(const char *msg, const char *file, int line)
{
	printf("KERNEL PANIC! %s at %s:%i\n", msg, file, line);
	halt();
}