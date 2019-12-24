#ifndef DUNKOS_TIMER_H
#define DUNKOS_TIMER_H

#include <timespec.h>

void init_timer(void);
void sleep_timespec(const struct timespec *duration);

#endif