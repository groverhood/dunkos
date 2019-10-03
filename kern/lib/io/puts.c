

#include <stdio.h>
#include <string.h>
#include <console.h>

int puts(const char *s)
{
	int result;
	acquire_console();

	result = (int) write_console(s, strlen(s));

	newline();

	release_console();

	return result;
}