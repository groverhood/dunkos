#ifndef DUNKOS_CONSOLE_H
#define DUNKOS_CONSOLE_H

#include <stddef.h>

void init_console(void);

void toggle_scrolling(void);

void acquire_console(void);

void clear_console(void);

void scroll_down(void);

void newline(void);

ssize_t write_console(void *buf, size_t count);

void release_console(void);

#endif