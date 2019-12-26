#ifndef DUNKOS_STDIO_H
#define DUNKOS_STDIO_H

#include <stddef.h>
#include <stdarg.h>

/* Discrete marker with all bits set. */
#define EOF (-1)

/* Opaque I/O stream object. Use this over file descriptors whenever 
   applicable. This struct is not available in kernel mode. */
typedef struct _fdbuf FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* These functions are all available in kernel mode. */

int puts(const char *s);
int vsnprintf(char *dest, size_t sz, const char *format, va_list argv);
int vsprintf(char *dest, const char *format, va_list argv);
int snprintf(char *dest, size_t sz, const char *format, ...);
int sprintf(char *dest, const char *format, ...);
int vprintf(const char *format, va_list argv);
int printf(const char *format, ...);
int putchar(int c);

/* END kernel-friendly functions. */

FILE *fopen(const char *path, const char *modes);
FILE *fdopen(int fd, const char *modes);
void fclose(FILE *stream);
void fflush(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fileno(FILE *stream);
int vfprintf(FILE *stream, const char *format, va_list argv);
int fprintf(FILE *stream, const char *format, ...);
int vdprintf(int fd, const char *format, va_list argv);
int dprintf(int fd, const char *format, ...);
int fputc(int c, FILE *stream);
int vfscanf(FILE *stream, const char *format, va_list argv);
int fscanf(FILE *stream, const char *format, ...);

void flockfile(FILE *stream);
void funlockfile(FILE *stream);

#define putc fputc

int getchar(void);
int vsscanf(char *str, const char *format, va_list argv);
int sscanf(char *str, const char *format, ...);
int vscanf(const char *format, va_list argv);
int scanf(const char *format, ...);

#endif