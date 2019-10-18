#ifndef DUNKOS_TIMER_H
#define DUNKOS_TIMER_H

struct timespec {
	unsigned long seconds;
	unsigned long nanoseconds;
};

#define duration(s, n) ((struct timespec){ s, n })

void init_timer(void);

void sleep_ticks(long);

void sleep_time(const struct timespec *duration);

#endif