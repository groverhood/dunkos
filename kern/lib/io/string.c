#include <string.h>

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
	register unsigned char *ptr = (unsigned char*)dest;
	while (len-- > 0)
		*ptr++ = val;
	return dest;
}