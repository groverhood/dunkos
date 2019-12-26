/**
 *  Common entry points for user I/O.
 **/

#include <stdio.h>

int vprintf(const char *format, va_list argv)
{
    return vfprintf(stdout, format, argv);
}

int vscanf(const char *format, va_list argv)
{
    return vfscanf(stdin, format, argv);
}

int scanf(const char *format, ...)
{
    int argc;
    va_list argv;

    va_start(argv, format);
    argc = vscanf(format, argv);
    va_end(argv);

    return argc;
}