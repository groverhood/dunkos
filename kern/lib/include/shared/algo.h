#ifndef DUNKOS_ALGO_H
#define DUNKOS_ALGO_H

#include <stdint.h>
#include <stddef.h>

#define pure __attribute__((pure))

#ifndef __cplusplus

#define max(a, b) \
({ const __typeof__(a) _a = (a); \
	const __typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })

#define min(a, b) \
({ const __typeof__(a) _a = (a); \
	const __typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#endif

#define isnegative(i, size) ({ const __typeof__(i) _i = i; \
						!(_i ^ (1 << (size * 8 - 1))); })

#define iisnegative(i) (isnegative(i, sizeof i))

#define _name(x) #x
/* Get stringified version of something, including a macro. */
#define name(x) _name(x)

#define halt() __asm__ volatile ("hlt")

#define getbyte(i, n) (((i) >> (((n) * 8))) & 0xFF)

inline static size_t hash_uint64(uint64_t h)
{
	h ^= h >> 33;
	h *= 0xFF51AFD7ED558CCDUL;
	h ^= h >> 33;
	h *= 0xC4CEB9FE1A85EC53UL;
	h ^= h >> 33;
	return h;
}

#define div_rnd_up(n, d) (((n) + (d) - 1) / (d))

#define safemul(a, b, res)\
({\
	__typeof__(a) _a = a;\
	__typeof__(b) _b = b;\
	__typeof__(_a * _b) prod = _a * _b;\
	*res = prod;\
	!!(prod & (1 << sizeof prod - 1))\
	== !!(_a & (1 << sizeof _a - 1)\
	== !!(_b & (1 << sizeof _b - 1)));\
})

#define safeadd(a, b, res)\
({\
	__typeof__(a) _a = a;\
	__typeof__(b) _b = b;\
	__typeof__(_a * _b) sum = _a * _b;\
	*res = sum;\
	!!(sum & (1 << sizeof sum - 1))\
	== !!(_a & (1 << sizeof _a - 1)\
	== !!(_b & (1 << sizeof _b - 1)));\
})

static inline size_t hash_str(const char *restrict s)
{
	size_t h = (size_t)525201411107845655;
	while (*s) {
		h = ((h ^ *s) * 0x5BD1E9955BD1E995);
		h = h ^ (h >> 47);
	}

	return h;
}

#endif