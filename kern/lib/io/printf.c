#include <stdio.h>

#define PRINTF_GBUFSZ (0x100)
static char gbuf[PRINTF_GBUFSZ];

int vprintf(const char *format, va_list argv)
{
	int argc = vsprintf(gbuf, format, argv);
	write_console(gbuf, strlen(gbuf));
	return argc;
}