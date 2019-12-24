#ifndef DUNKOS_TIMESPEC_H
#define DUNKOS_TIMESPEC_H

#include <stdint.h>

struct timespec {
	uint64_t seconds;
	uint64_t nanoseconds;
};

#define NANO_PER_SEC 1000000000
#define US_PER_SEC 1000000
#define MS_PER_SEC 1000

#endif