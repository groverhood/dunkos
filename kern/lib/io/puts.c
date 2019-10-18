

#include <stdio.h>
#include <string.h>
#include <console.h>

int puts(const char *s)
{
	int result;
	acquire_console();

	result = (int)write_console(s, strlen(s));

	newline();

	release_console();

	return result;
}

int putchar(int c)
{
	unsigned char ch = (unsigned char)c;
	acquire_console();
	write_console(&ch, 1);
	release_console();
	return c;
}