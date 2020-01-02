#include <string.h>
#include <stdint.h>

size_t strlen(const char *s)
{
	size_t result = 0;
	while (*s++) {
		result++;
	}
	return result;
}

char *strchr(const char *str, int chr)
{
	int c;
	while ((c = *str) != chr) {
		str++;
		if (!c) {
			return 0;
		}
	}

	return (char *)str;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	/* Naive approach. */
	unsigned char *pdest = dest;
	const unsigned char *psrc = src;
	while (n-- > 0) {
		*pdest++ = *psrc++;
	}

	return dest;
}

void *memset(void *dest, register int val, register size_t len)
{
	unsigned char *ptr = (unsigned char*)dest;
	if (len % sizeof(size_t) > 0) {
		while (len-- > 0 && len % sizeof(size_t) > 0)
			*ptr++ = val;
	}

	size_t *wptr = (size_t *)ptr;
	union {
		unsigned char data[sizeof *wptr];
		size_t val;
	} wval;

	size_t i;
	for (i = 0; i < sizeof *wptr; ++i) {
		wval.data[i] = val;
	}

	while (len > 0) {
		*wptr++ = wval.val;
		len -= sizeof *wptr;
	}
	
	return dest;
}

int strncmp(const char *left, const char *right, size_t count)
{
	int cmp = 0;
	while (cmp == 0 && *left && *right && count > 0) {
		cmp = (*left - *right);
		left++;
		right++;
		count--;
	}

	return (cmp != 0) ? (cmp) : (int)(count);
}