#ifndef DUNKOS_TIMER_H
#define DUNKOS_TIMER_H

#include <stdint.h>

struct timespec {
	uint64_t seconds;
	uint64_t nanoseconds;
};

#define duration(s, n) ((struct timespec){ s, n })

void init_timer(void);

void sleep_ticks(int64_t);

void sleep_time(const struct timespec *duration);

#endif