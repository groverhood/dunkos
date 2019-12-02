#include <util/debug.h>
#include <algo.h>
#include <stdio.h>

void panic(const char *msg, const char *file, const char *line)
{
	printf("KERNEL PANIC! %s at %s:%s\n", msg, file, line);
	halt();
}